#pragma once

#include <stdexcept>

#define ENSURE(condition) if (!condition) { throw std::runtime_error{" condition " #condition " failed"};  }

namespace Consts
{
    constexpr auto g_authenication_header = "x-custom-authenication-ticket";
    constexpr auto g_secret_seed = "super_secret_seed";
    constexpr auto g_issuer = "auth_service";
    constexpr auto g_login_field = "login";
}