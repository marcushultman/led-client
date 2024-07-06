#pragma once

#include <charconv>
#include <queue>

#include "font/font.h"
#include "http/http.h"
#include "present/presenter.h"
#include "util/spotiled/spotiled.h"

struct TextService : present::Presentable {
  TextService(async::Scheduler &main_scheduler, spotiled::Renderer &, present::PresenterQueue &);

  http::Response handleRequest(http::Request req);

  void onStart() final;
  void onStop() final;

 private:
  async::Scheduler &_main_scheduler;
  spotiled::Renderer &_renderer;
  present::PresenterQueue &_presenter;
  std::unique_ptr<font::TextPage> _text = font::TextPage::create();
  std::queue<http::Request> _requests;
  std::shared_ptr<void> _sentinel;
};
