#include "chat_service.hpp"

#include <rpp/rpp.hpp>

namespace ChatService 
{
    Service::State Service::InitState()
    {
        rpp::subjects::serialized_subject<UserObservable> subj{};

        auto events = subj.get_observable()
                    | rpp::ops::flat_map([](const UserObservable& observable) {
                          auto event = ChatService::Proto::Event{};
                          event.set_user(observable.get_key());
                          event.mutable_login();
                          const auto login_obs = rpp::source::just(event).as_dynamic();

                          event.mutable_logout();
                          const auto logout_obs = rpp::source::just(event).as_dynamic();
                          const auto messages   = observable
                                              | rpp::ops::map([](const Proto::Event::Message& message) {
                                                    Proto::Event event{};
                                                    event.mutable_message()->CopyFrom(message);
                                                    return event;
                                                });
                          return rpp::source::concat(login_obs, messages.as_dynamic(), logout_obs);
                      })
                    | rpp::ops::publish();
                    
        events.subscribe([](const Proto::Event& ev)
        {
            std::cout << ev.Utf8DebugString();
        });

        return State{.user_observables = subj.get_observer().as_dynamic(), .all_events = events, .disposable = events.connect()};
    }
    
    grpc::ServerBidiReactor<Proto::Event_Message, Proto::Event>* Service::ChatStream(grpc::CallbackServerContext* ctx)
    {
        struct Result final : public grpc::ServerBidiReactor<Proto::Event_Message, Proto::Event>
        {
            void OnReadDone(bool ok) override 
            {
                if (!ok)
                {
                    subject.get_observer().on_completed();
                    Finish(grpc::Status::OK);
                    return;
                }

                subject.get_observer().on_next(message);
                StartRead(&message);
            }

            void OnWriteDone(bool /*ok*/) override 
            {

            }

            void OnDone() override 
            {
                subject.get_observer().on_completed();
                delete this;
            }

            UserSubject subject{};
            Proto::Event::Message message{};
        };

        const auto new_handler = new Result();
        m_state.user_observables.on_next(UserObservable{"user", new_handler->subject.get_observable()});
        return new_handler;
    }
}
