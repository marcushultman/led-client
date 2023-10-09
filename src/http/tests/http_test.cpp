#include "http/http.h"

#include <future>

int main() {
  auto res = async::Thread::create("response");
  auto http = http::Http::create();

  std::promise<void> done;
  size_t num_res = 0;
  auto callback = [&](http::Response res) {
    ++num_res;
    printf("response %d (%s)\n", int(num_res), res.body.substr(0, 32).c_str());
    if (num_res == 2) {
      done.set_value();
    }
  };

  printf("request#1\n");
  auto req1 = http->request(
      {
          .url = "https://google.com",
      },
      {.post_to = res->scheduler(), .callback = callback});
  printf("request#2\n");
  auto req2 = http->request(
      {
          .url = "https://bing.com",
      },
      {.post_to = res->scheduler(), .callback = callback});

  done.get_future().get();
  printf("done\n");

  //
  return 0;
}
