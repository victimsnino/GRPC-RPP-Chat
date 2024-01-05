add_executable(client_app main.cpp)

target_link_libraries(client_app PRIVATE auth_client)
