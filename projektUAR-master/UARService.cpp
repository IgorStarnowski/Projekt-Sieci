#include "UARService.h"
#include <QStringList>

UARService::UARService(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    // Łączymy tykanie timera z metodą performStep
    connect(m_timer, &QTimer::timeout, this, &UARService::performStep);
}

// Sterowanie

void UARService::startSimulation(int intervalMs) {
    if(intervalMs < 10) intervalMs = 10;
    m_timer->start(intervalMs); // Uruchomienie wewnętrznego timera
}

void UARService::stopSimulation() {
    m_timer->stop();
}

void UARService::resetSimulation() {
    stopSimulation();
    m_uar.reset(); // Reset matematyki
    m_step = 0;    // Reset licznika czasu
}

bool UARService::isRunning() const {
    return m_timer->isActive();
}

void UARService::setInterval(int intervalMs) {
    if(intervalMs < 10) intervalMs = 10;
    m_timer->setInterval(intervalMs);
}

double UARService::getInterval() const  {
    return m_timer->interval() / 1000.0;
}

// Główna pętla

void UARService::performStep() {
    SimulationData data;

    // Logika
    double y = m_uar.symuluj();

    // Pakowanie danych
    data.x = static_cast<double>(m_step);
    data.y = y;
    data.setpoint = m_uar.getGen().getVal();
    data.error = m_uar.getE();
    data.u = m_uar.getU();

    // Składowe PID
    data.uP = m_uar.getPID().getUP();
    data.uI = m_uar.getPID().getUI();
    data.uD = m_uar.getPID().getUD();

    m_step++;

    // Wysłanie danych do gui
    emit simulationUpdated(data);
}

// Konfiguracja

void UARService::configurePID(double k, double Ti, double Td, int trybIdx) {
    LiczCalk tryb = (trybIdx == 0) ? LiczCalk::Wew : LiczCalk::Zew;
    m_uar.getPID().setNastawy(k, Ti, Td, tryb);
}

void UARService::resetPID() { m_uar.resetPID(); }

// void UARService::updateTrybCalk() {
//     m_uar.
// }

void UARService::configureGenerator(int trybIdx, double okres, double amplituda, double skladowaStala, double wypelnienie, int interwal_ms) {
    TrybGen tryb = (trybIdx == 0) ? TrybGen::Pros : TrybGen::Sin;
    m_uar.getGen().setParams(tryb, okres, amplituda, skladowaStala, wypelnienie, interwal_ms);
}

void UARService::configureARX(const QString &aStr, const QString &bStr, int k,
                              double uMin, double uMax, double yMin, double yMax, double szumStd, bool limityOn)
{
    std::vector<double> va, vb;
    QStringList aList = aStr.split(",", Qt::SkipEmptyParts);
    QStringList bList = bStr.split(",", Qt::SkipEmptyParts);

    for(const QString &s : aList) va.push_back(s.trimmed().toDouble());
    for(const QString &s : bList) vb.push_back(s.trimmed().toDouble());

    if(va.empty()) va = {-0.5, 0.0, 0.0};
    if(vb.empty()) vb = {0.5, 0.0, 0.0};

    m_uar.getARX().setParams(va, vb, k);
    m_uar.getARX().setLimity(uMin, uMax, yMin, yMax, limityOn);
    m_uar.getARX().setSzum(szumStd);
}
