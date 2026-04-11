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
    void sendConf(int pakietID, const RegulatorPID &pid);
    void sendConf(int pakietID, const ModelARX &arx);
    void onReadyRead();
    void rozlacz();
private slots:
    void onConnect();
    void onDisc();
private:
    QTcpSocket m_socket;
signals:
   void polaczonoZSerwerem();
   void rozlaczonoZSerwerem();
   void otrzymanoPID(RegulatorPID pid);
   void otrzymanoARX(ModelARX arx);
};

#endif // KLIENTTCP_H
