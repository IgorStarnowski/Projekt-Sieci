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
    void sendConf(int pakietID, const RegulatorPID &pid);
    void sendConf(int pakietID, const ModelARX &arx);
private slots:
    void onNewCon();
    void onRedyRead();
    void onDisc();
private:
    QTcpServer m_server;
    QTcpSocket* m_clientSocket = nullptr;
    RegulatorPID m_lokalnyPID;
signals:
   void klientPodlaczony();
   void klientRozlaczony();
   void otrzymanoPID(RegulatorPID pid);
   void otrzymanoARX(ModelARX arx);
};

#endif // SERWERTCP_H
