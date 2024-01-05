#include <auth_service.hpp>
#include <chat_service.hpp>
#include <grpc++/server_builder.h>

#include <string>

int main()
{
    AuthService::Service auth_service{{{"test", AuthService::Service::HashPassword("test")}, {"test2", AuthService::Service::HashPassword("test")}}, "secret"};
    ChatService::Service chat_service{};

    grpc::ServerBuilder  builder{};

    std::string server_address("0.0.0.0:50051");

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&auth_service);
    builder.RegisterService(&chat_service);

    auto server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
}