#pragma once

#include <chat.pb.h>
#include <grpc++/client_context.h>

#include <string>

namespace ChatClient
{
    class Handler
    {
    public:
        Handler(std::string token);
        ~Handler() noexcept;

        void SendMessage();

    private:
        grpc::ClientContext m_context{};
    };
}