#include <QApplication>
#include <QMainWindow>
#include <QLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>

#include <rppqt/rppqt.hpp>
#include <rpp/rpp.hpp>

#include <auth_client.hpp>
#include <chat_client.hpp>

int main(int argc, char* argv[])
{
    QApplication app{argc, argv};

    const auto window = new QMainWindow();
    const auto layout = new QVBoxLayout();

    window->setCentralWidget(new QWidget);
    window->centralWidget()->setLayout(layout);

    const auto login_field = new QLineEdit();
    const auto password_field = new QLineEdit();
    password_field->setEchoMode(QLineEdit::Password);
    const auto login_button = new QPushButton("Login");
    const auto login_error = new QLabel();

    const auto chat = new QTextEdit();
    chat->setVisible(false);
    chat->setReadOnly(true);

    const auto new_message_line = new QLineEdit();
    new_message_line->setVisible(false);
    
    layout->addWidget(new QLabel("Login: "));
    layout->addWidget(login_field);   

    layout->addWidget(new QLabel("Password: "));
    layout->addWidget(password_field);

    layout->addWidget(login_button);  
    layout->addWidget(login_error);
    layout->addWidget(chat);
    layout->addWidget(new_message_line);

    const auto wrap_text_field = [](QLineEdit* field) {
        const auto field_filled = rppqt::source::from_signal(*field, &QLineEdit::textChanged)
        | rpp::operators::map([](const QString& text) { return !text.isEmpty(); })
        | rpp::operators::start_with(false)
        | rpp::operators::multicast(rpp::subjects::replay_subject<bool>{1})
        | rpp::operators::ref_count();

        field_filled.subscribe([field](bool valid) { field->setStyleSheet(valid ? "border: 1px solid black" : "border: 1px solid red"); });
        return field_filled;
    };

    const auto login_result =
        rppqt::source::from_signal(*login_button, &QPushButton::clicked)
        | rpp::operators::map([login_field, password_field](auto) {
              return AuthClient::Authenicate(login_field->text().toStdString(), password_field->text().toStdString());
          })
        | rpp::operators::publish()
        | rpp::operators::ref_count();

    wrap_text_field(login_field)
        | rpp::operators::combine_latest([](bool login, bool password) { return login && password; }, wrap_text_field(password_field))
        | rpp::operators::combine_latest([](bool fields, bool login) { return fields && !login; }, login_result | rpp::operators::map([](const auto& result) {
                                                                                                       return std::holds_alternative<std::string>(result);
                                                                                                   }) | rpp::operators::start_with(false))
        | rpp::operators::subscribe([=](bool valid) { login_button->setEnabled(valid); });

    const auto handler = login_result
    | rpp::operators::filter([](const auto& result) {
        return std::holds_alternative<std::string>(result);
    })
    | rpp::operators::map([](const auto& result) {
        return std::get<std::string>(result);
    })
    | rpp::operators::map([](const auto& token) {
        return ChatClient::Handler{token};
    })
    | rpp::operators::multicast(rpp::subjects::replay_subject<ChatClient::Handler>{1})
    | rpp::operators::ref_count();

    handler
    | rpp::operators::flat_map([](const ChatClient::Handler& h){ return h.GetEvents();})
    | rpp::operators::map([](const ChatService::Proto::Event& ev) { return QString::fromStdString(ev.ShortDebugString()); })
    | rpp::operators::scan(QString{}, [](const QString& acc, const QString& ev) { return acc + ev + "\n"; })
    // | rpp::operators::observe_on(rppqt::schedulers::main_thread_scheduler{})
    | rpp::operators::subscribe([chat](const QString& text) { chat->setText(text); });
    
    login_result
    // | rpp::operators::observe_on(rppqt::schedulers::main_thread_scheduler{})
    | rpp::operators::subscribe([=](const auto& result) {
        if (std::holds_alternative<std::string>(result)) {
            login_error->setText("Logged!");
            password_field->setEnabled(false);
            login_field->setEnabled(false);
            chat->setVisible(true);
            new_message_line->setVisible(true);
        }
        if (const auto custom_error = std::get_if<AuthService::Proto::FailedLoginResponse>(&result))
            login_error->setText(QString::fromStdString(AuthService::Proto::FailedLoginResponse::Status_Name(custom_error->status())));
        else if (const auto grpc = std::get_if<grpc::Status>(&result))
            login_error->setText(QString::fromStdString(grpc->error_message()));
    });


    window->show();
    return app.exec();
}