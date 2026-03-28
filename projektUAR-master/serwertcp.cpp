#include "serwertcp.h"

SerwerTCP::SerwerTCP(QObject *parent) : QObject(parent), m_server(this)
{
    connect(&m_server, SIGNAL(newConnection()), this, SLOT(onNewCon()));
}
void SerwerTCP::startListening(int port){
    if(m_server.listen(QHostAddress::Any, port)){
        std::cout << "Serwer na porcie: " << port << std::endl;
    } else {
        std::cout << "Blad startu" << std::endl;
    }
}
void SerwerTCP::onNewCon(){
    m_clientSocket = m_server.nextPendingConnection();
    std::cout << "Klient polaczony" << std::endl;
    connect(m_clientSocket, SIGNAL(readyRead()),this, SLOT(onRedyRead()));
    connect(m_clientSocket, SIGNAL(disconnected()), this, SLOT(onDisc()));
}
void SerwerTCP::onRedyRead(){
    QByteArray dane = m_clientSocket->readAll();
    QDataStream in(&dane, QIODevice::ReadOnly);
    qint8 pakietID;
    in >> pakietID;
    if (pakietID == 1) {
        RegulatorPID odebranyPID;
        in >> odebranyPID;
        std::cout << "Odebrano konfig PID" << std::endl;
        odebranyPID.wypiszKonf();
        std::cout << "[SERWER] Odsylam testowy ARX do klienta..." << std::endl;
        ModelARX odpARX;
        odpARX.setParams({9.9, 8.8}, {7.7, 6.6}, 5);
        this->sendConf(2, odpARX);
    }else if(pakietID == 2){
        ModelARX odebranyARX;
        in >> odebranyARX;
        std::cout << "Odebrano konfig ARX" << std::endl;
        odebranyARX.wypiszKonf();
        std::cout << "[SERWER] Odsylam testowy PID do klienta..." << std::endl;
        RegulatorPID odpPID;
        odpPID.setNastawy(3.14, 9.99, 0.55, LiczCalk::Wew);
        this->sendConf(1, odpPID);
    }
}
void SerwerTCP::sendConf(int pakietID, const RegulatorPID &pid){
    if(!m_clientSocket || m_clientSocket->state() != QAbstractSocket::ConnectedState){
        std::cout << "[SERWER] Brak klienta" << std::endl;
    }
       QByteArray dane;
       QDataStream out(&dane, QIODevice::WriteOnly);
       out << (qint8)pakietID;
       out << pid;
       m_clientSocket->write(dane);
       m_clientSocket->waitForBytesWritten();
       std::cout << "Wyslano konfig" << std::endl;
    }
void SerwerTCP::sendConf(int pakietID, const ModelARX &arx){
    if(!m_clientSocket || m_clientSocket->state() != QAbstractSocket::ConnectedState){
        std::cout << "[SERWER] Brak klienta" << std::endl;
    }
    QByteArray dane;
    QDataStream out(&dane, QIODevice::WriteOnly);
    out << (qint8)pakietID;
    out << arx;
    m_clientSocket->write(dane);
    m_clientSocket->waitForBytesWritten();
    std::cout << "Wyslano konfig" << std::endl;
}

void SerwerTCP::onDisc(){
    std::cout << "Klient sie rozlaczyl" << std::endl;
    m_clientSocket->deleteLater();
    m_clientSocket = nullptr;
}
