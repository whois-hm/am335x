#include "virtualkeyboard.h"

struct
{
   btn_type t;
    char const *nor;
    char const *press;
}static keytable[btn_type_max] =
{
{    btn_type_num1, "1", "!"},
{    btn_type_num2, "2", "@"},
{    btn_type_num3, "3", "#"},
{    btn_type_num4, "4", "$"},
{    btn_type_num5, "5", "%"},
{    btn_type_num6, "6", "^"},
{    btn_type_num7, "7", "&"},
{    btn_type_num8, "8", "*"},
{    btn_type_num9, "9", "("},
{    btn_type_num0, "0", ")"},
{    btn_type_hyphen, "-", "_"},
{    btn_type_plus, "+", "="},
{    btn_type_q, "q", "Q"},
{    btn_type_w, "w", "W"},
{    btn_type_e, "e", "E"},
{    btn_type_r, "r", "R"},
{    btn_type_t, "t", "T"},
{    btn_type_y, "y", "Y"},
{    btn_type_u, "u", "U"},
{    btn_type_i, "i", "I"},
{    btn_type_o, "o", "O"},
{    btn_type_p, "p", "P"},
{    btn_type_bracket_left, "{", "["},
{    btn_type_bracket_right, "}", "]"},
{    btn_type_a, "a", "A"},
{    btn_type_s, "s", "S"},
{    btn_type_d, "d", "D"},
{    btn_type_f, "f", "F"},
{    btn_type_g, "g", "G"},
{    btn_type_h, "h", "H"},
{    btn_type_j, "j", "J"},
{    btn_type_k, "k", "K"},
{    btn_type_l, "l", "L"},
{    btn_type_semicolon, ";", ":"},
{    btn_type_quote, "'", "\""},
{    btn_type_z, "z", "Z"},
{    btn_type_x, "x", "X"},
{    btn_type_c, "c", "C"},
{    btn_type_v, "v", "V"},
{    btn_type_b, "b", "B"},
{    btn_type_n, "n", "N"},
{    btn_type_m, "m", "M"},
{    btn_type_clamp_left, ",", "<"},
{    btn_type_clamp_right, ".", ">"},
{    btn_type_clamp_question, "/", "?"},
{    btn_type_space, " ", " "},
{    btn_type_back, "<-", "<-"},
{    btn_type_shift, "sh", "sh"},
{    btn_type_confirm, "ok", "ok"},
{    btn_type_cancel, "can", "can"}
};



virtualkeyboard::virtualkeyboard(const QRect &rect, QWidget *parent) :
    QLabel(parent),
    _text_label(nullptr),
    _bpress(false)
{
    resize(rect.width(), rect.height());
    int text_label_x = rect.x();
    int text_label_y = rect.y();
    int text_label_w = rect.width();
    int text_label_h = rect.height() / 10;/*10:1*/

    int button_w = rect.width() / 10;/*1 line 10 button */
    int button_y = (rect.height() - text_label_h) / 5; /*5 line */

    int button_current_y = text_label_x + text_label_h;
    int idx = 0;

    _text_label = new QTextEdit(this);
    _text_label->setGeometry(text_label_x, text_label_y, text_label_w, text_label_h);
    for(int y= 0; y < 5; y++)
    {
        for(int x = 0; x < 10; x++)
        {

            btns[idx] = new QPushButtonID(keytable[idx].t, this);
            btns[idx]->setGeometry((x * button_w)+rect.x(), button_current_y, button_w, button_y);
            btns[idx]->setText(keytable[idx].nor);
            connect(btns[idx], SIGNAL(click_id(int)), this, SLOT(slot_click_btn(int)));
            idx++;
        }
        button_current_y+=button_y;
    }
    clear_keyboard();
}
virtualkeyboard::~virtualkeyboard()
{

}
void virtualkeyboard::setprefix(const QString &str)
{
    _text_label->setPlainText(str);
}
void virtualkeyboard::hideEvent(QHideEvent *)
{
    clear_keyboard();
}
void virtualkeyboard::clear_keyboard()
{
    _text_label->clear();
    _bpress = false;
    update_keyboard(_bpress);

}
void virtualkeyboard::update_keyboard(bool bpress)
{
    _bpress = bpress;
    for(int  i = 0; i < btn_type_max; i++)
    {
        btns[i]->setText(_bpress ? keytable[i].press : keytable[i].nor);
    }
}
void virtualkeyboard::slot_click_btn(int btn)
{
    if(btn >= btn_type_num1 && btn <= btn_type_space)
    {
        _text_label->setPlainText(_text_label->toPlainText() + QString(_bpress ? keytable[btn].press : keytable[btn].nor));
    }
    else if(btn_type_back == btn)
    {
        QString curstr = _text_label->toPlainText();
        if(!curstr.isEmpty())
        {
           curstr.remove(curstr.size()-1, 1);
           _text_label->setPlainText(curstr);
        }
    }
    else if(btn_type_shift == btn)
    {
        update_keyboard(!_bpress);
    }
    else if(btn_type_confirm == btn)
    {
        emit click_confirm(_text_label->toPlainText());
        hide();
    }
    else if(btn_type_cancel == btn)
    {
        hide();
    }

}
