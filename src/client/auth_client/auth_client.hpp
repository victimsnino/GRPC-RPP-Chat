#pragma once

#include <auth.pb.h>
#include <grpcpp/support/status.h>

#include <string>
#include <variant>

namespace AuthClient {
    std::variant<std::string, grpc::Status, AuthService::Proto::FailedLoginResponse> Authenicate(std::string login, std::string password);
}