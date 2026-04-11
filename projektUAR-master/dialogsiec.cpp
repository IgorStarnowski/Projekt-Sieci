#include "dialogsiec.h"
#include "ui_dialogsiec.h"
#include <QIntValidator>
#include <QRegularExpressionValidator>
#include <QMessageBox>
Dialogsiec::Dialogsiec(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialogsiec)
{
    ui->setupUi(this);
    this->setWindowTitle("Ustawienia sieciowe");
    ui->portEdit->setValidator(new QIntValidator(1,65535, this));
    QRegularExpression rxIP("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    QValidator *ipValidator = new QRegularExpressionValidator(rxIP, this);
    ui->adresEdit->setValidator(ipValidator);
    ui->adresEdit->setText("127.0.0.1");
    ui->portEdit->setText("12345");
    ui->klientRadio->setChecked(true);
}
bool Dialogsiec::czyKlient() const {return ui->klientRadio->isChecked();}
QString Dialogsiec::getIP() const {return ui->adresEdit->text();}
int Dialogsiec::getPort() const {return ui->portEdit->text().toInt();}

Dialogsiec::~Dialogsiec()
{
    delete ui;
}

void Dialogsiec::on_serwerRadio_toggled(bool checked)
{
    if(checked){
        ui->adresEdit->setEnabled(false);
        ui->adresEdit->setText("Nie dotyczy - nasłuchiwanie");
    } else {
        ui->adresEdit->setEnabled(true);
        ui->adresEdit->setText("127.0.0.1");
    }
}
void Dialogsiec::accept(){
    if(ui->klientRadio->isChecked()){
        QString wpisaneIp = ui->adresEdit->text();
        QRegularExpression rxIP("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
        QRegularExpressionMatch sprawdz = rxIP.match(wpisaneIp);
        if(!sprawdz.hasMatch()){
            ui->adresEdit->setStyleSheet("border: 2px solid red; background-color: #ffe6e6;");
            QMessageBox::warning(this, "Błąd konfiguracji", "Wprowadź poprawny adres IP\n(np. 127.0.0.1)");
            return;
        }
    }
    ui->adresEdit->setStyleSheet("");
    QDialog::accept();
}
