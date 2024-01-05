add_executable(auth_server_main main.cpp)
target_link_libraries(auth_server_main PRIVATE auth_server)

add_subdirectory(server)
add_subdirectory(proto)