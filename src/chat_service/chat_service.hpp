#pragma once

#include <chat.grpc.pb.h>

#include <rpp/rpp.hpp>

namespace ChatService 
{
    using UserSubject    = rpp::subjects::publish_subject<Proto::Event::Message>;
    using UserObservable = rpp::grouped_observable<std::string, Proto::Event::Message, decltype(std::declval<UserSubject>().get_observable())>;

    class Service final : public Proto::Server::CallbackService 
    {
    public:
        Service() = default;

        grpc::ServerBidiReactor<Proto::Event::Message, Proto::Event>* ChatStream(grpc::CallbackServerContext* ctx) override;

    private:

        struct State
        {
            rpp::dynamic_observer<UserObservable> user_observables;
            rpp::dynamic_observable<Proto::Event> all_events;
            rpp::disposable_wrapper               disposable;
        };

        static State InitState();
        State m_state = InitState();
    };
}