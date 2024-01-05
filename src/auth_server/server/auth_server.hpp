#pragma once

#include <auth.grpc.pb.h>

namespace AuthService 
{
    class Service final : public Proto::Server::Service 
    {
    public:
        Service(std::unordered_map<std::string, size_t> initial_users);

        grpc::Status Login(grpc::ServerContext *context, 
                           const Proto::LoginRequest *request,
                           Proto::LoginResponse *response) override;

        static size_t hash_password(std::string_view data) 
        {
            return std::hash<std::string_view>{}(data);
        }
    private:
        std::unordered_map<std::string, size_t> m_users{};
    };
} // namespace AuthService