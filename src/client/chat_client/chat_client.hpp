#pragma once

#include <chat.pb.h>
#include <grpc++/support/sync_stream.h>
#include <string>


namespace ChatClient {
    class Handler
    {
    public:
        Handler(std::string token);
    private:
        std::unique_ptr<grpc::ClientReaderWriter<ChatService::Proto::Event_Message, ChatService::Proto::Event>> m_stream;
    };
}