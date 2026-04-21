#include "serwertcp.h"
#include "QMessageBox"
#include <QDateTime>

SerwerTCP::SerwerTCP(QObject *parent) : QObject(parent), m_server(this)
{
    connect(&m_server, SIGNAL(newConnection()), this, SLOT(onNewCon()));
}
void SerwerTCP::startListening(int port){
    if(m_server.listen(QHostAddress::Any, port)){
            QMessageBox::information(nullptr, "Sukces", "Serwer NASŁUCHUJE na porcie: " + QString::number(port));
        } else {
            QMessageBox::critical(nullptr, "Błąd Serwera", "Nie udało się otworzyć portu!\nPowód: " + m_server.errorString());
        }
}
void SerwerTCP::onNewCon(){
    m_clientSocket = m_server.nextPendingConnection();
    emit klientPodlaczony();
    connect(m_clientSocket, SIGNAL(readyRead()),this, SLOT(onRedyRead()));
    connect(m_clientSocket, SIGNAL(disconnected()), this, SLOT(onDisc()));
}
void SerwerTCP::onRedyRead(){
    QDataStream in(m_clientSocket);
    in.setVersion(QDataStream::Qt_6_0);

    while (true) {
        if (oczekiwanyRozmiar == 0) {
            if (m_clientSocket->bytesAvailable() < sizeof(quint32)) return;
            in >> oczekiwanyRozmiar;
        }
        if (m_clientSocket->bytesAvailable() < (oczekiwanyRozmiar - sizeof(quint32))) return;
        quint8 pobraneID;
        qint64 czasWyslania;
        in >> pobraneID >> czasWyslania;
        TypPakietu typ = static_cast<TypPakietu>(pobraneID);
        switch (typ) {
            case KONF_ARX: {
                ModelARX odebranyARX;
                in >> odebranyARX;
                emit otrzymanoNowyARX(odebranyARX);
                break;
            }
            case PROBKI_SYGNAL: {
                double t, y;
                in >> t >> y;
                emit otrzymanoProbki(t, y);
                break;
            }
            default:
                qDebug() << "[SERWER] Odrzucono nieznany pakiet o ID:" << pobraneID;
                break;
        }
        oczekiwanyRozmiar = 0;
    }
}
void SerwerTCP::sendConf(TypPakietu pakietID, const RegulatorPID &pid){
    if(!m_clientSocket || m_clientSocket->state() != QAbstractSocket::ConnectedState){
        std::cout << "[SERWER] Brak klienta" << std::endl;
        return;
    }
    QByteArray dane;
    QDataStream out(&dane, QIODevice::WriteOnly);
    out << (quint32)0;
    out << (quint8)pakietID;
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    out << timestamp;
    out << pid;
    out.device()->seek(0);
    out << (quint32)dane.size();
    m_clientSocket->write(dane);
    m_clientSocket->waitForBytesWritten();
    std::cout << "Wyslano konfig" << std::endl;
}
void SerwerTCP::sendConf(TypPakietu id, const GeneratorWartosci &gen) {
    if(!m_clientSocket || m_clientSocket->state() != QAbstractSocket::ConnectedState){
        std::cout << "[SERWER] Brak klienta" << std::endl;
        return;
    }
    QByteArray dane;
    QDataStream out(&dane, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);

    out << (quint32)0;
    out << (quint8)id;
    out << QDateTime::currentMSecsSinceEpoch();
    out << gen;
    out.device()->seek(0);
    out << (quint32)dane.size();
    m_clientSocket->write(dane);
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
