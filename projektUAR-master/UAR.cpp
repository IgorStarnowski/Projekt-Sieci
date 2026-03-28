#include "UAR.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Model ARX

ModelARX::ModelARX() : gen(std::random_device{}()) {
    m_A = {-0.5, 0.0, 0.0};
    m_B = {0.5, 0.0, 0.0};
    m_k = 1;
    // Inicjalizacja buforów zerami
    m_historia_u.resize(20, 0.0);
    m_historia_y.resize(20, 0.0);
}

void ModelARX::setParams(const std::vector<double>& wsp_a, const std::vector<double>& wsp_b, unsigned int opoznienie_k) {
    m_A = wsp_a;
    m_B = wsp_b;
    m_k = (opoznienie_k < 1) ? 1 : opoznienie_k;

    // Rozszerz bufory jeśli potrzeba (z zapasem)
    size_t required = m_k + m_B.size() + m_A.size() + 5;
    if (m_historia_u.size() < required) {
        m_historia_u.resize(required, 0.0);
        m_historia_y.resize(required, 0.0);
    }
}

void ModelARX::setLimity(double minU, double maxU, double minY, double maxY, bool wlaczone) {
    m_minU = minU; m_maxU = maxU;
    m_minY = minY; m_maxY = maxY;

    m_ogranicz_u = wlaczone;
    m_ogranicz_y = wlaczone;
}

void ModelARX::setSzum(double std_dev) {
    m_szum = std_dev;
    if (m_szum > 0.0) dist = std::normal_distribution<double>(0.0, m_szum);
}

double ModelARX::symuluj(double u_raw) {
    // Nasycenie wejścia przed obliczeniami
    double u = u_raw;
    if (m_ogranicz_u) {
        if (u > m_maxU) u = m_maxU;
        if (u < m_minU) u = m_minU;
    }

    // Aktualizacja historii sterowania
    m_historia_u.push_front(u);
    if (m_historia_u.size() > 100) m_historia_u.pop_back();

    // Obliczenia ARX
    double y_temp = 0.0;

    // Część od sterowania (B)
    for (size_t i = 0; i < m_B.size(); ++i) {
        size_t idx = m_k + i;
        if (idx < m_historia_u.size()) {
            y_temp += m_B[i] * m_historia_u[idx];
        }
    }

    // Część od wyjścia (A) - odejmujemy
    for (size_t i = 0; i < m_A.size(); ++i) {
        size_t idx = i;
        if (idx < m_historia_y.size()) {
            y_temp -= m_A[i] * m_historia_y[idx];
        }
    }

    // Dodanie szumu
    if (m_szum > 0.0001) {
        y_temp += dist(gen);
    }

    // Nasycenie wyjścia
    if (m_ogranicz_y) {
        if (y_temp > m_maxY) y_temp = m_maxY;
        if (y_temp < m_minY) y_temp = m_minY;
    }

    // Aktualizacja historii wyjścia
    m_historia_y.push_front(y_temp);
    if (m_historia_y.size() > 100) m_historia_y.pop_back();

    return y_temp;
}
std::vector<std::byte> ModelARX::serializuj()const{
    const size_t RZ_A = m_A.size() * sizeof(double);
    const size_t RZ_B = m_B.size() * sizeof(double);
    const size_t LEN_A = m_A.size();
    const size_t LEN_B = m_B.size();
    const size_t RZ_PROSTE = sizeof(m_k) + sizeof(m_szum) + sizeof(m_minU)*4 + sizeof(m_ogranicz_u)*2;
    std::vector<std::byte> buf(sizeof(size_t)*2 + RZ_A + RZ_B + RZ_PROSTE);
    std::byte* ptr = buf.data();
    memcpy(ptr,&LEN_A, sizeof(size_t));
    memcpy(ptr += sizeof(size_t), m_A.data(), RZ_A);
    memcpy(ptr += RZ_A, &LEN_B, sizeof(size_t));
    memcpy(ptr+= sizeof(size_t), m_B.data(), RZ_B);
    ptr += RZ_B;
    memcpy(ptr, &m_k, sizeof(m_k));
    memcpy(ptr += sizeof(m_k), &m_szum, sizeof(m_szum));
    memcpy(ptr += sizeof(m_szum), &m_ogranicz_u, sizeof(m_ogranicz_u));
    memcpy(ptr += sizeof(m_ogranicz_u), &m_ogranicz_y, sizeof(m_ogranicz_y));
    memcpy(ptr += sizeof(m_ogranicz_y), &m_minU, sizeof(m_minU));
    memcpy(ptr += sizeof(m_minU), &m_maxU, sizeof(m_maxU));
    memcpy(ptr += sizeof(m_maxU), &m_minY, sizeof(m_minY));
    memcpy(ptr += sizeof(m_minY), &m_maxY, sizeof(m_maxY));

    return buf;

}
void ModelARX::deserializuj(const std::vector<std::byte>& buf){
    const std::byte* ptr = buf.data();
    size_t LEN_A = 0, LEN_B = 0;
    memcpy(&LEN_A, ptr, sizeof(size_t));
    m_A.resize(LEN_A);
    size_t RZ_A = LEN_A * sizeof(double);
    memcpy(m_A.data(), ptr+= sizeof(size_t), RZ_A);
    memcpy(&LEN_B, ptr += RZ_A, sizeof(size_t));
    m_B.resize(LEN_B);
    size_t RZ_B = LEN_B * sizeof(double);
    memcpy(m_B.data(), ptr+= sizeof(size_t), RZ_B);
    ptr += RZ_B;
    memcpy(&m_k, ptr, sizeof(m_k));
    memcpy(&m_szum, ptr += sizeof(m_k), sizeof(m_szum));
    memcpy(&m_ogranicz_u, ptr += sizeof(m_szum), sizeof(m_ogranicz_u));
    memcpy(&m_ogranicz_y, ptr += sizeof(m_ogranicz_u), sizeof(m_ogranicz_y));
    memcpy(&m_minU, ptr += sizeof(m_ogranicz_y), sizeof(m_minU));
    memcpy(&m_maxU, ptr += sizeof(m_minU), sizeof(m_maxU));
    memcpy(&m_minY, ptr += sizeof(m_maxU), sizeof(m_minY));
    memcpy(&m_maxY, ptr += sizeof(m_minY), sizeof(m_maxY));
}

void ModelARX::reset() {
    std::fill(m_historia_u.begin(), m_historia_u.end(), 0.0);
    std::fill(m_historia_y.begin(), m_historia_y.end(), 0.0);
}

// Regulator PID

RegulatorPID::RegulatorPID() {}

void RegulatorPID::setNastawy(double k, double Ti, double Td, LiczCalk tryb) {
    m_k = k;
    m_Ti = Ti;
    m_Td = Td;

    if (tryb != m_liczCalk)
    {
        if (tryb == LiczCalk::Wew)
            m_suma_e = m_suma_e / m_Ti * 1.0;
        else
            m_suma_e = m_suma_e * m_Ti * 1.0;
    }


    m_liczCalk = tryb;
}
std::vector<std::byte> RegulatorPID::serializuj() const{
    constexpr size_t S_K = sizeof(m_k);
    constexpr size_t S_TI = sizeof(m_Ti);
    constexpr size_t S_TD = sizeof(m_Td);
    constexpr size_t S_LC = sizeof(m_liczCalk);

    std::vector<std::byte> buf(S_K+S_TI+S_TD+S_LC);
    std::byte* ptr = buf.data();
    memcpy(ptr, &m_k, S_K);
    memcpy(ptr+=S_K, &m_Ti, S_TI);
    memcpy(ptr+=S_TI, &m_Td, S_TD);
    memcpy(ptr+=S_TD, &m_liczCalk, S_LC);
    return buf;
}
void RegulatorPID::deserializuj(const std::vector<std::byte>& buf){
    const std::byte* ptr = buf.data();
    constexpr size_t S_K = sizeof(m_k);
    constexpr size_t S_TI = sizeof(m_Ti);
    constexpr size_t S_TD = sizeof(m_Td);
    memcpy(&m_k, ptr, S_K);
    memcpy(&m_Ti, ptr += S_K, S_TI);
    memcpy(&m_Td, ptr += S_TI, S_TD);
    memcpy(&m_liczCalk, ptr += S_TD, sizeof(m_liczCalk));
}


double RegulatorPID::symuluj(double e) {
    // Proporcjonalna
    m_u_P = m_k * e * 1.0;

    double I_temp = 0.0;

    // Całkująca
    if (m_Ti == 0.0) {
        m_u_I = 0.0; // Człon wyłączony
    } else {
        if (m_liczCalk == LiczCalk::Wew) { // Stała PRZED sumą
            m_suma_e += e / m_Ti;
            I_temp = m_suma_e * 1.0;
        } else { // Stała W sumie (Pod całką)
            m_suma_e += e;  // / m_Ti;
            I_temp = m_suma_e / m_Ti * 1.0;  // * m_k;
        }
    }

    m_u_I = m_k * I_temp;

    // Różniczkująca
    m_u_D = m_Td * (e - m_prev_e) * m_k;
    m_prev_e = e;

    return m_u_P + m_u_I + m_u_D;
}

void RegulatorPID::reset() {
    m_u_P = m_u_I = m_u_D = 0.0;
    m_suma_e = 0.0;
    m_prev_e = 0.0;
}

void RegulatorPID::resetMemory() {
    m_suma_e = 0.0;
    m_prev_e = 0.0;

    m_u_I = 0.0;
}

// Generator

GeneratorWartosci::GeneratorWartosci() {}

void GeneratorWartosci::setParams(TrybGen tryb, double okres_rzecz, double ampl, double off, double wyp, int interwal_ms) {
    m_tryb = tryb;
    m_T_RZ = okres_rzecz;
    m_A = ampl;
    m_S = off;
    m_p = wyp;
    m_T_T = interwal_ms;
    aktualizujT();
}

void GeneratorWartosci::aktualizujT() {
    // Przeliczenie okresu w sekundach na okres w próbkach
    if (m_T_T <= 0) m_T_T = 200;
    double probek_na_sekunde = m_T_RZ * 1000.0 / m_T_T;
    m_T_probki = static_cast<int>(probek_na_sekunde);
    if (m_T_probki < 1) m_T_probki = 1;
}

double GeneratorWartosci::generuj() {
    // Obliczenie fazy sygnału
    int faza = m_i % m_T_probki;
    double val = 0.0;

    if (m_tryb == TrybGen::Sin) {
        double ratio = (double)faza / (double)m_T_probki;
        val = m_A * std::sin(ratio * 2.0 * M_PI) + m_S;
    } else {
        // Prostokąt
        double limit = m_p * m_T_probki;
        if (faza < limit) val = m_A + m_S;
        else val = -m_A + m_S;
    }

    m_w_i = val;
    m_i++;
    return val;
}

void GeneratorWartosci::reset() {
    m_i = 0;
    m_w_i = 0.0;
}

// UAR

ProstyUAR::ProstyUAR() {}

double ProstyUAR::symuluj() {
    // Wyznacz wartość zadaną
    double w = m_genWart.generuj();

    // Oblicz uchyb (wartość zadana - poprzednie wyjście obiektu)
    // Sprzężenie zwrotne bierze y z poprzedniego kroku
    m_e_i = w - m_y_i;

    // Regulator PID
    m_u_i = m_PID.symuluj(m_e_i);

    // ARX
    m_y_i = m_ARX.symuluj(m_u_i);

    return m_y_i;
}

void ProstyUAR::reset() {
    m_ARX.reset();
    m_PID.reset();
    m_genWart.reset();
    m_y_i = 0.0;
    m_e_i = 0.0;
    m_u_i = 0.0;
}

void ProstyUAR::resetPID()  {
    m_PID.resetMemory();
}
