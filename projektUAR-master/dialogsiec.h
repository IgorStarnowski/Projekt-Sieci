#ifndef DIALOGSIEC_H
#define DIALOGSIEC_H

#include <QDialog>

namespace Ui {
class Dialogsiec;
}

class Dialogsiec : public QDialog
{
    Q_OBJECT

public:
    explicit Dialogsiec(QWidget *parent = nullptr);
    ~Dialogsiec();
    bool czyKlient() const;
    QString getIP() const;
    int getPort() const;
private slots:
    void on_serwerRadio_toggled(bool checked);

private:
    Ui::Dialogsiec *ui;
    void accept() override;
};

#endif // DIALOGSIEC_H
