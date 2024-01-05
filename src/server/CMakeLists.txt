add_executable(server main.cpp)
target_link_libraries(server PRIVATE auth_service)