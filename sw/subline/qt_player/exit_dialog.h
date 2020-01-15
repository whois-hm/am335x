#pragma once
#include "define.h"


class exit_dialog : public lmdialog
{
    Q_OBJECT

    virtual void readyto_show();

    QPushButton _close_button;
    QPushButton _confirm_button;    
    QLabel _exit_label;

public:
     exit_dialog(QWidget *parent = 0);
    virtual ~exit_dialog();
    static lmdialog *create_dialog(QWidget *p);
private slots:
    void slot_click_cancel(bool check);
    void slot_click_confirm(bool check);


};



