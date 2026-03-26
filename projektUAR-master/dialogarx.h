#ifndef DIALOGARX_H
#define DIALOGARX_H
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QMessageBox>

namespace Ui { class DialogARX; }

class DialogARX : public QDialog {
    Q_OBJECT
public:
    explicit DialogARX(QWidget *parent = nullptr);
    ~DialogARX();

    void setData(QString a, QString b, int k, double minU, double maxU, double minY, double maxY, double noise, bool limityWlaczone);
    QString getA() const;
    QString getB() const;

    int getK() const;

    double getMinU() const;
    double getMaxU() const;
    double getMinY() const;
    double getMaxY() const;
    double getNoise() const;

    bool getLimityWlaczone() const;

private slots:
    void accept();
private:
    Ui::DialogARX *ui;
};
#endif
