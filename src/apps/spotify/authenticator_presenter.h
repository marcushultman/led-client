#pragma once

#include <functional>
#include <memory>
#include <string>

#include "async/scheduler.h"
#include "http/http.h"
#include "present/presenter.h"

namespace spotify {

struct AuthenticatorPresenter {
  using AccessTokenCallback =
      std::function<void(std::string access_token, std::string refresh_token)>;
  virtual ~AuthenticatorPresenter() = default;
  virtual void finishPresenting() = 0;

  static std::unique_ptr<AuthenticatorPresenter> create(async::Scheduler &main_scheduler,
                                                        http::Http &,
                                                        present::PresenterQueue &presenter,
                                                        bool verbose,
                                                        AccessTokenCallback);
};

}  // namespace spotify
