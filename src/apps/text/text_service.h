#pragma once

#include <queue>

#include "font/font.h"
#include "http/http.h"
#include "present/presenter.h"
#include "util/spotiled/spotiled.h"

struct TextService : present::Presentable {
  TextService(async::Scheduler &main_scheduler, present::PresenterQueue &);

  http::Response handleRequest(http::Request req);

  void onStart() final;
  void onRenderPass(spotiled::LED &, std::chrono::milliseconds elapsed) final;

 private:
  async::Scheduler &_main_scheduler;
  present::PresenterQueue &_presenter;
  std::unique_ptr<font::TextPage> _text = font::TextPage::create();
  std::queue<http::Request> _requests;
  std::chrono::milliseconds _timeout;
  double _speed = 0;
};
