#ifndef UARSERVICE_H
#define UARSERVICE_H

#include <QObject>
#include <QTimer>
#include "UAR.h"
#include <QString>

struct SimulationData {
    double x;        // Oś czasu (nr kroku)
    double y;        // Wartość regulowana
    double setpoint; // Wartość zadana
    double error;    // Uchyb
    double u;        // Sterowanie
    double uP;
    double uI;
    double uD;       // Składowe PID
};

class UARService : public QObject {
    Q_OBJECT

public:
    explicit UARService(QObject *parent = nullptr);

    // --- Metody sterujące symulacją ---
    void startSimulation(int intervalMs);
    void stopSimulation();
    void resetSimulation();
    bool isRunning() const;
    void setInterval(int intervalMs); // Zmiana prędkości w locie
    double getInterval() const;

    // --- Konfiguracja ---
    void configurePID(double k, double Ti, double Td, int trybIdx);
    void resetPID();
    // void updateTrybCalk();
    void configureGenerator(int trybIdx, double okres, double amplituda, double skladowaStala, double wypelnienie, int interwal_ms);

    void configureARX(const QString &aStr, const QString &bStr, int k,
                      double uMin, double uMax, double yMin, double yMax, double szumStd, bool limityOn);

signals:
    // Wysyłany do MainWindow, gdy są nowe dane do narysowania
    void simulationUpdated(SimulationData data);

private slots:
    // Slot wewnętrzny, który działa razem z timerem
    void performStep();

private:
    ProstyUAR m_uar;
    QTimer *m_timer;
    int m_step = 0;
};

#endif // UARSERVICE_H
