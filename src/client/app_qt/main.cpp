#include <QApplication>
#include <QMainWindow>
#include <QLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

#include <rppqt/rppqt.hpp>
#include <rpp/rpp.hpp>

int main(int argc, char* argv[])
{
    QApplication app{argc, argv};

    auto window = new QMainWindow();
    auto layout = new QVBoxLayout();

    window->setCentralWidget(new QWidget);
    window->centralWidget()->setLayout(layout);

    auto login_field = new QLineEdit();
    auto password_field = new QLineEdit();
    password_field->setEchoMode(QLineEdit::Password);
    auto login_button = new QPushButton("Login");
    
    layout->addWidget(new QLabel("Login: "));
    layout->addWidget(login_field);   

    layout->addWidget(new QLabel("Password: "));
    layout->addWidget(password_field);

    layout->addWidget(login_button);  

    const auto wrap_text_field = [](QLineEdit* field) {
        const auto field_filled = rppqt::source::from_signal(*field, &QLineEdit::textChanged)
        | rpp::operators::map([](const QString& text) { return !text.isEmpty(); })
        | rpp::operators::start_with(false)
        | rpp::operators::multicast(rpp::subjects::replay_subject<bool>{1})
        | rpp::operators::ref_count();

        field_filled.subscribe([field](bool valid) { field->setStyleSheet(valid ? "border: 1px solid black" : "border: 1px solid red"); });
        return field_filled;
    };
   
    wrap_text_field(login_field)
        | rpp::operators::combine_latest([](bool login, bool password) { return login && password;}, wrap_text_field(password_field))
        | rpp::operators::subscribe([=](bool valid) { login_button->setEnabled(valid); });
    
    // rppqt::source::from_signal(*login_button, &QPushButton::click)
    // | rpp::source::map([](bool) { 
        
    //  });

    window->show();
    return app.exec();
}