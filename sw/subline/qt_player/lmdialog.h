#pragma once
#include <QDialog>


class lmdialog : public QDialog
{
protected:
    QString val;
    QWidget *parent;
    lmdialog(QWidget *p) : QDialog(p), parent(p)
    {
        setWindowTitle("dialog");
        setModal(true);
    }
    virtual void readyto_show() { }
    virtual void setCompleteString(const QString &str){val = str;}
public:
    virtual ~lmdialog() { }
    QString dialog_exec()
    {
        readyto_show();
        exec();
        return val;
    }

};
