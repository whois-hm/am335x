#pragma once
#include "define.h"

class rtsp_sdp_client : public QObject, public live5rtspclient
{
    Q_OBJECT
public:
    rtsp_sdp_client (UsageEnvironment& env,
                     const report &notify,
                     char const* rtspurl,
                     char const *authid = nullptr,
                     char const *authpwd = nullptr):
        QObject(),
        live5rtspclient(env,
                        notify,
                        rtspurl,
                        authid,
                        authpwd) { }
    virtual ~rtsp_sdp_client(){}
private:
    virtual void response_option(int code, char* str)
    {

        if(code == 0)
        {
            /*get sdp*/
            sendcommand(rtsp_describe);
        }
        else
        {
            /*stop*/
            emit signal_report_response(QString (str));
        }

        delete [] str;
    }
    virtual void response_describe(int code, char* str)
    {
        /*stop*/
        emit signal_report_response(QString (str));
        delete [] str;
    }
signals:
    void signal_report_response(QString str);
};

class play_rtsp_writer_dialog : public lmdialog
{
    Q_OBJECT

    virtual void readyto_show();

    QPushButton _close_button;
    QPushButton _confirm_button;    
    QPushButton _keyboard_show_button;
    QPushButton _info_sdp_button;
    QPushButton _info_sdp_close_button;
    QLabel _url_label;
    QTextEdit _info_text;
    QTextEdit _info_sdp_text;
    virtualkeyboard _keyboard;
    live5scheduler<rtsp_sdp_client> *_rtsp_schelduler;
public:
     play_rtsp_writer_dialog(QWidget *parent = 0);
    virtual ~play_rtsp_writer_dialog();
    static lmdialog *create_dialog(QWidget *p);
private slots:
    void slot_click_cancel(bool check);
    void slot_click_confirm(bool check);
    void slot_click_keyboard(bool check);
    void slot_click_sdpinfo(bool check);
    void slot_from_keyboard(QString str);
    void slot_recv_sdp(QString str);
    void slot_click_sdpinfo_close(bool check);

};



