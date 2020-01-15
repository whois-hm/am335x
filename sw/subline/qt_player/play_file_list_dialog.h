#pragma once
#include "define.h"


class play_file_list_dialog : public lmdialog
{
    Q_OBJECT
     virtual void readyto_show();
    virtual void showEvent(QShowEvent *event);
    QStringList _filelist;
    QStringList _filters;
    QListWidget _listwidget;
    QPushButton _close_button;
    QPushButton _confirm_button;
    QPushButton _info_button;
    QTextEdit _info_text;
    QPushButton _info_text_close_button;
public:
     play_file_list_dialog(QWidget *parent = 0);
    virtual ~play_file_list_dialog();

    static lmdialog *create_dialog(QWidget *p);

private slots:
    void slot_click_cancel(bool check);
    void slot_click_confirm(bool check);
    void slot_click_info(bool check);
    void slot_click_info_text_cancel(bool check);
    void slot_click_listwidget(int row);

};



