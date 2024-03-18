
add_library(chat_service 
    STATIC 
        chat_service.cpp
        chat_service.hpp
)

target_link_libraries(chat_service 
    PUBLIC
        gRPC::grpc++ 
        chat_proto 
        rpp
    PRIVATE 
        jwt-cpp
        common
)

target_include_directories(chat_service PUBLIC .)

add_subdirectory(proto)