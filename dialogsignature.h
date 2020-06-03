#ifndef DIALOGSIGNATURE_H
#define DIALOGSIGNATURE_H

#include <QDialog>

namespace Ui {
class DialogSignature;
}

class DialogSignature : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSignature(QWidget *parent = nullptr);
    ~DialogSignature();

private:
    Ui::DialogSignature *ui;
};

#endif // DIALOGSIGNATURE_H
