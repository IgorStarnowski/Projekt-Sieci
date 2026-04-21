#ifndef KLIENTTCP_H
#define KLIENTTCP_H

#include <QObject>
#include <QTcpSocket>
#include <vector>
#include <cstddef>
#include "UAR.h"

class klientTCP : public QObject
{
    Q_OBJECT
public:
    explicit klientTCP(QObject *parent = nullptr);
    void conToServ(const QString& ip, int port);
    void sendConf(TypPakietu pakietID, const ModelARX &arx);
    void onReadyRead();
    void rozlacz();
private slots:
    void onConnect();
    void onDisc();
private:
    QTcpSocket m_socket;
    quint32 oczekiwanyRozmiar = 0;
signals:
   void polaczonoZSerwerem();
   void rozlaczonoZSerwerem();
   void otrzymanoNowyPID(const RegulatorPID &pid);
    void otrzymanoNowyGen(const GeneratorWartosci &gen);
    void otrzymanoProbki(double t, double u, double w);
    void otrzymanoKomende(int akcja);
};

#endif // KLIENTTCP_H
