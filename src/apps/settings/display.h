#pragma once

#include "http/http.h"
#include "present/presenter.h"

namespace settings {

struct DisplayService : present::Presentable {
  DisplayService(present::PresenterQueue &);

  http::Response operator()(http::Request);

  void start(SpotiLED &, present::Callback) final;
  void stop() final;

 private:
  present::PresenterQueue &_presenter;
  int _brightness = 0;
};

}  // namespace settings
