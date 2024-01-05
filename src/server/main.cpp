#include <auth_service.hpp>
#include <grpc++/server_builder.h>

#include <string>

int main()
{
    AuthService::Service service{{{"test", AuthService::Service::HashPassword("test")}}, "secret"};
    grpc::ServerBuilder  builder{};

    std::string server_address("0.0.0.0:50051");

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    auto server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
}