#include "mainwindow.h"
#include "custom_event.h"
#include <QFileDialog>


markpixel::markpixel(int idx) :
    pixel(),
    idx(idx) { }
markpixel::markpixel(markpixel &rhs) :
    pixel(dynamic_cast<pixel &>(rhs)),
    idx(rhs.idx) { }
markpixel::~markpixel() { }
markpixel *markpixel::clone()
{
   return new markpixel(*this);
}

markpcm::markpcm(int idx) :
    pcm(),
    idx(idx) { }
markpcm::markpcm(markpcm &rhs) :
    pcm(dynamic_cast<pcm &>(rhs)),
    idx(rhs.idx) { }
markpcm::~markpcm() { }
markpcm *markpcm::clone()
{
   return new markpcm(*this);
}

playback_manager::playback_manager(int idx,
                                   const avattr &attr,
                                   char const *url,
                                   unsigned connectiontime,
                                   char const *auth_id,
                                   char const *auth_pwd) :
    playback(attr, url, connectiontime, auth_id, auth_pwd),
    _url(url),
    _bpause(true),
    _idx(idx),
    _end(false),
    _reader_video(nullptr),
    _reader_audio(nullptr)
{
    if(!can())
    {
        return;
    }
    if(_inst->has(avattr::avattr_type_string(avattr_key::frame_video)))
    {

        _reader_video = QThread::create([this]()->void{
            busyscheduler sch;
            while(!_end)
            {

                markpixel mpix(_idx);
                int res = _inst->take(std::string(avattr_key::frame_video),
                                                dynamic_cast<pixel &>(mpix));
                if(res < 0)
                {
                    post_event(MainWindow::_instance, nullptr, custom_event_id_endof_pixel);
                    break;
                }
                if(res == 0)
                {

                    continue;
                }


                post_event(QApplication::instance(), mpix.clone(), custom_event_id_send_pixel);
            }
        });
        _reader_video->start();
    }
    if(_inst->has(avattr::avattr_type_string(avattr_key::frame_audio)))
    {
        _reader_audio = QThread::create([this]()->void{
            while(!_end)
            {
                markpcm mpcm(_idx);
                pcm_require require(dynamic_cast<pcm &>(mpcm), 100);

                int res = _inst->take(std::string(avattr_key::frame_audio), require);
                if(res < 0)
                {

                    break;
                }
                if(res >=0)
                {
                    continue;
                }

                post_event(MainWindow::_instance, require.first.clone(), custom_event_id_send_pcm);
            }
        });
        _reader_audio->start();
    }

}
playback_manager::~playback_manager()
{
    _end = true;
    if(_reader_video)
    {
        _reader_video->wait();
        delete  _reader_video;
    }
    if(_reader_audio)
    {
        _reader_audio->wait();
        delete _reader_audio;
    }

    _reader_audio = nullptr;
    _reader_video = nullptr;
}
bool playback_manager::can()
{
    return _inst ? true : false;
}
void playback_manager::play()
{
    if(_bpause)
    {
        _bpause = false;
        playback::play();
    }
}
bool playback_manager::ispause()
{
    return _bpause;
}
void playback_manager::pause()
{
    if(!_bpause)
    {
        _bpause = true;
        playback::pause();
    }
}
void playback_manager::resume()
{
    if(_bpause)
    {
        _bpause = false;
        playback::resume();
    }
}
bool playback_manager::match_url(char *url)
{
    if(url)
    {
        return _url == std::string(url);
    }
    return false;
}



MainWindow *MainWindow::_instance = nullptr;
QApplication *MainWindow::_app = nullptr;
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _hqvga_display_pannel(this),
    _duration_slider(Qt::Horizontal, this),
    _playlistwidget(this),
    _left_button("<<", this),
    _play_button("|>", this),
    _right_button(">>", this),
    _info_button("info", this),
    _list_del_button("del", this),
     _duration_label("", this),
    _playback_manager(nullptr)
{
    setGeometry(0,0,WINDOW_WIDTH, WINDOW_HEIGHT);


    _main_menu_bar = new QMenuBar(this);
    _main_menu_bar->setGeometry(0, 0, this->width()-50, 23);

    QMenu *media_menu = _main_menu_bar->addMenu(TEXT_MEDIA);
    QMenu *play_menu = media_menu->addMenu(TEXT_PLAY);

    play_menu->addAction(TEXT_FILE);
    play_menu->addAction(TEXT_RTSP);
    media_menu->addAction(TEXT_EXIT);

    QMenu *help_menu = _main_menu_bar->addMenu(TEXT_HELP);
    help_menu->addAction(TEXT_ABOUTE);
    help_menu->addAction(TEXT_VERSION);
    help_menu->addAction(TEXT_WRITER);

    connect(_main_menu_bar, SIGNAL(triggered(QAction*)), this, SLOT(slot_trigger_action(QAction *)));

    this->setMenuBar(_main_menu_bar);
    /*
        input action dialog /media/play/file
     */
    extern lmdialog *create_play_file_list_dialog(QWidget *p);
    _menu_bar_actor.append(QPair<QString, lmdialog *>(QString(TEXT_FILE), create_play_file_list_dialog(this)));
    /*
        input action dialog /media/play/rtsp
     */
    extern lmdialog *create_play_rtsp_writer_dialog(QWidget *p);
    _menu_bar_actor.append(QPair<QString, lmdialog *>(QString(TEXT_RTSP), create_play_rtsp_writer_dialog(this)));

    /*
        input action dialog /media/exit
     */
    extern lmdialog *create_exit_dialog(QWidget *p);
    _menu_bar_actor.append(QPair<QString, lmdialog *>(QString(TEXT_EXIT), create_exit_dialog(this)));

    _hqvga_display_pannel.setGeometry(0, _main_menu_bar->y() + _main_menu_bar->height(), 240, 160);


    _duration_slider.setGeometry(0, _hqvga_display_pannel.y() + _hqvga_display_pannel.height() + 5, _hqvga_display_pannel.width(), 20);


    _playlistwidget.setGeometry(_hqvga_display_pannel.width(),
                                _main_menu_bar->y() + _main_menu_bar->height(),
                                WINDOW_WIDTH - _hqvga_display_pannel.width(),
                                _hqvga_display_pannel.height() + _duration_slider.height());


    _play_button.setGeometry(0 + 30, _playlistwidget.y() + _playlistwidget.height() + 5, 30, 30);
    _left_button.setGeometry(_play_button.x()-30, _play_button.y(), 30, 30);
    _right_button.setGeometry(_play_button.x()+30, _play_button.y(), 30, 30);

    _info_button.setGeometry(_playlistwidget.x(), _playlistwidget.y() + _playlistwidget.height() + 5, _playlistwidget.width()/2, 30);
    _list_del_button.setGeometry(_playlistwidget.x() + (_playlistwidget.width()/2), _playlistwidget.y() + _playlistwidget.height() + 5, _playlistwidget.width()/2, 30);

    connect(&_list_del_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_playlist_del(bool)));
    connect(&_info_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_info(bool)));
    connect(&_right_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_right(bool)));
    connect(&_left_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_left(bool)));
    connect(&_play_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_play(bool)));
    connect(&_playlistwidget, SIGNAL(itemPressed(QListWidgetItem *)), this, SLOT(slot_doubleclick_list(QListWidgetItem *)));

    int duration_lab_x = _right_button.x() + _right_button.width() + 10;
    _duration_label.setGeometry(duration_lab_x,
                                _playlistwidget.y() + _playlistwidget.height(),
                                (_duration_slider.x() + _duration_slider.width()) - duration_lab_x,
                                WINDOW_HEIGHT - (_playlistwidget.y() + _playlistwidget.height()) );
    QFont f;
    f.setPointSize(7);
    _duration_label.setFont(f);
    cleanup_Playing();
}
MainWindow::~MainWindow()
{
    if(_playback_manager)
    {
        delete _playback_manager;
        _playback_manager = nullptr;
    }
}
void MainWindow::slot_doubleclick_list(QListWidgetItem *item)
{

    if(_playback_manager)
    {
        delete _playback_manager;
        _playback_manager = nullptr;
    }
    update_display_pannel(QColor(0,0,0));
    _play_button.setText("|>");
    _duration_label.setText("");
    _duration_label.hide();
    _duration_slider.setValue(0);



    avattr attr;
    attr.set(avattr_key::frame_video, avattr_key::frame_video, 0, 0.0);
    attr.set(avattr_key::width, avattr_key::width, _hqvga_display_pannel.width(), _hqvga_display_pannel.width());
    attr.set(avattr_key::height, avattr_key::height, _hqvga_display_pannel.height(), _hqvga_display_pannel.height());
    attr.set(avattr_key::pixel_format, avattr_key::pixel_format, AV_PIX_FMT_RGB24, 0);
//AV_PIX_FMT_YUV420P
    _playback_manager = new playback_manager(0,
                                             attr,
                                             item->text().toLocal8Bit().data(),
                                             0);
    if(!_playback_manager->can())
    {
        delete _playback_manager;
        _playback_manager = nullptr;
        QMessageBox box(QMessageBox::NoIcon,
                        QString("dialog"),
                        QString("can't open playback manager"),
                        QMessageBox::Ok,
                        this);
        box.setModal(true);
        box.exec();
    }
    duration_div last_duration = _playback_manager->duration();
    update_duration(triple_int(0,0,0),
                    triple_int(0,0,0),
                    triple_int(std::get<0>(last_duration),
                               std::get<1>(last_duration),
                               std::get<2>(last_duration)));
    unsigned sec = (std::get<0>(last_duration)  * 60 * 60) + std::get<1>(last_duration) * 60 +  std::get<2>(last_duration);
    _duration_slider.setRange(0, sec);
    _duration_label.show();
}
void MainWindow::slot_click_info(bool check)
{
    Q_UNUSED(check);
}
void MainWindow::slot_click_right(bool check)
{    
    Q_UNUSED(check);
    if(_playback_manager)
    {
        _playback_manager->seek(5.0);
    }
}
void MainWindow::slot_click_play(bool check)
{
    Q_UNUSED(check);
    if(_playback_manager)
    {
        if(_playback_manager->ispause())
        {
            _play_button.setText("||");
            _playback_manager->resume();
        }
        else
        {
            _play_button.setText("|>");
            _playback_manager->pause();
        }
    }
}
void MainWindow::slot_click_left(bool check)
{
    Q_UNUSED(check);
    if(_playback_manager)
    {
        _playback_manager->seek(-5.0);
    }
}
void MainWindow::slot_click_playlist_del(bool check)
{
Q_UNUSED(check);
    QListWidgetItem *item = nullptr;
    item = _playlistwidget.takeItem(_playlistwidget.currentRow());
    _playlistwidget.update();
    if(item)
    {
        if(_playback_manager &&
           _playback_manager->match_url(item->text().toLocal8Bit().data()))
        {
            delete _playback_manager;
            _playback_manager = nullptr;
        }
        update_display_pannel(QColor(0,0,0));
        _play_button.setText("|>");
    }
    if(_playlistwidget.count() <= 0)
    {
        cleanup_Playing();
    }
}
void MainWindow::cleanup_Playing()
{
     update_display_pannel(QColor(0,0,0));
     _playlistwidget.clear();

    _hqvga_display_pannel.hide();
    _duration_slider.hide();
    _playlistwidget.hide();
    _left_button.hide();
    _play_button.hide();
    _right_button.hide();
    _info_button.hide();
    _list_del_button.hide();
    _duration_label.setText("");
    _duration_label.hide();
}
void MainWindow::update_duration(const triple_int &start,
                     const triple_int &cur, const triple_int &last)
{
    _duration_label.setText(QString("start : %1:%2:%3\ncur : %4:%5:%6\nlast : %7:%8:%9")
                            .arg(std::get<0>(start)).arg(std::get<1>(start)).arg(std::get<2>(start))
                            .arg(std::get<0>(cur)).arg(std::get<1>(cur)).arg(std::get<2>(cur))
                            .arg(std::get<0>(last)).arg(std::get<1>(last)).arg(std::get<2>(last)));
}
void MainWindow::update_duration(double cur)
{
    QStringList dur = _duration_label.text().split("\n");
    int h, m,s;
    unsigned ms_total = cur * 1000;
    s = ms_total / 1000;
    m = s / 60;
    h = m / 60;
    s = s % 60;
    m = m % 60;


    dur[1] = QString("cur : %1:%2:%3").arg(h).arg(m).arg(s);
    _duration_label.setText(dur[0] + QString("\n") + dur[1] + QString("\n") + dur[2]);
    _duration_slider.setValue(cur);
}
void MainWindow::update_display_pannel(const QColor &colormode)
{
    QPixmap pix(_hqvga_display_pannel.width(), _hqvga_display_pannel.height());
    pix.fill(colormode);
    _hqvga_display_pannel.setPixmap(pix);
}
void MainWindow::update_display_pannel(const QPixmap &pixmap)
{
    _hqvga_display_pannel.setPixmap(pixmap);
}
void MainWindow::ready_to_playing(const QString &target)
{
    _playlistwidget.addItem(target);
    if(!_hqvga_display_pannel.isVisible())_hqvga_display_pannel.show();
    if(!_duration_slider.isVisible())_duration_slider.show();
    if(!_playlistwidget.isVisible())_playlistwidget.show();
    if(!_left_button.isVisible())_left_button.show();
    if(!_play_button.isVisible())_play_button.show();
    if(!_right_button.isVisible())_right_button.show();
    if(!_info_button.isVisible())_info_button.show();
    if(!_list_del_button.isVisible())_list_del_button.show();
}
void MainWindow::slot_trigger_action(QAction *act)
{
    for(int i = 0; i < _menu_bar_actor.size(); i++)
    {

        if(_menu_bar_actor.at(i).first == act->text())
        {
            QString res = _menu_bar_actor.at(i).second->dialog_exec();
            /*
             * * do somthing
             */
            if(res.isEmpty())
            {
                return;
            }
            if(_menu_bar_actor.at(i).first == QString(TEXT_FILE) ||
                    _menu_bar_actor.at(i).first == QString(TEXT_RTSP))
            {
                ready_to_playing(res);
            }
            if(_menu_bar_actor.at(i).first == QString(TEXT_EXIT) )
            {
                qApp->exit();
            }
            break;
        }
    }

}
void MainWindow::eventfilter_handler(QEvent *e)
{
    custom_base_event *evt = (custom_base_event *)e;
    if(custom_event_id_endof_pixel == evt->_id)
    {
        update_display_pannel(QColor(0,0,0));
        if(_playback_manager)
        {
            delete _playback_manager;
            _playback_manager = nullptr;
        }
        update_display_pannel(QColor(0,0,0));
        _play_button.setText("|>");
        _duration_label.setText("");
        _duration_label.hide();
        _duration_slider.setValue(0);
    }
    if(evt->_id == custom_event_id_send_pixel)
    {

        markpixel *pix = (markpixel *)evt->_ptr;
        QImage im(pix->read(), pix->width(), pix->height(), QImage::Format_RGB888);
        QPixmap map;
        if(map.convertFromImage(im))
        {
            update_display_pannel(map);
        }
        else
        {

            update_display_pannel(QColor(0,0,0));
        }
        update_duration(pix->getpts());
        delete pix;
        evt->_ptr = nullptr;
    }

}
void MainWindow::startApplication(int argc, char *argv[])
{
    extern void install_eventfilter();
    if(!_app &&
            !_instance)
    {
        _app = new QApplication(argc, argv);
        install_eventfilter();
        _instance = new MainWindow();
        _instance->show();
        _app->exec();
    }
}
void MainWindow::endApplication()
{
    if(_app &&
            _instance)
    {
        delete _instance;
        delete _app;
    }
}
void MainWindow::eventFilter(QEvent *e)
{
    if(_app &&
            _instance)
    {

        _instance->eventfilter_handler(e);
    }
}
void mainwindow_eventFilter(QEvent *e)
{

    MainWindow::eventFilter(e);
}
void startApplication(int argc, char *argv[])
{
    MainWindow::startApplication(argc, argv);
}
void endApplication()
{
    extern void delete_eventfilter();
    void delete_eventfilter();
    MainWindow::endApplication();
}

