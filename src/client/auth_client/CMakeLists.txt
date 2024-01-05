add_library(auth_client STATIC auth_client.cpp auth_client.hpp)

target_link_libraries(auth_client 
    PUBLIC
        gRPC::grpc++ 
        auth_proto 
)
target_include_directories(auth_client PUBLIC .)