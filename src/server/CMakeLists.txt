add_executable(server_app main.cpp)
target_link_libraries(server_app 
    PRIVATE 
        auth_service
        chat_service
)