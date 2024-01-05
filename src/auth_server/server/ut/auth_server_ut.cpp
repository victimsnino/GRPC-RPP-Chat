#include <snitch/snitch.hpp>

#include <auth_server.hpp>

TEST_CASE("ServerHandlesLogin") 
{
    constexpr auto login = "test";
    constexpr auto password = "password";
    AuthService::Service server{{{login, AuthService::Service::hash_password(password)}}};
    AuthService::Proto::LoginRequest request{};
    AuthService::Proto::LoginResponse response{};

    const auto make_request = [&]() {
        grpc::ServerContext ctx{};
        return server.Login(&ctx, &request, &response);
    };

    const auto parse_error_response = [&]() {
        const auto status = make_request();
        CHECK(status.error_code() == grpc::StatusCode::UNAUTHENTICATED);

        AuthService::Proto::FailedLoginResponse response{};
        CHECK(response.ParseFromString(status.error_details()));
        return response.status();
    };

    SECTION("Empty request") 
    {
        CHECK(parse_error_response() == AuthService::Proto::FailedLoginResponse::EmptyLogin);

        SECTION("set random login") 
        {
            request.set_login("123");
            CHECK(parse_error_response() == AuthService::Proto::FailedLoginResponse::EmptyPassword);

            SECTION("set random password") 
            {
                request.set_password("123");
                CHECK(parse_error_response() == AuthService::Proto::FailedLoginResponse::UserNotFound);
            }
        }
    }
    SECTION("Valid user with invalid password")
    {
        request.set_login(login);
        request.set_password("123");
        CHECK(parse_error_response() == AuthService::Proto::FailedLoginResponse::InvalidPassword);
    }
    SECTION("Valid user valid password")
    {
        request.set_login(login);
        request.set_password(password);
        CHECK(make_request().ok());
    }
}