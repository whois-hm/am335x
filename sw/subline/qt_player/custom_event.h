#pragma once
#include "define.h"
enum custom_event_id
{
    custom_event_id_send_pixel,
    custom_event_id_send_pcm,
    custom_event_id_endof_pixel,
    
};

class custom_base_event : public QEvent
{
public:
    custom_base_event(void *ptr, custom_event_id id);
    virtual ~custom_base_event();
    static const QEvent::Type custom_base_event_id;
    void *_ptr;
    custom_event_id _id;
};



class custom_filter : public QObject
{
public:
    static custom_filter*_instance;
    custom_filter();
    virtual ~custom_filter();
    virtual bool eventFilter(QObject *obj, QEvent *e);
};

class post_event
{
private:
     custom_base_event *_evt;
     QObject *_receiver;
public:
    post_event(QObject *re, custom_base_event &&evt) :
        _evt(new custom_base_event (evt._ptr, evt._id)),
    _receiver(re){QCoreApplication::postEvent(_receiver, _evt);}
    post_event(QObject *re, void *ptr, custom_event_id id) :
        _evt(new custom_base_event(ptr, id)),
    _receiver(re){QCoreApplication::postEvent(_receiver, _evt);}
    virtual ~post_event()
    {




    }
};
