
#include <auth_client.hpp>
#include <chat_client.hpp>

#include <rpp/operators/filter.hpp>

struct AuthenicationResult
{
    std::string token;
    std::string login;
};

AuthenicationResult Authenicate()
{
    while (true)
    {
        std::string login{};
        std::string password{};
        std::cout << "Enter login: ";
        std::cin >> login;
        std::cout << "Enter password: ";
        std::cin >> password;

        const auto result = AuthClient::Authenicate(login, password);
        if (const auto token = std::get_if<std::string>(&result))
            return {.token=*token, .login=login};

        if (const auto custom_error = std::get_if<AuthService::Proto::FailedLoginResponse>(&result))
        {
            std::cout << "Error: " << AuthService::Proto::FailedLoginResponse::Status_Name(custom_error->status()) << std::endl;
            continue;
        }

        if (const auto grpc = std::get_if<grpc::Status>(&result))
        {
            std::cout << "Error: " << grpc->error_message() << std::endl;
            continue;
        }
    }
}

int main()
{
    const auto [token, name] = Authenicate();
    std::cout << "Authenicated with token " << token << std::endl;
    ChatClient::Handler chat{token};

    chat.GetEvents()
    | rpp::operators::filter([&name](const ChatService::Proto::Event& ev) { return ev.user() != name;})
    | rpp::operators::subscribe([](const ChatService::Proto::Event& ev) { std::cout << ev.ShortDebugString() << std::endl; });

    std::string in{};
    while(true)
    {
        std::getline(std::cin, in);
        chat.SendMessage(in);
        in.clear();
    }
    return 0;
}