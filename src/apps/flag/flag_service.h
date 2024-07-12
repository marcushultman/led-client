#pragma once

#include "http/http.h"
#include "present/presenter.h"
#include "util/spotiled/spotiled.h"

struct FlagService : present::Presentable {
  FlagService(present::PresenterQueue &);

  http::Response handleRequest(http::Request);

 private:
  void onRenderPass(spotiled::LED &, std::chrono::milliseconds elapsed) final;

  present::PresenterQueue &_presenter;
};
