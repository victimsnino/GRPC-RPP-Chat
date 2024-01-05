add_library(auth_server 
    STATIC 
        auth_server.cpp
        auth_server.hpp
)

target_link_libraries(auth_server 
    PUBLIC
        auth_proto 
        gRPC::grpc++ 
    PRIVATE 
        common
)

target_include_directories(auth_server PUBLIC .)

if (BUILD_TESTS)
    add_subdirectory(ut)
endif()