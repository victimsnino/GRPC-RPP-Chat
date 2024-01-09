#include <auth_service.hpp>
#include <chat_service.hpp>
#include <grpc++/server_builder.h>

#include <string>
#include <common.hpp>


int main()
{
    AuthService::Service auth_service{{{"test", AuthService::Service::HashPassword("test")}, {"test2", AuthService::Service::HashPassword("test")}}, Consts::g_secret_seed};
    ChatService::Service chat_service{};

    grpc::ServerBuilder  builder{};

    std::string server_address("0.0.0.0:50051");

    builder.AddListeningPort(server_address, grpc::experimental::LocalServerCredentials(grpc_local_connect_type::LOCAL_TCP));
    builder.RegisterService(&auth_service);
    builder.RegisterService(&chat_service);

    auto server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
}