#include "server.h"

#include <asio.hpp>
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
      auto method = readRequestMethod(peer);
      writeResponse(peer);
      accept();

      if (method == "GET") {
        _main_work = _main_scheduler.schedule(async::Fn(_on_request));
      }
    });
  }

  int port() const final { return _acceptor.local_endpoint().port(); }

 private:
  std::string_view readRequestMethod(tcp::socket &peer) {
    auto bytes_read = peer.read_some(asio::mutable_buffer(_buffer.data(), _buffer.size()));
    auto request = std::string_view(_buffer.data(), bytes_read);
    return request.substr(0, request.find_first_of(' '));
  }

  void writeResponse(tcp::socket &peer) {
    auto body = std::string("<html><body>OK</body></html>");
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
  std::array<char, 128> _buffer;

  std::unique_ptr<async::Thread> _thread;
  async::Lifetime _asio_work;
  async::Lifetime _main_work;
};

std::unique_ptr<Server> makeServer(async::Scheduler &main_scheduler, OnRequest on_request) {
  return std::make_unique<ServerImpl>(main_scheduler, on_request);
}
