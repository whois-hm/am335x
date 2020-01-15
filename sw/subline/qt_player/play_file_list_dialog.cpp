#include "play_file_list_dialog.h"

play_file_list_dialog::play_file_list_dialog(QWidget *parent) :
    lmdialog(parent),
    _listwidget(this),
    _close_button("close", this),
    _confirm_button("confirm", this),
    _info_button("info", this),
    _info_text(this),
    _info_text_close_button("close", this)
{    
    _filters.append(".mp4");
    _filters.append(".avi");



    connect(&_close_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_cancel(bool)));
    connect(&_confirm_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_confirm(bool)));
    connect(&_info_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_info(bool)));
    connect(&_info_text_close_button, SIGNAL(clicked(bool)), this, SLOT(slot_click_info_text_cancel(bool)));
    connect(&_listwidget, SIGNAL(currentRowChanged(int)), this, SLOT(slot_click_listwidget(int)));
}

play_file_list_dialog::~play_file_list_dialog()
{

}
void play_file_list_dialog::slot_click_listwidget(int row)
{
    if(row >= 0)
    {
        _confirm_button.show();
        _info_button.show();
    }
}
void play_file_list_dialog::slot_click_cancel(bool check)
{
    setCompleteString("");
    reject();
}
void play_file_list_dialog::slot_click_confirm(bool check)
{
    QListWidgetItem *item = nullptr;
    item = _listwidget.item(_listwidget.currentRow());
    if(!item)
    {
        return;
    }
    setCompleteString(item->text());
    accept();
}
void play_file_list_dialog::slot_click_info(bool check)
{
    QListWidgetItem *item = nullptr;
    item = _listwidget.item(_listwidget.currentRow());
    if(!item)
    {
        return;
    }

    mediacontainer container(item->text().toLocal8Bit().data());
    std::string info_str;
    container.dump_meta(&info_str);

    _info_text.clear();
    _info_text.setPlainText(QString(info_str.c_str()));

    _listwidget.hide();
    _close_button.hide();
    _confirm_button.hide();
    _info_button.hide();        
    _info_text.show();
    _info_text_close_button.show();
}
void play_file_list_dialog::slot_click_info_text_cancel(bool check)
{
    Q_UNUSED(check);
    _listwidget.show();
    _close_button.show();
    _confirm_button.show();
    _info_button.show();

    _info_text.hide();
    _info_text_close_button.hide();
}
void play_file_list_dialog::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    _info_text.hide();


    _close_button.show();
    _confirm_button.hide();
    _info_button.hide();
    _info_text_close_button.hide();

    _listwidget.addItems(_filelist);
    _listwidget.show();
}
void play_file_list_dialog::readyto_show()
{
    unsigned total = 0;
    unsigned find_len = 0;
    QString find_last = "";

    {
        QDirIterator iter("/home", QDirIterator::Subdirectories);
        while(iter.hasNext())
        {
            iter.next();
            total++;
        }
    }

    _filelist.clear();
    _listwidget.clear();
    _info_text.clear();
    setCompleteString("");
    setGeometry(parent->x(),
                parent->y(),
                parent->width(),
                parent->height());
    _listwidget.setGeometry(0,
                            0,
                            parent->width(),
                            parent->height() - 50);
    _info_text.setGeometry(0,
                           0,
                           parent->width(),
                           parent->height() - 50);

    _close_button.setGeometry(parent->width() - 50 - 10,
                              parent->height() - 50 + 10,
                              50,
                              30);
    _confirm_button.setGeometry(parent->width() - 50 - 10 - 50 - 10,
                              parent->height() - 50 + 10,
                              50,
                              30);
    _info_button.setGeometry(10,
                              parent->height() - 50 + 10,
                              50,
                              30);
    _info_text_close_button.setGeometry(parent->width() - 50 - 10,
                                        parent->height() - 50 + 10,
                                        50,
                                        30);


    QDirIterator iter("/home", QDirIterator::Subdirectories);
    QProgressDialog sub_dialog( this);
    sub_dialog.setGeometry(parent->x() + 10,
                           parent->y() + (parent->height() / 4),
                           parent->width() - 20,
                           parent->height() / 2);
    sub_dialog.setWindowModality(Qt::WindowModal);
    sub_dialog.setMinimumDuration(1);
    sub_dialog.setRange(0, total);
    sub_dialog.setLabelText(QString("found file : %1\n(%2)").arg(find_len).arg(find_last));
    sub_dialog.setFixedSize(parent->width() - 20,
                            parent->height() / 2);


    for (int i = 0; i < total && iter.hasNext(); i++)
    {
        sub_dialog.setValue(i);
        for(int t = 0; t < _filters.size(); t++)
        {
            if(iter.fileName().contains(_filters.at(t), Qt::CaseSensitive))
            {
                find_len++;
                find_last = iter.filePath();
                _filelist.append(find_last);
                sub_dialog.setLabelText(QString("found file : %1\n(%2)").arg(find_len).arg(find_last));
                break;
            }
        }

        if(sub_dialog.wasCanceled())
        {
            break;
        }
        iter.next();
    }

}

lmdialog *play_file_list_dialog::create_dialog(QWidget *p)
{
    return new play_file_list_dialog(p);
}
lmdialog *create_play_file_list_dialog(QWidget *p)
{
    return play_file_list_dialog::create_dialog(p);
}
