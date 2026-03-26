#ifndef UAR_H
#define UAR_H

#include <vector>
#include <deque>
#include <cstdlib>
#include <cmath>
#include <random>

//using namespace std;

enum LiczCalk {Wew = 0, Zew = 1}; // Wew: Stala w sumie, Zew: Stala przed suma
enum TrybGen {Sin = 0, Pros = 1};

// ARX
class ModelARX
{
private:
    std::vector<double> m_A;
    std::vector<double> m_B;
    unsigned int m_k; // Opóźnienie

    // Parametry szumu
    double m_szum = 0.0;
    std::mt19937 gen;
    std::normal_distribution<double> dist;

    // Bufory historii
    std::deque<double> m_historia_u;
    std::deque<double> m_historia_y;

    // Ograniczenia
    bool m_ogranicz_u = true;
    bool m_ogranicz_y = true;
    double m_minU = -10.0, m_maxU = 10.0;
    double m_minY = -10.0, m_maxY = 10.0;

public:
    ModelARX();

    // Konfiguracja
    void setParams(const std::vector<double>& wsp_a, const std::vector<double>& wsp_b, unsigned int opoznienie_k);
    void setLimity(double minU, double maxU, double minY, double maxY, bool wlaczone);
    void setSzum(double std_dev);

    // Główna funkcja
    double symuluj(double sterowanie);
    void reset();
};

// PID
class RegulatorPID
{
private:
    double m_k = 1.0;
    double m_Ti = 1.0;
    double m_Td = 0.0;
    LiczCalk m_liczCalk = LiczCalk::Wew;

    double m_prev_e = 0.0;
    double m_suma_e = 0.0;

    // Składowe do wykresów
    double m_u_P = 0.0;
    double m_u_I = 0.0;
    double m_u_D = 0.0;

public:
    RegulatorPID();

    void setNastawy(double k, double Ti, double Td, LiczCalk tryb);
    // void updateTrybCalki();
    double symuluj(double uchyb);
    void reset();
    void resetMemory();

    // Gettery do wykresów
    double getUP() const { return m_u_P; }
    double getUI() const { return m_u_I; }
    double getUD() const { return m_u_D; }
};

// Klasa Generatora
class GeneratorWartosci
{
private:
    int m_T_probki = 50; // Wyliczony okres w próbkach
    int m_T_T = 200;     // Interwał timera (ms)
    double m_T_RZ = 10.0; // Okres rzeczywisty (s)

    int m_i = 0; // Licznik kroków

    double m_A = 1.0;
    double m_S = 0.0;
    double m_p = 0.5; // Wypełnienie
    TrybGen m_tryb = TrybGen::Pros;

    double m_w_i = 0.0; // Ostatnia wartość

    void aktualizujT(); // Przelicza TRZ na T_probki

public:
    GeneratorWartosci();

    void setParams(TrybGen tryb, double okres_rzecz, double ampl, double off, double wyp, int interwal_ms);
    double generuj();
    double getVal() const { return m_w_i; }
    void reset();
};

// UAR
class ProstyUAR
{
private:
    ModelARX m_ARX;
    RegulatorPID m_PID;
    GeneratorWartosci m_genWart;

    double m_y_i = 0.0;
    double m_e_i = 0.0;
    double m_u_i = 0.0;

public:
    ProstyUAR();

    double symuluj();
    void reset();
    void resetPID();

    ModelARX& getARX() { return m_ARX; }
    RegulatorPID& getPID() { return m_PID; }
    GeneratorWartosci& getGen() { return m_genWart; }

    // Gettery sygnalow
    double getE() const { return m_e_i; }
    double getU() const { return m_u_i; }
    double getY() const { return m_y_i; }
};

#endif
