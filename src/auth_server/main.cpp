#include <auth_server.hpp>

#include <grpc++/server_builder.h>

#include <string>

int main()
{
   AuthService::Service service{{{"test", AuthService::Service::hash_password("test")}}};
   grpc::ServerBuilder builder{};

    std::string server_address("0.0.0.0:50051");

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    auto server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
}