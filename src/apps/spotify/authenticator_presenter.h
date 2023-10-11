#pragma once

#include <functional>
#include <memory>
#include <string>

#include "apps/settings/brightness_provider.h"
#include "async/scheduler.h"
#include "http/http.h"
#include "present/presenter.h"

namespace spotify {

struct AuthenticatorPresenter {
  using AccessTokenCallback =
      std::function<void(std::string access_token, std::string refresh_token)>;
  virtual ~AuthenticatorPresenter() = default;
  static std::unique_ptr<AuthenticatorPresenter> create(async::Scheduler &main_scheduler,
                                                        http::Http &,
                                                        present::PresenterQueue &presenter,
                                                        settings::BrightnessProvider &brightness,
                                                        bool verbose,
                                                        AccessTokenCallback);
};

}  // namespace spotify
