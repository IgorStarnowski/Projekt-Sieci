#ifndef SERWERTCP_H
#define SERWERTCP_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <iostream>
#include <vector>
#include "UAR.h"

class SerwerTCP : public QObject
{
    Q_OBJECT
public:
    explicit SerwerTCP(QObject *parent = nullptr);
    void startListening(int port);
    void zatrzymaj();
    void sendConf(TypPakietu pakietID, const RegulatorPID &pid);
    void sendConf(TypPakietu pakietID, const GeneratorWartosci &gen);
    QString pobierzIP();
private slots:
    void onNewCon();
    void onRedyRead();
    void onDisc();
private:
    QTcpServer m_server;
    QTcpSocket* m_clientSocket = nullptr;
    RegulatorPID m_lokalnyPID;
    quint32 oczekiwanyRozmiar = 0;
signals:
   void klientPodlaczony();
   void klientRozlaczony();
   void otrzymanoNowyARX(const ModelARX &arx);
   void otrzymanoProbki(double t, double y);
};

#endif // SERWERTCP_H
