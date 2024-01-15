#include "server.h"

#include <algorithm>
#include <asio.hpp>
#include <iostream>
#include <numeric>
#include <set>
#include <string_view>

#include "async/scheduler.h"
#include "http/util.h"

namespace http {
namespace {

Method methodFromString(std::string_view str) {
  auto cmp = [](auto lhs, auto rhs) { return std::toupper(lhs) == rhs; };
  if (std::equal(str.begin(), str.end(), "GET", cmp)) {
    return Method::GET;
  } else if (std::equal(str.begin(), str.end(), "POST", cmp)) {
    return Method::POST;
  } else if (std::equal(str.begin(), str.end(), "PUT", cmp)) {
    return Method::PUT;
  } else if (std::equal(str.begin(), str.end(), "DELETE", cmp)) {
    return Method::DELETE;
  } else if (std::equal(str.begin(), str.end(), "HEAD", cmp)) {
    return Method::HEAD;
  } else if (std::equal(str.begin(), str.end(), "OPTIONS", cmp)) {
    return Method::OPTIONS;
  }
  return Method::UNKNOWN;
}

struct Connection : public std::enable_shared_from_this<Connection> {
  using tcp = asio::ip::tcp;
  using OnDone = std::function<void(std::shared_ptr<Connection>)>;

  Connection(async::Scheduler &main_scheduler,
             RequestHandler &handler,
             asio::io_context &ctx,
             tcp::socket &&peer,
             OnDone on_done)
      : _main_scheduler{main_scheduler},
        _handler{handler},
        _ctx{ctx},
        _peer{std::move(peer)},
        _on_done{std::move(on_done)} {}

  void start() {
    asio::error_code err;
    auto req = readRequest(err);

    if (err) {
      std::cerr << "Failed to read request: " << err << std::endl;
      if (err != asio::error::operation_aborted) {
        _on_done(shared_from_this());
        _peer.close();
      }
      return;
    }

#if 0
      std::cout << int(req.method) << " " << req.url << " "
                << (req.headers.contains("action") ? req.headers["action"] : "") << "\n"
                << req.body << std::endl;
#endif

    _handler_work =
        _main_scheduler.schedule([this, self = shared_from_this(), req = std::move(req)]() mutable {
          if (auto *handler = std::get_if<SyncHandler>(&_handler)) {
            sendResponse((*handler)(std::move(req)));

          } else if (auto *handler = std::get_if<AsyncHandler>(&_handler)) {
            _main_work = (*handler)(
                std::move(req), [this, self](auto res) mutable { sendResponse(std::move(res)); },
                [this, self](auto offset, auto data, auto lifetime) mutable {
                  sendData(data, std::move(lifetime));
                });
          }
        });

    pollSocket();
  }

  void pollSocket() {
    _peer.async_read_some(asio::buffer(_buffer), [this, self = shared_from_this()](auto err, auto) {
      if (!_peer.is_open()) {
        return;
      }
      if (!err) {
        pollSocket();
        return;
      }
      if (err == asio::error::eof) {
        _handler_work = _main_scheduler.schedule([this, self] { _main_work.reset(); });
        asio::error_code ignored_err;
        _peer.shutdown(tcp::socket::shutdown_both, ignored_err);
      }
      if (err != asio::error::operation_aborted) {
        _on_done(self);
        _peer.close(err);
      }
    });
  }

  void sendResponse(http::Response res) {
    if (res.headers["content-length"].empty()) {
      res.headers["content-length"] = std::to_string(_content_length = res.body.size());
    } else {
      _content_length = std::stoll(res.headers["content-length"]);
    }

    if (res.headers["content-type"].empty()) {
      // todo: better default?
      res.headers["content-type"] = "text/html";
    }

    asio::post(_ctx, [this, self = shared_from_this(), res = std::move(res)]() mutable {
      writeResponse(std::move(res));
    });
  }

  void sendData(std::string_view data, http::Lifetime lifetime) {
    asio::post(_ctx, [this, self = shared_from_this(), data, lifetime]() mutable {
      writeData(data, std::move(lifetime));
    });
  }

 private:
  std::string_view readIntoBuffer(asio::error_code &err) {
    size_t offset = 0, size = std::max<size_t>(_buffer.size(), 128);
    std::string_view buffer;
    for (;;) {
      _buffer.resize(size);
      auto bytes_read = _peer.read_some(
          asio::mutable_buffer(_buffer.data() + offset, _buffer.size() - offset), err);
      if (offset + bytes_read < size || err) {
        return std::string_view(_buffer.data(), offset + bytes_read);
      }
      offset += bytes_read;
      size *= 2;
    }
  }

  Request readRequest(asio::error_code &err) {
    std::string_view buffer = readIntoBuffer(err);

    Request req;

    if (err) {
      return req;
    }

    auto method = buffer.substr(0, buffer.find_first_of(' '));
    req.method = methodFromString(method);
    buffer.remove_prefix(method.size() + 1);

    auto path = buffer.substr(0, buffer.find_first_of(' '));
    buffer.remove_prefix(buffer.find_first_of("\n") + 1);

    while (buffer.find('\r') != 0) {
      auto key = std::string(buffer.substr(0, buffer.find(":")));
      auto value = std::string(trim(buffer.substr(key.size() + 1)));
      std::transform(key.begin(), key.end(), key.begin(), ::tolower);
      req.headers[std::move(key)] = std::move(value);
      buffer.remove_prefix(buffer.find_first_of("\n") + 1);
    }

    if (auto it = req.headers.find("host"); it != req.headers.end()) {
      req.url = "http://" + it->second;
    }
    req.url += path;

    buffer.remove_prefix(buffer.find_first_of("\n") + 1);
    req.body = buffer;

    return req;
  }

  void writeResponse(http::Response res) {
    struct Handle {
      http::Response res;
      std::string status;
    };
    auto handle = std::make_shared<Handle>();
    handle->res = std::move(res);
    handle->status = std::to_string(handle->res.status);

    using namespace std::string_view_literals;
    auto buffers = std::vector<asio::const_buffer>{
        asio::buffer("HTTP/1.0 "sv),
        asio::buffer(handle->status),
        handle->res.status == 200 ? asio::buffer(" OK"sv) : asio::buffer(" No Content"sv),
        asio::buffer("\r\n"sv),
    };
    buffers.reserve(buffers.size() + handle->res.headers.size() * 4 + 2);

    for (auto &[key, value] : handle->res.headers) {
      buffers.insert(buffers.end(), {asio::buffer(key), asio::buffer(": "sv), asio::buffer(value),
                                     asio::buffer("\r\n"sv)});
    }
    buffers.push_back(asio::buffer("\r\n"sv));
    buffers.push_back(asio::buffer(handle->res.body));

    auto bytes = handle->res.body.size();

    sendBuffers(std::move(buffers), bytes, std::move(handle));
  }

  void writeData(std::string_view buffer, http::Lifetime &&lifetime) {
    if (_out_buffer) {
      _pending_send = [this, buffer, lifetime = std::move(lifetime)]() mutable {
        sendBuffers(asio::buffer(buffer), buffer.size(), std::move(lifetime));
      };
    } else {
      sendBuffers(asio::buffer(buffer), buffer.size(), std::move(lifetime));
    }
  }

  template <typename Buffers>
  void sendBuffers(Buffers &&buffers, int64_t num_bytes, http::Lifetime &&lifetime) {
    assert(!_out_buffer);
    _out_buffer = std::move(lifetime);
    _peer.async_send(buffers, [this, self = shared_from_this(), num_bytes](auto err, auto) {
      _out_buffer.reset();
      _bytes_sent += num_bytes;

      if (auto pending_send = std::exchange(_pending_send, {})) {
        return pending_send();
      } else if (_bytes_sent < _content_length) {
        return;
      }
      if (!err) {
        asio::error_code ignored_err;
        _peer.shutdown(tcp::socket::shutdown_both, ignored_err);
      }
      if (err != asio::error::operation_aborted) {
        _on_done(self);
        _peer.close(err);
      }
    });
  }

  async::Scheduler &_main_scheduler;
  RequestHandler &_handler;
  asio::io_context &_ctx;
  tcp::socket _peer;
  OnDone _on_done;
  std::vector<char> _buffer;
  http::Lifetime _out_buffer;
  std::function<void()> _pending_send;
  int64_t _bytes_sent = 0;
  int64_t _content_length = 0;
  async::Lifetime _handler_work;  // set on asio - runs on main
  async::Lifetime _main_work;     // set on main - runs on main
};

struct ServerImpl : Server {
  using tcp = asio::ip::tcp;

  ServerImpl(async::Scheduler &main_scheduler, RequestHandler handler)
      : _main_scheduler{main_scheduler},
        _handler{std::move(handler)},
        _thread{async::Thread::create("asio")} {
    _acceptor.set_option(tcp::acceptor::reuse_address(true));
    _acceptor.listen();
    accept();
    _asio_work = _thread->scheduler().schedule([this] { _ctx.run(); });
  }
  ~ServerImpl() {
    _acceptor.close();
    _ctx.stop();
  }

  int port() const final { return _acceptor.local_endpoint().port(); }

 private:
  void accept() {
    _acceptor.async_accept(_ctx, [this](auto err, tcp::socket peer) {
      if (!_acceptor.is_open()) {
        return;
      }
      if (!err) {
        auto connection =
            std::make_shared<Connection>(_main_scheduler, _handler, _ctx, std::move(peer),
                                         [this](auto conn) { _connections.erase(conn); });
        _connections.insert(connection);
        connection->start();
      } else {
        std::cerr << "Failed to accept connection: " << err << std::endl;
      }

      accept();
    });
  }

  async::Scheduler &_main_scheduler;
  RequestHandler _handler;

  asio::io_context _ctx;
  tcp::acceptor _acceptor{_ctx, tcp::endpoint(tcp::v4(), 8080)};

  std::unique_ptr<async::Thread> _thread;
  async::Lifetime _asio_work;

  std::set<std::shared_ptr<Connection>> _connections;
};

}  // namespace

std::unique_ptr<Server> makeServer(async::Scheduler &main_scheduler, RequestHandler handler) {
  return std::make_unique<ServerImpl>(main_scheduler, handler);
}

}  // namespace http
