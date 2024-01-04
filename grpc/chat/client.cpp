#include <chat.pb.h>
#include <chat.grpc.pb.h>
#include <grpc++/create_channel.h>

#define ENSURE(condition) if (!condition) { std::cout << " condition " << #condition << " failed"; throw 0; }

class ChatClient {
public:
    ChatClient(std::shared_ptr<grpc::Channel> channel)
        : m_stub{Server::NewStub(std::move(channel))}
    {}

    void SendMessage(const std::string& text) {
        grpc::ClientContext context;
        auto stream = m_stub->ChatStream(&context);

        Message::Text input{};
        input.set_text(text);

        ENSURE(stream->Write(input))
        ENSURE(stream->WritesDone())

        Message output{};
        while(stream->Read(&output)) {
            std::cout << ">> " << output.text().text() << std::endl;
        }
        ENSURE(stream->Finish().ok())
    }
private:
    std::unique_ptr<Server::Stub> m_stub;
};

int main()
{
    ChatClient client{grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials())};
    client.SendMessage("Hello");
    client.SendMessage("Hello 2");
    return 0;
}