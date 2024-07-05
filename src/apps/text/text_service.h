#pragma once

#include <charconv>
#include <queue>

#include "font/font.h"
#include "http/http.h"
#include "present/presenter.h"

struct TextService : present::Presentable {
  TextService(async::Scheduler &main_scheduler, present::PresenterQueue &presenter);

  http::Response handleRequest(http::Request req);

  void onStart(spotiled::Renderer &) final;
  void onStop() final;

 private:
  async::Scheduler &_main_scheduler;
  present::PresenterQueue &_presenter;
  std::unique_ptr<font::TextPage> _text = font::TextPage::create();
  std::queue<http::Request> _requests;
  std::shared_ptr<void> _sentinel;
};
