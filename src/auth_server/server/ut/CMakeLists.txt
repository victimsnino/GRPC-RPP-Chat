add_executable(auth_server_ut
    auth_server_ut.cpp
)

target_link_libraries(auth_server_ut PRIVATE snitch::snitch auth_server)