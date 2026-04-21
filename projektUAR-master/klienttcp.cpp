#include "klienttcp.h"
#include <iostream>
#include <QByteArray>
#include <QDateTime>
#include "UAR.h"

klientTCP::klientTCP(QObject *parent) : QObject(parent)
{
    connect(&m_socket, SIGNAL(connected()), this, SLOT(onConnect()));
    connect(&m_socket, SIGNAL(disconnected()), this, SLOT(onDisc()));
}
void klientTCP::conToServ(const QString& ip, int port){
    std::cout<<"Laczenie " << ip.toStdString() << ": " << port << std::endl;
    m_socket.connectToHost(ip, port);
    m_socket.waitForConnected(2000);
    connect(&m_socket, &QTcpSocket::readyRead, this, &klientTCP::onReadyRead);
}
void klientTCP::onConnect(){
    emit polaczonoZSerwerem();
}
void klientTCP::onDisc(){
    emit rozlaczonoZSerwerem();
}

void klientTCP::sendConf(TypPakietu pakietID, const ModelARX &arx){
   QByteArray dane;
   QDataStream out(&dane, QIODevice::WriteOnly);
   out << (quint32)0;
   out << (quint8)pakietID;
   qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
   out << timestamp;
   out << arx;
   out.device()->seek(0);
   out << (quint32)dane.size();
   m_socket.write(dane);
   m_socket.waitForBytesWritten();
   std::cout << "Wyslano konfig" << std::endl;
}
void klientTCP::onReadyRead()
{
    QDataStream in(&m_socket);
    in.setVersion(QDataStream::Qt_6_0);

    while (true) {
        if (oczekiwanyRozmiar == 0) {
            if (m_socket.bytesAvailable() < sizeof(quint32)) return;
            in >> oczekiwanyRozmiar;
        }

        if (m_socket.bytesAvailable() < (oczekiwanyRozmiar - sizeof(quint32))) return;
        quint8 pobraneID;
        qint64 czasWyslania;
        in >> pobraneID >> czasWyslania;
        TypPakietu typ = static_cast<TypPakietu>(pobraneID);
        switch (typ) {
            case KONF_PID: {
                RegulatorPID odebranyPID;
                in >> odebranyPID;
                emit otrzymanoNowyPID(odebranyPID);
                break;
            }
            case KONF_GEN: {
                GeneratorWartosci odebranyGen;
                in >> odebranyGen;
                emit otrzymanoNowyGen(odebranyGen);
                break;
            }
            case SYM_KONTROLKI: {
                qint32 akcja;
                in >> akcja;
                emit otrzymanoKomende(akcja);
                break;
            }
            case PROBKI_SYGNAL: {
                double t, u, w;
                in >> t >> u >> w;
                emit otrzymanoProbki(t, u, w);
                break;
            }
            default:
                qDebug() << "[KLIENT] Odrzucono nieznany pakiet o ID:" << pobraneID;
                break;
        }
        oczekiwanyRozmiar = 0;
    }
}
void klientTCP::rozlacz(){
    if(m_socket.state() == QAbstractSocket::ConnectedState){
        m_socket.disconnectFromHost();
    }
}
