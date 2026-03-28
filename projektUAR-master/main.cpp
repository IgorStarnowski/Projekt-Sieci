//#include "mainwindow.h"
//#include <QApplication>
//#include <iostream>
//#include "UAR.h"

//int main(int argc, char *argv[])
//{
//    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
//    return a.exec();
//}
#include <QCoreApplication>
#include <iostream>
#include <thread>
#include "serwertcp.h"
#include "klienttcp.h"
#include "UAR.h"
#include <chrono>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    int wybor;
    std::cout << "=== TEST SIECIOWY UAR ===" << std::endl;
    std::cout << "1. Uruchom jako SERWER (Czeka na paczki)" << std::endl;
    std::cout << "2. Uruchom jako KLIENT (Interaktywne wysylanie)" << std::endl;
    std::cout << "Wybierz (1 lub 2): ";
    std::cin >> wybor;

    SerwerTCP* serwer = nullptr;
    klientTCP* klient = nullptr;

    if (wybor == 1) {
        serwer = new SerwerTCP();
        serwer->startListening(12345);
        std::cout << "[SERWER] Nasluchuje na porcie 12345. Czekam na klienta..." << std::endl;
    }
    else if (wybor == 2) {
        klient = new klientTCP();
        klient->conToServ("127.0.0.1", 12345);
        std::thread watekMenu([klient, &a]() {
            char akcja;
            while (true) {
                std::cout << "\n--- MENU KLIENTA ---\n";
                std::cout << "p - Wyslij konfiguracje PID\n";
                std::cout << "a - Wyslij konfiguracje ARX\n";
                std::cout << "q - Wyjdz z programu\n";
                std::cout << "Wybor: ";
                std::cin >> akcja;

                if (akcja == 'p') {
                    RegulatorPID testPID;
                    testPID.setNastawy(3.14, 9.99, 0.55, LiczCalk::Wew);
                    std::cout << ">> Wysylam paczke PID..." << std::endl;
                    QMetaObject::invokeMethod(klient, [klient, testPID]() {
                        klient->sendConf(1, testPID);
                    });
                }
                else if (akcja == 'a') {
                    ModelARX testARX;
                    testARX.setParams({1.5, -0.7}, {0.3, 0.1}, 2);
                    std::cout << ">> Wysylam paczke ARX..." << std::endl;
                    QMetaObject::invokeMethod(klient, [klient, testARX]() {
                        klient->sendConf(2, testARX);
                    });
                }
                else if (akcja == 'q') {
                    std::cout << "Zamykanie..." << std::endl;
                    a.quit();
                    break;
                }
                else {
                    std::cout << "Nieznana komenda!" << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        });
        watekMenu.detach();
    }
    else {
        std::cout << "Zly wybor. Zamykam." << std::endl;
        return 0;
    }

    return a.exec();
}
