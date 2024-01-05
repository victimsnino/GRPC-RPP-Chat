#include "auth_service.hpp"

#include <auth.pb.h>
#include <common.hpp>
#include <jwt-cpp/jwt.h>

namespace AuthService
{
    namespace
    {
        grpc::Status BuildStatus(grpc::StatusCode code, const std::string& message, Proto::FailedLoginResponse::Status reason)
        {
            Proto::FailedLoginResponse response{};
            response.set_status(reason);
            return grpc::Status{code, message, response.SerializeAsString()};
        }
    }

    Service::Service(std::unordered_map<std::string, size_t> initial_users, std::string secret)
        : m_users{std::move(initial_users)}
        , m_secret{std::move(secret)}
    {
    }

    size_t Service::HashPassword(std::string_view data)
    {
        return std::hash<std::string_view>{}(data);
    }

    std::string Service::CreateToken(std::string login, std::string secret)
    {
        return jwt::create()
            .set_issuer("auth_service")
            .set_payload_claim("login", jwt::claim(std::move(login)))
            .sign(jwt::algorithm::hs256{std::move(secret)});
    }

    grpc::Status Service::Login(grpc::ServerContext* context, const Proto::LoginRequest* request, Proto::LoginResponse* response)
    {
        ENSURE(request);

        if (request->login().empty())
            return BuildStatus(grpc::StatusCode::UNAUTHENTICATED, "Login field must be filled", Proto::FailedLoginResponse::EmptyLogin);

        if (request->password().empty())
            return BuildStatus(grpc::StatusCode::UNAUTHENTICATED, "Password field must be filled", Proto::FailedLoginResponse::EmptyPassword);

        const auto itr = m_users.find(request->login());

        if (itr == m_users.cend())
            return BuildStatus(grpc::StatusCode::UNAUTHENTICATED, "No such an user", Proto::FailedLoginResponse::UserNotFound);

        if (itr->second != HashPassword(request->password()))
            return BuildStatus(grpc::StatusCode::UNAUTHENTICATED, "Invalid password", Proto::FailedLoginResponse::InvalidPassword);

        response->set_token(CreateToken(request->login(), m_secret));
        return grpc::Status::OK;
    }
}
