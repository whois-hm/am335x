#include "custom_event.h"
extern void mainwindow_eventFilter(QEvent *e);
custom_filter*custom_filter::_instance = nullptr;

 const QEvent::Type custom_base_event::custom_base_event_id = static_cast<QEvent::Type>(QEvent::registerEventType(QEvent::User + 100));
custom_base_event::custom_base_event(void *ptr, custom_event_id id) :
    QEvent(custom_base_event::custom_base_event_id),
    _ptr(ptr),
    _id(id)
{
}
custom_base_event::~custom_base_event()
{ }



custom_filter::custom_filter() :
    QObject()
{
    QApplication::instance()->installEventFilter(this);
}
custom_filter::~custom_filter()
{

}
bool custom_filter::eventFilter(QObject *obj, QEvent *e)
{

    if(e->type() >= QEvent::User)
    {

        mainwindow_eventFilter(e);
        return true;
    }
    return QObject::eventFilter(obj, e);
}
void install_eventfilter()
{
    if(!custom_filter::_instance)
    {
        custom_filter::_instance = new custom_filter();
    }
}
void delete_eventfilter()
{
    if(custom_filter::_instance)
    {
        delete custom_filter::_instance;
        custom_filter::_instance = nullptr;
    }
}
