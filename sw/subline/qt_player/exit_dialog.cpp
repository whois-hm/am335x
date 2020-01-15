#include "exit_dialog.h"

exit_dialog::exit_dialog(QWidget *parent) :
    lmdialog(parent),
    _close_button("close", this),
    _confirm_button("confirm", this),
    _exit_label("exit?", this)
{    
    _exit_label.setAlignment(Qt::AlignCenter);
    connect(&_close_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_cancel(bool)));
    connect(&_confirm_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_confirm(bool)));
}

exit_dialog::~exit_dialog()
{

}
void exit_dialog::readyto_show()
{
    setGeometry(parent->x() + 10,
                parent->y() + (parent->height() / 4),
                parent->width() - 20,
                parent->height() / 2);
    _exit_label.setGeometry(0, this->height() / 2 - 25, this->width(), 25);
    _close_button.setGeometry(this->width() - 50 - 10,
                              this->height() - 50 + 10,
                              50,
                              30);
    _confirm_button.setGeometry(this->width() - 50 - 10 - 50 - 10,
                              this->height() - 50 + 10,
                              50,
                              30);

}

void exit_dialog::slot_click_cancel(bool check)
{
    Q_UNUSED(check);
    setCompleteString("");
    reject();
}
void exit_dialog::slot_click_confirm(bool check)
{
    Q_UNUSED(check);
    setCompleteString("exit ok");
    accept();
}

lmdialog *exit_dialog::create_dialog(QWidget *p)
{
    return new exit_dialog(p);
}
lmdialog *create_exit_dialog(QWidget *p)
{
    return exit_dialog::create_dialog(p);
}
