add_library(client_auth STATIC client_auth.cpp client_auth.hpp)

target_link_libraries(client_auth 
    PUBLIC
        gRPC::grpc++ 
        auth_proto 
)
target_include_directories(client_auth PUBLIC .)