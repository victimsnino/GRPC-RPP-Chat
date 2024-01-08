#include "chat_service.hpp"

#include <rpp/rpp.hpp>

#include <common.hpp>

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
            std::cout << ev.Utf8DebugString() << std::endl;
        });

        return State{.user_observables = subj.get_observer().as_dynamic(), .all_events = events, .disposable = events.connect()};
    }
    
    grpc::ServerBidiReactor<Proto::Event_Message, Proto::Event>* Service::ChatStream(grpc::CallbackServerContext* ctx)
    {
        class Reactor final : public grpc::ServerBidiReactor<Proto::Event_Message, Proto::Event>
        {
            static auto InitObserver(const rpp::dynamic_observer<UserObservable>& user_observables)
            {
                UserSubject subject{};
                user_observables.on_next(UserObservable{"user", subject.get_observable()});
                return subject.get_observer();
            }

        public:
            Reactor(const rpp::dynamic_observer<UserObservable>& user_observables, const rpp::dynamic_observable<Proto::Event>& all_events)
                : m_observer{InitObserver(user_observables)}
                , m_disposable{all_events
                               | rpp::ops::subscribe_with_disposable([this](const Proto::Event& user_events) {
                                     std::lock_guard lock{m_write_mutex};
                                     m_write.push_back(user_events);
                                     if (m_write.size() == 1) {
                                        StartWrite(&m_write.front());
                                     }
                                 })}
            {
                

                StartRead(&m_read);
            }

        private:

            void OnReadDone(bool ok) override 
            {
                if (!ok)
                {
                    Finish(grpc::Status::OK);
                    return;
                }

                m_observer.on_next(m_read);
                StartRead(&m_read);
            }

            void OnWriteDone(bool ok) override 
            {
                ENSURE(ok);

                std::lock_guard lock{m_write_mutex};
                ENSURE(!m_write.empty());
                m_write.pop_front();

                if (!m_write.empty()) {
                    StartWrite(&m_write.front());
                }
            }

            void OnDone() override 
            {
                m_disposable.dispose();
                m_observer.on_completed();
                delete this;
            }

        private:
            decltype(std::declval<UserSubject>().get_observer()) m_observer{};
            rpp::disposable_wrapper                              m_disposable{};

            Proto::Event::Message m_read{};

            std::mutex               m_write_mutex{};
            std::deque<Proto::Event> m_write{};
        };

        const auto new_handler = new Reactor(m_state.user_observables, m_state.all_events);
        return new_handler;
    }
}
