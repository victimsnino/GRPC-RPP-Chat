#pragma once

#include <chat.pb.h>
#include <grpc++/client_context.h>

#include <string>

#include <rpp/observables.hpp>

namespace ChatClient
{
    class Handler
    {
    public:
        Handler(const std::string& token);

        void SendMessage(const std::string& message) const;
        const rpp::dynamic_observable<ChatService::Proto::Event>& GetEvents() const;

    private:
        struct State;
        
        std::shared_ptr<State> m_state;
    };
}