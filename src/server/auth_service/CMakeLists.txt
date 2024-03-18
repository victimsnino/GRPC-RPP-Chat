add_library(auth_service 
    STATIC 
        auth_service.cpp
        auth_service.hpp
)

target_link_libraries(auth_service 
    PUBLIC
        auth_proto 
        gRPC::grpc++
    PRIVATE 
        jwt-cpp
        common
)

target_include_directories(auth_service PUBLIC .)

if (BUILD_TESTS)
    add_subdirectory(ut)
endif()

add_subdirectory(proto)