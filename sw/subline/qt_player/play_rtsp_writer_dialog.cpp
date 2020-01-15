#include "play_rtsp_writer_dialog.h"

play_rtsp_writer_dialog::play_rtsp_writer_dialog(QWidget *parent) :
    lmdialog(parent),
    _close_button("close", this),
    _confirm_button("confirm", this),
    _keyboard_show_button("keyboard", this),
    _info_sdp_button("info", this),
    _info_sdp_close_button("close", this),
    _url_label("input url", this),
    _info_text(this),
    _info_sdp_text(this),
    _keyboard(QRect(parent->x(), parent->y(), parent->width(), parent->height()), this),
    _rtsp_schelduler(nullptr)
{    
    _url_label.setAlignment(Qt::AlignCenter);

    _keyboard.hide();
    _info_sdp_text.hide();
    _info_sdp_close_button.hide();
    connect(&_close_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_cancel(bool)));
    connect(&_confirm_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_confirm(bool)));
    connect(&_keyboard_show_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_keyboard(bool)));    
    connect(&_info_sdp_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_sdpinfo(bool)));
    connect(&_info_sdp_close_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_sdpinfo_close(bool)));

    connect(&_keyboard, SIGNAL(click_confirm(QString)), this, SLOT(slot_from_keyboard(QString)));
}

play_rtsp_writer_dialog::~play_rtsp_writer_dialog()
{

}
void play_rtsp_writer_dialog::readyto_show()
{
    setGeometry(parent->x(), parent->y(), parent->width(), parent->height());
    _url_label.setGeometry(0, parent->height() / 2 - 25, parent->width(), 25);
    _info_text.setGeometry(parent->width() / 4, parent->height() / 2, parent->width() / 2, 30);
    _info_sdp_text.setGeometry(0,
                               0,
                               parent->width(),
                               parent->height() - 50);
    _info_sdp_text.hide();


    _close_button.setGeometry(parent->width() - 50 - 10,
                              parent->height() - 50 + 10,
                              50,
                              30);
    _info_sdp_close_button.setGeometry(parent->width() - 50 - 10,
                              parent->height() - 50 + 10,
                              50,
                              30);
    _info_sdp_close_button.hide();
    _confirm_button.setGeometry(parent->width() - 50 - 10 - 50 - 10,
                              parent->height() - 50 + 10,
                              50,
                              30);
    _keyboard_show_button.setGeometry(10,
                              parent->height() - 50 + 10,
                              50,
                              30);

    _info_sdp_button.setGeometry(10 + 50,
                              parent->height() - 50 + 10,
                              50,
                              30);

    _info_text.setPlainText(QString("rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov"));
}
void play_rtsp_writer_dialog::slot_click_sdpinfo_close(bool check)
{
    _url_label.show();
    _info_text.show();
    _close_button.show();

    _confirm_button.show();
    _keyboard_show_button.show();
    _info_sdp_button.show();
    _info_sdp_text.hide();
    _info_sdp_close_button.hide();
    if(_rtsp_schelduler)
    {
        disconnect(_rtsp_schelduler->refcallable(), SIGNAL(signal_report_response(QString)), this, SLOT(slot_recv_sdp(QString)));
        delete  _rtsp_schelduler;
        _rtsp_schelduler = NULL;
    }
}
void play_rtsp_writer_dialog::slot_click_sdpinfo(bool check)
{
    _info_sdp_text.clear();
    _url_label.hide();
    _info_text.hide();
    _close_button.hide();

    _confirm_button.hide();
    _keyboard_show_button.hide();
    _info_sdp_button.hide();


    if(_rtsp_schelduler)
    {
        disconnect(_rtsp_schelduler->refcallable(), SIGNAL(signal_report_response(QString)), this, SLOT(slot_recv_sdp(QString)));
        delete  _rtsp_schelduler;
        _rtsp_schelduler = NULL;
    }
    _rtsp_schelduler = new live5scheduler<rtsp_sdp_client>();
    _rtsp_schelduler->start(true,
                            live5rtspclient::report(),
                            _info_text.toPlainText().toLocal8Bit().data(),
                            nullptr,
                            nullptr);
    connect(_rtsp_schelduler->refcallable(), SIGNAL(signal_report_response(QString)), this, SLOT(slot_recv_sdp(QString)));
    _info_sdp_text.show();
    _info_sdp_close_button.show();
}
void play_rtsp_writer_dialog::slot_from_keyboard(QString str)
{
    _info_text.setPlainText(str);
}
void play_rtsp_writer_dialog::slot_recv_sdp(QString str)
{
    _info_sdp_text.setPlainText(str);
}
void play_rtsp_writer_dialog::slot_click_cancel(bool check)
{
    Q_UNUSED(check);
    setCompleteString("");
    reject();
}
void play_rtsp_writer_dialog::slot_click_confirm(bool check)
{
    Q_UNUSED(check);
    setCompleteString(_info_text.toPlainText());
    accept();
}
void play_rtsp_writer_dialog::slot_click_keyboard(bool check)
{
    Q_UNUSED(check);
    _keyboard.move(0, 0);
    _keyboard.setprefix(_info_text.toPlainText());
    _keyboard.show();
}
lmdialog *play_rtsp_writer_dialog::create_dialog(QWidget *p)
{
    return new play_rtsp_writer_dialog(p);
}
lmdialog *create_play_rtsp_writer_dialog(QWidget *p)
{
    return play_rtsp_writer_dialog::create_dialog(p);
}
