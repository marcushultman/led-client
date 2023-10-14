#include "server.h"

#include <algorithm>
#include <asio.hpp>
#include <iostream>

#include "async/scheduler.h"

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

std::string_view trim(std::string_view value) {
  while (value.size() && value.front() == ' ') {
    value = value.substr(1);
  }
  return value.substr(0, std::min(value.find_first_of("\r"), value.find_first_of("\n")));
}

}  // namespace

struct ServerImpl : Server {
  using tcp = asio::ip::tcp;

  ServerImpl(async::Scheduler &main_scheduler, OnRequest on_request)
      : _main_scheduler{main_scheduler},
        _on_request{std::move(on_request)},
        _thread{async::Thread::create("asio")},
        _asio_work_guard{_ctx.get_executor()},
        _asio_work{_thread->scheduler().schedule([this] {
          _acceptor.set_option(tcp::acceptor::reuse_address(true));
          _acceptor.listen();
          accept();
          _ctx.run();
        })} {}
  ~ServerImpl() {
    _asio_work_guard.reset();
    _ctx.stop();
  }

  void accept() {
    _acceptor.async_accept(_ctx, [this](auto err, tcp::socket peer) {
      if (err) {
        std::cerr << "Failed to accept connection: " << err << std::endl;
        return;
      }
      auto req = readRequest(peer, err);

      if (err) {
        std::cerr << "Failed to read request: " << err << std::endl;
        asio::post(_ctx, [this] { accept(); });
        return;
      }

#if 0
      std::cout << int(req.method) << " " << req.url << " "
                << (req.headers.contains("action") ? req.headers["action"] : "") << "\n"
                << req.body << std::endl;
#endif

      _main_work =
          _main_scheduler.schedule([this, peer = std::make_shared<tcp::socket>(std::move(peer)),
                                    req = std::move(req)]() mutable {
            asio::post(_ctx,
                       [this, peer = std::move(peer), res = _on_request(std::move(req))]() mutable {
                         writeResponse(*peer, std::move(res));
                         accept();
                       });
          });
    });
  }

  int port() const final { return _acceptor.local_endpoint().port(); }

 private:
  std::string_view readIntoBuffer(tcp::socket &peer, asio::error_code &err) {
    size_t offset = 0, size = std::max<size_t>(_buffer.size(), 128);
    std::string_view buffer;
    for (;;) {
      _buffer.resize(size);
      auto bytes_read = peer.read_some(
          asio::mutable_buffer(_buffer.data() + offset, _buffer.size() - offset), err);
      if (offset + bytes_read < size || err) {
        return std::string_view(_buffer.data(), offset + bytes_read);
      }
      offset += bytes_read;
      size *= 2;
    }
  }

  Request readRequest(tcp::socket &peer, asio::error_code &err) {
    std::string_view buffer = readIntoBuffer(peer, err);

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
      std::transform(value.begin(), value.end(), value.begin(), ::tolower);
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

  void writeResponse(tcp::socket &peer, http::Response res) {
    auto data = "HTTP/1.0 " + std::to_string(res.status) + " " +
                (res.status == 200 ? "OK" : "No Content") + "\r\n";
    if (res.headers["content-length"].empty()) {
      res.headers["content-length"] = std::to_string(res.body.size());
    }
    if (res.headers["content-type"].empty()) {
      res.headers["content-type"] = "text/html";
    }
    // todo: customize
    res.headers["access-control-allow-origin"] = "*";

    for (auto &[key, value] : res.headers) {
      data += key + ": " + value + "\r\n";
    }
    peer.send(asio::buffer(data + "\r\n" + res.body));
  }

  async::Scheduler &_main_scheduler;
  OnRequest _on_request;

  asio::io_context _ctx;
  tcp::acceptor _acceptor{_ctx, tcp::endpoint(tcp::v4(), 8080)};
  std::vector<char> _buffer;

  std::unique_ptr<async::Thread> _thread;
  asio::executor_work_guard<asio::io_context::executor_type> _asio_work_guard;
  async::Lifetime _asio_work;
  async::Lifetime _main_work;
};

std::unique_ptr<Server> makeServer(async::Scheduler &main_scheduler, OnRequest on_request) {
  return std::make_unique<ServerImpl>(main_scheduler, on_request);
}

}  // namespace http
