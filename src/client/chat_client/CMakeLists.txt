add_library(chat_client STATIC chat_client.cpp chat_client.hpp)

target_link_libraries(chat_client 
    PUBLIC
        gRPC::grpc++ 
        chat_proto
        rpp

)

target_include_directories(chat_client PUBLIC .)