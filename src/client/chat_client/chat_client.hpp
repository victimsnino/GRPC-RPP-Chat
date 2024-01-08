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
        ~Handler() noexcept;

        void SendMessage(const std::string& message) const;
        const rpp::dynamic_observable<ChatService::Proto::Event>& GetEvents() const;

    private:
        grpc::ClientContext                                         m_context{};
        struct State
        {
            rpp::dynamic_observable<ChatService::Proto::Event> events;
            rpp::dynamic_observer<std::string>                 messages;
        };

        static State InitState(grpc::ClientContext& ctx, const std::string& token);
        State m_state;
    };
}