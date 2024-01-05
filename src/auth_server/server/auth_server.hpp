#pragma once

#include <auth.grpc.pb.h>

namespace AuthService
{
    class Service final : public Proto::Server::Service
    {
    public:
        Service(std::unordered_map<std::string, size_t> initial_users, std::string secret);

        grpc::Status Login(grpc::ServerContext       *context,
                           const Proto::LoginRequest *request,
                           Proto::LoginResponse      *response) override;

        static size_t HashPassword(std::string_view data);

        static std::string MakeToken(std::string login, std::string secret);

    private:
        std::unordered_map<std::string, size_t> m_users{};
        const std::string                       m_secret{};
    };
} // namespace AuthService