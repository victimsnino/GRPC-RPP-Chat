#include <auth.grpc.pb.h>
#include <auth.pb.h>
#include <grpc++/create_channel.h>

namespace AuthClient
{
    std::variant<std::string, grpc::Status, AuthService::Proto::FailedLoginResponse> Authenicate(std::string login, std::string password)
    {
        const auto stub = AuthService::Proto::Server::NewStub(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

        AuthService::Proto::LoginRequest request{};
        request.set_login(std::move(login));
        request.set_password(password);

        AuthService::Proto::LoginResponse response{};
        grpc::ClientContext               context;

        const auto status = stub->Login(&context, request, &response);

        if (status.ok())
            return response.token();

        if (AuthService::Proto::FailedLoginResponse custom_error{}; custom_error.ParseFromString(status.error_details()))
            return custom_error;

        return status;
    }
}