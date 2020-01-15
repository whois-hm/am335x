#pragma once
#include "define.h"

class markpixel : public pixel
{
private:
    int idx;
public:
   markpixel(int idx);
   markpixel(markpixel &rhs);
   virtual ~markpixel();
   markpixel *clone();
};
class markpcm : public pcm
{
private:
    int idx;
public:
   markpcm(int idx);
   markpcm(markpcm &rhs);
   virtual ~markpcm();
   markpcm *clone();
};
class playback_manager : public playback
{
private:
    std::string _url;
    bool _bpause;
    int _idx;
    bool _end;
    QThread *_reader_video;
    QThread *_reader_audio;

public:

    playback_manager(int idx,
                     const avattr &attr,
                     char const *url,
                     unsigned connectiontime,
                     char const *auth_id = nullptr,
                     char const *auth_pwd = nullptr);
    virtual ~playback_manager();
    bool can();
    void play();
    void pause();
    void resume();
    bool match_url(char *url);
    bool ispause();
};


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
     MainWindow(QWidget *parent = 0);
    ~MainWindow();
     static void startApplication(int argc, char *argv[]);
     static void endApplication();
     static void eventFilter(QEvent *e);
     static MainWindow *_instance;
     static QApplication *_app;
private:

     QMenuBar *_main_menu_bar;
     QList<QPair<QString, lmdialog *> > _menu_bar_actor;
     QLabel _hqvga_display_pannel;
     QSlider _duration_slider;
     QListWidget _playlistwidget;
     QPushButton _left_button;
     QPushButton _play_button;
     QPushButton _right_button;
     QPushButton _info_button;
     QPushButton _list_del_button;
     QLabel _duration_label;
     playback_manager *_playback_manager;

     void update_display_pannel(const QColor &colormode);
     void update_display_pannel(const QPixmap &pixmap);
     void update_duration(const triple_int &start,
                          const triple_int &cur, const triple_int &last);
     void update_duration(double cur);

     void ready_to_playing(const QString &target);
     void cleanup_Playing();
     void eventfilter_handler(QEvent *e);
private slots:
    void slot_trigger_action(QAction *act);
    void slot_click_playlist_del(bool check);
    void slot_click_info(bool check);
    void slot_click_right(bool check);
    void slot_click_play(bool check);
    void slot_click_left(bool check);
    void slot_doubleclick_list(QListWidgetItem *item);
};
