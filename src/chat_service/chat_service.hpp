#pragma once

#include <chat.grpc.pb.h>

#include <rpp/subjects/synchronize_subject.hpp>

namespace ChatService 
{
    class Service final : public Proto::Server::CallbackService 
    {
    public:
        Service() = default;

        grpc::ServerBidiReactor<Proto::Event_Message, Proto::Event>* ChatStream(grpc::CallbackServerContext* ctx) override;
    private:

    };
}