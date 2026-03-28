#include "klienttcp.h"
#include <iostream>
#include <QByteArray>
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
    std::cout << "Polaczono" << std::endl;
}
void klientTCP::onDisc(){
    std::cout << "Rozlaczono" << std::endl;
}
void klientTCP::sendConf(int pakietID, const RegulatorPID &pid){
   QByteArray dane;
   QDataStream out(&dane, QIODevice::WriteOnly);
   out << (qint8)pakietID;
   out << pid;
   m_socket.write(dane);
   m_socket.waitForBytesWritten();
   std::cout << "Wyslano konfig" << std::endl;
}
void klientTCP::sendConf(int pakietID, const ModelARX &arx){
   QByteArray dane;
   QDataStream out(&dane, QIODevice::WriteOnly);
   out << (qint8)pakietID;
   out << arx;
   m_socket.write(dane);
   m_socket.waitForBytesWritten();
   std::cout << "Wyslano konfig" << std::endl;
}
void klientTCP::onReadyRead() {
    QByteArray msg = m_socket.readAll();
    QDataStream in(&msg, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_6_0);

    qint8 pakietID;
    in >> pakietID;
    if (pakietID == 1) {
        RegulatorPID odebranyPID;
        in >> odebranyPID;

        std::cout << "\n<<< [KLIENT] DOSTALEM ODPOWIEDZ OD SERWERA! <<<" << std::endl;
        odebranyPID.wypiszKonf();
    }else if (pakietID == 2) {
        ModelARX odebranyARX;
        in >> odebranyARX;

        std::cout << "\n<<< [KLIENT] DOSTALEM ODPOWIEDZ OD SERWERA! <<<" << std::endl;
        odebranyARX.wypiszKonf();
    }
}
