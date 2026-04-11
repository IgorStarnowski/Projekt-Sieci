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
    emit klientPodlaczony();
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
        emit otrzymanoPID(odebranyPID);
    }else if(pakietID == 2){
        ModelARX odebranyARX;
        in >> odebranyARX;
        emit otrzymanoARX(odebranyARX);
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
    emit klientRozlaczony();
    m_clientSocket->deleteLater();
    m_clientSocket = nullptr;
}
void SerwerTCP::zatrzymaj(){
    m_server.close();
    if(m_clientSocket){
        m_clientSocket->disconnectFromHost();
    }
}
