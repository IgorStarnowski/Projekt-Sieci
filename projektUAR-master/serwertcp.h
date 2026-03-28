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
private slots:
    void onNewCon();
    void onRedyRead();
    void onDisc();
private:
    QTcpServer m_server;
    QTcpSocket* m_clientSocket = nullptr;
    RegulatorPID m_lokalnyPID;
};

#endif // SERWERTCP_H
