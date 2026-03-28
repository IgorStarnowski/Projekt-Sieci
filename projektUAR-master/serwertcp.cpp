#include "serwertcp.h"

SerwerTCP::SerwerTCP(QObject *parent) : QObject(parent), m_server(this)
{
    connect(&m_server, SIGNAL(newConnection()), this, SLOT(onNewCon()));
}
void SerwerTCP::startListening(int port){
    if(m_server.listen(QHostAddress::Any, port)){
        std::cout << "Serwer na porcie: " << port << std::endl;
    } else {
        std::cout << "Błąd startu" << std::endl;
    }
}
void SerwerTCP::onNewCon(){
    m_clientSocket = m_server.nextPendingConnection();
    std::cout << "Klient połączony" << std::endl;
    connect(m_clientSocket, SIGNAL(readyRead()),this, SLOT(onRedyRead()));
    connect(m_clientSocket, SIGNAL(disconnected()), this, SLOT(onDisc()));
}
void SerwerTCP::onRedyRead(){
    QByteArray dane = m_clientSocket->readAll();
    std::cout << "Otrzymano " << dane.size() << " bajtow" << std::endl;
    std::vector<std::byte> bufor(dane.size());
    memcpy(bufor.data(), dane.constData(), dane.size());
    m_lokalnyPID.deserializuj(bufor);
    std::cout << "Zaktualizowano PID" << std::endl;
}
void SerwerTCP::onDisc(){
    std::cout << "Klient sie rozlaczyl" << std::endl;
    m_clientSocket->deleteLater();
    m_clientSocket = nullptr;
}
