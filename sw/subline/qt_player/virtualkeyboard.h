#pragma once
#include "define.h"

enum btn_type
{
    btn_type_num1,
    btn_type_num2,
    btn_type_num3,
    btn_type_num4,
    btn_type_num5,
    btn_type_num6,
    btn_type_num7,
    btn_type_num8,
    btn_type_num9,
    btn_type_num0,
    btn_type_hyphen,
    btn_type_plus,
    btn_type_q,
    btn_type_w,
    btn_type_e,
    btn_type_r,
    btn_type_t,
    btn_type_y,
    btn_type_u,
    btn_type_i,
    btn_type_o,
    btn_type_p,
    btn_type_bracket_left,
    btn_type_bracket_right,
    btn_type_a,
    btn_type_s,
    btn_type_d,
    btn_type_f,
    btn_type_g,
    btn_type_h,
    btn_type_j,
    btn_type_k,
    btn_type_l,
    btn_type_semicolon,
    btn_type_quote,
    btn_type_z,
    btn_type_x,
    btn_type_c,
    btn_type_v,
    btn_type_b,
    btn_type_n,
    btn_type_m,
    btn_type_clamp_left,
    btn_type_clamp_right,
    btn_type_clamp_question,
    btn_type_space,
    btn_type_back,
    btn_type_shift,
    btn_type_confirm,
    btn_type_cancel,
    btn_type_max
};
class QPushButtonID : public QPushButton
{
    Q_OBJECT
    int id;
public:
    QPushButtonID(int id, QWidget *p) : QPushButton(p), id(id)
    {
        connect(this, SIGNAL(clicked(bool)), this , SLOT(clickid(bool)));
    }
private slots:
    void clickid(bool check)
    {
        Q_UNUSED(check);
        emit click_id(id);
    }
signals:
    void click_id(int id);
};
class virtualkeyboard : public QLabel
{
    Q_OBJECT
public:
    virtualkeyboard(const QRect &rect, QWidget *parent = 0);
    virtual ~ virtualkeyboard();
    void setprefix(const QString &str);

private:

    QTextEdit *_text_label;
    QPushButtonID *btns[btn_type_max];
    bool _bpress;

    void clear_keyboard();
    void update_keyboard(bool bpress);
    void hideEvent(QHideEvent *);
private slots:
    void slot_click_btn(int btn);
signals:
    void click_confirm(QString str);

};
