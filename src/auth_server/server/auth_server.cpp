#include "auth_server.hpp"

#include <auth.pb.h>

#include <common.hpp>

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
    Service::Service(std::unordered_map<std::string, size_t> initial_users)
        : m_users{std::move(initial_users)}
    {
    }

    grpc::Status Service::Login(grpc::ServerContext* context, const Proto::LoginRequest* request, Proto::LoginResponse* response) 
    {
        ENSURE(request);
        
        if (request->login().empty()) {
            return BuildStatus(grpc::StatusCode::UNAUTHENTICATED, "Login field must be filled", Proto::FailedLoginResponse::EmptyLogin);
        } 

        if (request->password().empty()) {
            return BuildStatus(grpc::StatusCode::UNAUTHENTICATED, "Password field must be filled", Proto::FailedLoginResponse::EmptyPassword);
        }

        const auto itr = m_users.find(request->login());

        if (itr == m_users.cend()) {
            return BuildStatus(grpc::StatusCode::UNAUTHENTICATED, "No such an user", Proto::FailedLoginResponse::UserNotFound);
        }

        if (itr->second != hash_password(request->password())) {
            return BuildStatus(grpc::StatusCode::UNAUTHENTICATED, "Invalid password", Proto::FailedLoginResponse::InvalidPassword);
        }
        
        return grpc::Status::OK;
    }

    
}

