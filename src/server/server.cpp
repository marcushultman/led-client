#include "server.h"

#include <asio.hpp>
#include <charconv>
#include <iostream>

#include "async/scheduler.h"

struct ServerImpl : Server {
  using tcp = asio::ip::tcp;

  ServerImpl(async::Scheduler &main_scheduler, OnRequest on_request)
      : _main_scheduler{main_scheduler},
        _on_request{std::move(on_request)},
        _thread{async::Thread::create("asio")},
        _asio_work{_thread->scheduler().schedule([this] {
          _acceptor.set_option(tcp::acceptor::reuse_address(true));
          _acceptor.listen();
          accept();
          _ctx.run();
        })} {}
  ~ServerImpl() { _ctx.stop(); }

  void accept() {
    _acceptor.async_accept(_ctx, [this](auto err, tcp::socket peer) {
      if (err) {
        return;
      }
      auto req = readRequest(peer);
      writeResponse(peer);
      accept();

      std::cout << req.method << " " << req.path << " " << req.action << "\n"
                << req.body << std::endl;

      _main_work = _main_scheduler.schedule([this, req] { _on_request(req); });
    });
  }

  int port() const final { return _acceptor.local_endpoint().port(); }

 private:
  ServerRequest readRequest(tcp::socket &peer) {
    size_t offset = 0, size = std::max<size_t>(_buffer.size(), 128);
    std::string_view buffer;
    for (;;) {
      _buffer.resize(size);
      auto bytes_read =
          peer.read_some(asio::mutable_buffer(_buffer.data() + offset, _buffer.size() - offset));
      if (offset + bytes_read < size) {
        buffer = std::string_view(_buffer.data(), offset + bytes_read);
        break;
      }
      offset += size;
      size *= 2;
    }

    ServerRequest req;

    req.method = buffer.substr(0, buffer.find_first_of(' '));
    buffer.remove_prefix(req.method.size() + 1);

    req.path = buffer.substr(0, buffer.find_first_of(' '));
    buffer.remove_prefix(buffer.find_first_of("\n") + 1);

    while (buffer.find('\r') != 0) {
      if (buffer.find("action") == 0) {
        std::from_chars(buffer.begin() + 8, buffer.end(), req.action);
      }
      buffer.remove_prefix(buffer.find_first_of("\n") + 1);
    }

    buffer.remove_prefix(buffer.find_first_of("\n") + 1);
    req.body = buffer;

    return req;
  }

  void writeResponse(tcp::socket &peer) {
    auto body = std::string(
        "<html><body><form method=\"post\"><input name=\"text\" "
        "placeholder=\"\"><input type=\"submit\"></form></body></html>");
    peer.send(
        asio::buffer("HTTP/1.0 200 OK\r\n"
                     "content-length: " +
                     std::to_string(body.size()) +
                     "\r\n"
                     "content-type: text/html\r\n\r\n" +
                     body));
  }

  async::Scheduler &_main_scheduler;
  OnRequest _on_request;

  asio::io_context _ctx;
  tcp::acceptor _acceptor{_ctx, tcp::endpoint(tcp::v4(), 8080)};
  std::vector<char> _buffer;

  std::unique_ptr<async::Thread> _thread;
  async::Lifetime _asio_work;
  async::Lifetime _main_work;
};

std::unique_ptr<Server> makeServer(async::Scheduler &main_scheduler, OnRequest on_request) {
  return std::make_unique<ServerImpl>(main_scheduler, on_request);
}
