add_executable(auth_service_ut
    auth_service_ut.cpp
)

target_link_libraries(auth_service_ut PRIVATE snitch::snitch auth_service)