add_library(auth_server 
    STATIC 
        auth_server.cpp
        auth_server.hpp
)

target_link_libraries(auth_server 
    PUBLIC
        auth_proto 
        gRPC::grpc++
        jwt-cpp
    PRIVATE 
        common
)

target_include_directories(auth_server PUBLIC .)

if (BUILD_TESTS)
    add_subdirectory(ut)
endif()

add_subdirectory(app)
add_subdirectory(proto)