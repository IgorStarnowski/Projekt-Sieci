#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_service(new UARService(this))
{
    ui->setupUi(this);
    oknoARX = new DialogARX(this);
    // Tworzenie wykresów i dodawanie do layoutu
    m_plotY = new QCustomPlot();
    m_plotError = new QCustomPlot();
    m_plotU = new QCustomPlot();
    m_plotUComp = new QCustomPlot();

    ui->layoutPlotY->addWidget(m_plotY);
    ui->layoutPlotError->addWidget(m_plotError);
    ui->layoutPlotU->addWidget(m_plotU);
    ui->layoutPlotUComp->addWidget(m_plotUComp);

    aktualnyCzas = 0.0;

    // Konfiguracja wykresów i połączeń
    setupPlots();
    setupConnections();

    // Inicjalizacja parametrów początkowych
    updateParameters();
    pushARXParamsToService();
    ui->statusLabel->setText("STATUS: TRYB LOKALNY");
    ui->statusLabel->setStyleSheet("background-color: #007bff; color: white; font-weight: bold; padding: 5px; border-radius: 3px;");
    zarzadzajKontrolkami(false, false);
    setupAutoSendConnections();
}

MainWindow::~MainWindow() {
    delete ui;
}

// Konfiguracja wykresów
void MainWindow::setupPlots() {
    // Wykres 1: Wartość Zadana (Setpoint) i Regulowana (Y)
    m_plotY->addGraph(); // Index 0: Wartość regulowana
    m_graphY_regulowana = m_plotY->graph(0);
    m_graphY_regulowana->setPen(QPen(Qt::blue));
    m_graphY_regulowana->setName("y (Wyjście)");

    m_plotY->addGraph(); // Index 1: Wartość zadana
    m_graphY_zadana = m_plotY->graph(1);
    // Linia przerywana dla wartości zadanej
    QPen setpointPen(Qt::red);
    setpointPen.setStyle(Qt::DashLine);
    m_graphY_zadana->setPen(setpointPen);
    m_graphY_zadana->setName("w (Zadana)");

    // Legenda
    m_plotY->legend->setVisible(true);
    QFont legendFont = font();
    legendFont.setPointSize(8);
    m_plotY->legend->setFont(legendFont);
    m_plotY->legend->setFillOrder(QCPLegend::foColumnsFirst);
    m_plotY->legend->setWrap(4);
    // Pozycjonowanie w prawym górnym rogu
    m_plotY->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);
    m_plotY->legend->setBrush(QBrush(QColor(255, 255, 255, 150)));
    m_plotY->legend->setBorderPen(Qt::NoPen);

    m_plotY->setInteraction(QCP::iRangeDrag, true);
    m_plotY->setInteraction(QCP::iRangeZoom, true);
    m_plotY->yAxis->setLabel("Wartość");

    // Wykres 2: Uchyb (Error)
    m_plotError->addGraph();
    m_graphError = m_plotError->graph(0);
    m_graphError->setPen(QPen(Qt::darkRed));
    m_plotError->yAxis->setLabel("Uchyb (e)");

    // Wykres 3: Sterowanie (U)
    m_plotU->addGraph();
    m_graphU = m_plotU->graph(0);
    m_graphU->setPen(QPen(Qt::darkGreen));
    m_plotU->yAxis->setLabel("Sterowanie (u)");

    // Wykres 4: PID
    m_plotUComp->addGraph(); // P
    m_graphU_P = m_plotUComp->graph(0);
    m_graphU_P->setPen(QPen(Qt::red));
    m_graphU_P->setName("P");

    m_plotUComp->addGraph(); // I
    m_graphU_I = m_plotUComp->graph(1);
    m_graphU_I->setPen(QPen(Qt::blue));
    m_graphU_I->setName("I");

    m_plotUComp->addGraph(); // D
    m_graphU_D = m_plotUComp->graph(2);
    m_graphU_D->setPen(QPen(Qt::green));
    m_graphU_D->setName("D");

    m_plotUComp->legend->setVisible(true);
    m_plotUComp->legend->setFont(legendFont);
    m_plotUComp->legend->setFillOrder(QCPLegend::foColumnsFirst);
    m_plotUComp->legend->setWrap(4);
    m_plotUComp->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);
    m_plotUComp->legend->setBrush(QBrush(QColor(255, 255, 255, 150)));
    m_plotUComp->legend->setBorderPen(Qt::NoPen);
    m_plotUComp->yAxis->setLabel("Składowe PID");
}

// Połączenia sygnałów i slotów
void MainWindow::setupConnections() {
    connect(m_service, &UARService::simulationUpdated, this, &MainWindow::onSimulationUpdated);

    // Przyciski sterujące
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::stopSimulation);
    connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::resetSimulation);
    connect(ui->btnPIDReset, &QPushButton::clicked, this, &MainWindow::resetPID);

    // Przycisk konfiguracji ARX
    connect(ui->btnARX, &QPushButton::clicked, this, &MainWindow::openARXDialog);

    // Zapis i odczyt
    connect(ui->btnSave, &QPushButton::clicked, this, &MainWindow::saveConfig);
    connect(ui->btnLoad, &QPushButton::clicked, this, &MainWindow::loadConfig);

    connect(ui->spinK,      &QDoubleSpinBox::editingFinished, this, &MainWindow::updateParameters);
    connect(ui->spinTi,     &QDoubleSpinBox::editingFinished, this, &MainWindow::updateParameters);
    connect(ui->spinTd,     &QDoubleSpinBox::editingFinished, this, &MainWindow::updateParameters);
    connect(ui->spinAmp,    &QDoubleSpinBox::editingFinished, this, &MainWindow::updateParameters);
    connect(ui->spinOffset, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateParameters);
    connect(ui->spinOkres,  &QDoubleSpinBox::editingFinished, this, &MainWindow::updateParameters);
    connect(ui->spinFill,   &QDoubleSpinBox::editingFinished, this, &MainWindow::updateParameters);

    connect(ui->spinWidth,    &QSpinBox::editingFinished, this, &MainWindow::updateParameters);
    connect(ui->spinInterval, &QSpinBox::editingFinished, this, &MainWindow::updateParameters);

    connect(ui->comboPIDMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateParameters);
    connect(ui->comboGenType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateParameters);
    connect(ui->spinInterval, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateParameters);
}

// Odbiór danych
void MainWindow::onSimulationUpdated(SimulationData data) {

    // Dodaj dane do wykresów
    double t = m_service->getInterval(); // Czas w sekundach

    m_graphY_regulowana->addData(aktualnyCzas, data.y);
    m_graphY_zadana->addData(aktualnyCzas, data.setpoint);
    m_graphError->addData(aktualnyCzas, data.error);
    m_graphU->addData(aktualnyCzas, data.u);
    m_graphU_P->addData(aktualnyCzas, data.uP);
    m_graphU_I->addData(aktualnyCzas, data.uI);
    m_graphU_D->addData(aktualnyCzas, data.uD);

    // Przesuwanie okna
    double windowSize = (double)ui->spinWidth->value();

    double startTime = aktualnyCzas - windowSize;
    if (startTime < 0) startTime = 0;

    // Ustawienie zakresów osi X
    m_plotY->xAxis->setRange(startTime, aktualnyCzas);     // Marginsu z prawej
    m_plotError->xAxis->setRange(startTime, aktualnyCzas);
    m_plotU->xAxis->setRange(startTime, aktualnyCzas);
    m_plotUComp->xAxis->setRange(startTime, aktualnyCzas);

    // Usuwanie starych danych
    double deleteOldData = startTime;
    if (deleteOldData > 0) {
        m_graphY_regulowana->data()->removeBefore(deleteOldData);
        m_graphY_zadana->data()->removeBefore(deleteOldData);
        m_graphError->data()->removeBefore(deleteOldData);
        m_graphU->data()->removeBefore(deleteOldData);
        m_graphU_P->data()->removeBefore(deleteOldData);
        m_graphU_I->data()->removeBefore(deleteOldData);
        m_graphU_D->data()->removeBefore(deleteOldData);
    }

    // Autoskalowanie Osi Y
    auto applySmartScale = [](QCustomPlot* plot) {
        plot->yAxis->rescale(true);
        QCPRange range = plot->yAxis->range();
        double d = range.upper - range.lower;

        if (d < 0.0001) {
            plot->yAxis->setRange(range.lower - 1.0, range.upper + 1.0);
        } else {
            double margin = d * 0.125;
            // Dodajemy margines u góry i u dołu
            plot->yAxis->setRange(range.lower - margin, range.upper + margin + 0.4);
        }
    };

    applySmartScale(m_plotY);
    applySmartScale(m_plotError);
    applySmartScale(m_plotU);
    applySmartScale(m_plotUComp);

    // Odświeżenie widoku
    m_plotY->replot();
    m_plotError->replot();
    m_plotU->replot();
    m_plotUComp->replot();

    aktualnyCzas += t;
}

// Sterowanie
void MainWindow::startSimulation() {
    int interval = ui->spinInterval->value();

    // Zaktualizuj parametry przed startem
    updateParameters();

    // Zlecenie startu dla UARService
    m_service->startSimulation(interval);
}

void MainWindow::stopSimulation() {
    // Zlecenie stopu dla UARService
    m_service->stopSimulation();
}

void MainWindow::resetSimulation() {
    // Reset w UARService
    m_service->resetSimulation();

    // Czyszczenie wykresów
    m_graphY_regulowana->data()->clear();
    m_graphY_zadana->data()->clear();
    m_graphError->data()->clear();
    m_graphU->data()->clear();
    m_graphU_P->data()->clear();
    m_graphU_I->data()->clear();
    m_graphU_D->data()->clear();

    // Replot
    m_plotY->replot();
    m_plotError->replot();
    m_plotU->replot();
    m_plotUComp->replot();

    aktualnyCzas = 0.0;
}

void MainWindow::resetPID()
{
    m_service->resetPID();
}

// Konfiguracja w locie
void MainWindow::updateParameters() {
    // PID
    m_service->configurePID(
        ui->spinK->value(),
        ui->spinTi->value(),
        ui->spinTd->value(),
        ui->comboPIDMethod->currentIndex()
        );

    //m_service->updateTrybCalk();


    // Generator
    m_service->configureGenerator(
        ui->comboGenType->currentIndex(),
        ui->spinOkres->value(),
        ui->spinAmp->value(),
        ui->spinOffset->value(),
        ui->spinFill->value(),
        ui->spinInterval->value()
        );

    // Jeśli zmieniono interwał w trakcie pracy
    if (m_service->isRunning()) {
        m_service->setInterval(ui->spinInterval->value());
    }
        ui->StatusBar->showMessage("Zaktualizowano parametry", 2000);
}

// Obsługa okna dialogowego ARX
void MainWindow::openARXDialog() {

    oknoARX->setData(m_curA, m_curB, m_curK,
                m_curMinU, m_curMaxU, m_curMinY, m_curMaxY, m_curNoise,
                m_curLimitsOn);

    if (oknoARX->exec() == QDialog::Accepted) {
        m_curA = oknoARX->getA();
        m_curB = oknoARX->getB();
        m_curK = oknoARX->getK();

        m_curMinU = oknoARX->getMinU();
        m_curMaxU = oknoARX->getMaxU();
        m_curMinY = oknoARX->getMinY();
        m_curMaxY = oknoARX->getMaxY();
        m_curNoise = oknoARX->getNoise();
        m_curLimitsOn = oknoARX->getLimityWlaczone();

        if (klient != nullptr) {
            ModelARX pakietARX;

            auto toVec = [](const QString& s) {
                std::vector<double> v;
                QStringList list = s.split(',', Qt::SkipEmptyParts);
                for(const QString& item : list) {
                    v.push_back(item.trimmed().toDouble());
                }
                return v;
            };

            pakietARX.setParams(toVec(m_curA), toVec(m_curB), m_curK);
            pakietARX.setLimity(m_curMinU, m_curMaxU, m_curMinY, m_curMaxY, m_curLimitsOn);
            pakietARX.setSzum(m_curNoise);

            klient->sendConf(KONF_ARX, pakietARX);

            ui->StatusBar->showMessage("Wysłano parametry obiektu ARX (po edycji)", 3000);
        }

        pushARXParamsToService();
    }
}

void MainWindow::pushARXParamsToService() {
    m_service->configureARX(m_curA, m_curB, m_curK,
                            m_curMinU, m_curMaxU, m_curMinY, m_curMaxY, m_curNoise,
                            m_curLimitsOn);
}

// Zapis i odczyt (JSON)
void MainWindow::saveConfig() {
    QString fileName = QFileDialog::getSaveFileName(this, "Zapisz konfigurację", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;

    QJsonObject root;

    // ARX
    QJsonObject arxObj;
    arxObj["A"] = m_curA;
    arxObj["B"] = m_curB;
    arxObj["k"] = m_curK;
    arxObj["minU"] = m_curMinU;
    arxObj["maxU"] = m_curMaxU;
    arxObj["minY"] = m_curMinY;
    arxObj["maxY"] = m_curMaxY;
    arxObj["noise"] = m_curNoise;
    arxObj["limitsOn"] = m_curLimitsOn;
    root["ARX"] = arxObj;

    // PID
    QJsonObject pidObj;
    pidObj["k"] = ui->spinK->value();
    pidObj["Ti"] = ui->spinTi->value();
    pidObj["Td"] = ui->spinTd->value();
    pidObj["method"] = ui->comboPIDMethod->currentIndex();
    root["PID"] = pidObj;

    // Generator
    QJsonObject genObj;
    genObj["type"] = ui->comboGenType->currentIndex();
    genObj["period"] = ui->spinOkres->value();
    genObj["amp"] = ui->spinAmp->value();
    genObj["offset"] = ui->spinOffset->value();
    genObj["fill"] = ui->spinFill->value();
    genObj["interval"] = ui->spinInterval->value();
    root["Gen"] = genObj;

    QJsonDocument doc(root);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void MainWindow::loadConfig() {
    QString fileName = QFileDialog::getOpenFileName(this, "Wczytaj konfigurację", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();

    // Wczytanie ARX
    if (root.contains("ARX")) {
        QJsonObject arxObj = root["ARX"].toObject();
        m_curA = arxObj["A"].toString();
        m_curB = arxObj["B"].toString();
        m_curK = arxObj["k"].toInt();
        m_curMinU = arxObj["minU"].toDouble();
        m_curMaxU = arxObj["maxU"].toDouble();
        m_curMinY = arxObj["minY"].toDouble();
        m_curMaxY = arxObj["maxY"].toDouble();
        m_curNoise = arxObj["noise"].toDouble();
        m_curLimitsOn = arxObj["limitsOn"].toBool();

        pushARXParamsToService();
    }

    // Wczytanie PID
    if (root.contains("PID")) {
        QJsonObject pidObj = root["PID"].toObject();
        ui->spinK->setValue(pidObj["k"].toDouble());
        ui->spinTi->setValue(pidObj["Ti"].toDouble());
        ui->spinTd->setValue(pidObj["Td"].toDouble());
        ui->comboPIDMethod->setCurrentIndex(pidObj["method"].toInt());
    }

    // Wczytanie Generatora
    if (root.contains("Gen")) {
        QJsonObject genObj = root["Gen"].toObject();
        ui->comboGenType->setCurrentIndex(genObj["type"].toInt());
        ui->spinOkres->setValue(genObj["period"].toDouble());
        ui->spinAmp->setValue(genObj["amp"].toDouble());
        ui->spinOffset->setValue(genObj["offset"].toDouble());
        ui->spinFill->setValue(genObj["fill"].toDouble());
        ui->spinInterval->setValue(genObj["interval"].toInt());
    }

    // Wymuszenie aktualizacji
    updateParameters();

}
void MainWindow::zarzadzajKontrolkami(bool polaczono, bool toKlient) {
    // Przycisk łączenia i rozłączania reaguje zawsze na stan połączenia
    ui->btnSiec->setEnabled(!polaczono);
    ui->btnRozlacz->setEnabled(polaczono);

    if (polaczono) {
        if (toKlient) {
            ui->btnStart->setEnabled(false);
            ui->btnStop->setEnabled(false);
            ui->btnReset->setEnabled(false);
            ui->btnPIDReset->setEnabled(false);

            ui->comboPIDMethod->setEnabled(false);
            ui->spinK->setEnabled(false);
            ui->spinTi->setEnabled(false);
            ui->spinTd->setEnabled(false);

            ui->comboGenType->setEnabled(false);
            ui->spinOkres->setEnabled(false);
            ui->spinAmp->setEnabled(false);
            ui->spinOffset->setEnabled(false);
            ui->spinFill->setEnabled(false);
            ui->spinInterval->setEnabled(true);

            ui->btnARX->setEnabled(true);

            ui->btnSave->setEnabled(false);
            ui->btnLoad->setEnabled(false);

        } else {

            ui->btnStart->setEnabled(true);
            ui->btnStop->setEnabled(true);
            ui->btnReset->setEnabled(true);
            ui->btnPIDReset->setEnabled(true);

            ui->comboPIDMethod->setEnabled(true);
            ui->spinK->setEnabled(true);
            ui->spinTi->setEnabled(true);
            ui->spinTd->setEnabled(true);

            ui->comboGenType->setEnabled(true);
            ui->spinOkres->setEnabled(true);
            ui->spinAmp->setEnabled(true);
            ui->spinOffset->setEnabled(true);
            ui->spinFill->setEnabled(true);
            ui->spinInterval->setEnabled(true);

            oknoARX->zablokujPola(true);

            ui->btnSave->setEnabled(true);
            ui->btnLoad->setEnabled(true);
        }
    } else {

        ui->btnStart->setEnabled(true);
        ui->btnStop->setEnabled(true);
        ui->btnReset->setEnabled(true);
        ui->btnPIDReset->setEnabled(true);

        ui->btnARX->setEnabled(true);
        oknoARX->zablokujPola(false);

        ui->comboPIDMethod->setEnabled(true);
        ui->spinK->setEnabled(true);
        ui->spinTi->setEnabled(true);
        ui->spinTd->setEnabled(true);

        ui->comboGenType->setEnabled(true);
        ui->spinOkres->setEnabled(true);
        ui->spinAmp->setEnabled(true);
        ui->spinOffset->setEnabled(true);
        ui->spinFill->setEnabled(true);
        ui->spinInterval->setEnabled(true);

        ui->btnSave->setEnabled(true);
        ui->btnLoad->setEnabled(true);
    }
}
void MainWindow::setupAutoSendConnections() {

    connect(ui->spinK, &QDoubleSpinBox::editingFinished,
            this, &MainWindow::sendPIDConfig);
    connect(ui->spinTi, &QDoubleSpinBox::editingFinished,
            this, &MainWindow::sendPIDConfig);
    connect(ui->spinTd, &QDoubleSpinBox::editingFinished,
            this, &MainWindow::sendPIDConfig);

    connect(ui->comboPIDMethod, QOverload<int>::of(&QComboBox::activated),
            this, &MainWindow::sendPIDConfig);

    connect(ui->comboGenType, QOverload<int>::of(&QComboBox::activated),
            this, &MainWindow::sendGenConfig);

    connect(ui->spinOkres, &QDoubleSpinBox::editingFinished,
            this, &MainWindow::sendGenConfig);

    connect(ui->spinAmp, &QDoubleSpinBox::editingFinished,
            this, &MainWindow::sendGenConfig);

    connect(ui->spinOffset, &QDoubleSpinBox::editingFinished,
            this, &MainWindow::sendGenConfig);

    connect(ui->spinFill, &QDoubleSpinBox::editingFinished,
            this, &MainWindow::sendGenConfig);

}
void MainWindow::sendGenConfig() {
    if (serwer == nullptr) return;

    TrybGen tryb = static_cast<TrybGen>(ui->comboGenType->currentIndex());
    double okres = ui->spinOkres->value();
    double ampl = ui->spinAmp->value();
    double off = ui->spinOffset->value();
    double wyp = ui->spinFill->value();

    int interwal_ms = 200;

    GeneratorWartosci pakietGen;
    pakietGen.setParams(tryb, okres, ampl, off, wyp, interwal_ms);

    serwer->sendConf(KONF_GEN, pakietGen);

    ui->StatusBar->showMessage("Wysłano parametry generatora [Auto]", 2000);
}

void MainWindow::sendPIDConfig() {
    if (serwer == nullptr) return;

    m_pidK = ui->spinK->value();
    m_pidTi = ui->spinTi->value();
    m_pidTd = ui->spinTd->value();
    m_pidMethod = static_cast<LiczCalk>(ui->comboPIDMethod->currentIndex());

    RegulatorPID pakietPID;
    pakietPID.setNastawy(m_pidK, m_pidTi, m_pidTd, m_pidMethod);

    serwer->sendConf(KONF_PID, pakietPID);
    ui->StatusBar->showMessage("Wysłano nastawy układu (PID) [Auto]", 2000);
}

void MainWindow::sendARXConfig() {
    if (klient == nullptr) return;

    ModelARX pakietARX;

    auto toVec = [](const QString& s) {
        std::vector<double> v;
        QStringList list = s.split(',', Qt::SkipEmptyParts);
        for(const QString& item : list) {
            v.push_back(item.trimmed().toDouble());
        }
        return v;
    };
    pakietARX.setParams(toVec(m_curA), toVec(m_curB), m_curK);
    pakietARX.setLimity(m_curMinU, m_curMaxU, m_curMinY, m_curMaxY, m_curLimitsOn);
    pakietARX.setSzum(m_curNoise);

    klient->sendConf(KONF_ARX, pakietARX);
    ui->StatusBar->showMessage("Wysłano parametry obiektu ARX [Auto]", 2000);
}

void MainWindow::on_btnSiec_clicked()
{
    Dialogsiec oknoSieci(this);
    if(oknoSieci.exec() == QDialog::Accepted){
        bool toKlient = oknoSieci.czyKlient();
        int port = oknoSieci.getPort();
        QString ip = oknoSieci.getIP();
        if(toKlient){
            klient = new klientTCP(this);
            connect(klient, &klientTCP::polaczonoZSerwerem, this, [this](){
                QString ipSerwera = klient->pobierzIP();
                ui->statusLabel->setText("STATUS: POŁĄCZONO Z SERWEREM (" + ipSerwera + ")");
                ui->statusLabel->setStyleSheet("background-color: #28a745; color: white; font-weight: bold; padding: 5px; border-radius: 3px;");
                zarzadzajKontrolkami(true, true);
            });
            connect(klient, &klientTCP::rozlaczonoZSerwerem, this, [this](){
                ui->statusLabel->setText("STATUS: BŁĄD POŁĄCZENIA/ROZŁĄCZONO");
                ui->statusLabel->setStyleSheet("background-color: #dc3545; color: white; font-weight: bold; padding: 5px; border-radius: 3px;");
                zarzadzajKontrolkami(false, true);
            });
            connect(klient, &klientTCP::otrzymanoNowyPID, this, &MainWindow::odbierzPID);
            connect(klient, &klientTCP::otrzymanoNowyGen, this, &MainWindow::odbierzGen);
            klient->conToServ(ip,port);
        } else {
            serwer = new SerwerTCP(this);
            connect(serwer, &SerwerTCP::otrzymanoNowyARX, this, &MainWindow::odbierzARX);
            serwer->startListening(port);
            ui->statusLabel->setText("STATUS: NASŁUCHIWANIE NA PORCIE " + QString::number(port));
            ui->statusLabel->setStyleSheet("background-color: #007bff; color: white;");
            connect(serwer, &SerwerTCP::klientPodlaczony, this, [this](){
                QString ipKlienta = serwer->pobierzIP();
                ui->statusLabel->setText("STATUS: POŁĄCZONO Z KLIENTEM (" + ipKlienta + ")");
                ui->statusLabel->setStyleSheet("background-color: #28a745; color: white; font-weight: bold; padding: 5px; border-radius: 3px;");
                zarzadzajKontrolkami(true, false);
            });
            connect(serwer, &SerwerTCP::klientRozlaczony, this, [this](){
                ui->statusLabel->setText("STATUS: BŁĄD POŁĄCZENIA/ROZŁĄCZONO");
                ui->statusLabel->setStyleSheet("background-color: #dc3545; color: white; font-weight: bold; padding: 5px; border-radius: 3px;");
                zarzadzajKontrolkami(false, false);
            });
        }
    } else {

        qDebug() << "Odrzucono ustawienia sieciowe";
        ui->statusLabel->setText("STATUS: TRYB LOKALNY");
        ui->statusLabel->setStyleSheet("background-color: #007bff; color: white; font-weight: bold; padding: 5px; border-radius: 3px;");
        zarzadzajKontrolkami(false, false);
    }
}


void MainWindow::on_btnRozlacz_clicked()
{
    QMessageBox::StandardButton odpowiedz;
    odpowiedz = QMessageBox::warning(this,"Ostrzeżenie", "Czy na pewno chcesz rozłączyć?", QMessageBox::Yes | QMessageBox::No);
    if(odpowiedz == QMessageBox::Yes){
        if (klient != nullptr){
            klient->rozlacz();
            klient->deleteLater();
            klient = nullptr;
            qDebug() << "Zabito klienta";
        }
        if (serwer != nullptr){
            serwer->zatrzymaj();
            serwer->deleteLater();
            serwer = nullptr;
            qDebug() << "Zabito serwer";
        }
    ui->statusLabel->setText("STATUS: TRYB LOKALNY");
    ui->statusLabel->setStyleSheet("background-color: #007bff; color: white; font-weight: bold; padding: 5px; border-radius: 3px;");
    zarzadzajKontrolkami(false, false);
    }
}

void MainWindow::odbierzPID(RegulatorPID pid) {

    m_pidK = pid.getK();
    m_pidTi = pid.getTi();
    m_pidTd = pid.getTd();
    m_pidMethod = pid.getMethod();

    updatePIDUI();

    updateParameters();

    ui->StatusBar->showMessage("Otrzymano i zaktualizowano: NASTAWY PID", 3000);
}

void MainWindow::odbierzARX(ModelARX arx) {

    auto fromVec = [](const std::vector<double>& v) {
        QStringList list;
        for(double val : v) {
            list << QString::number(val);
        }
        return list.join(", ");
    };

    m_curA = fromVec(arx.getA());
    m_curB = fromVec(arx.getB());
    m_curK = arx.getK();

    m_curMinU = arx.getMinU();
    m_curMaxU = arx.getMaxU();
    m_curMinY = arx.getMinY();
    m_curMaxY = arx.getMaxY();
    m_curNoise = arx.getNoise();
    m_curLimitsOn = arx.getLimitsOn();
    if (oknoARX != nullptr) {
        oknoARX->setData(m_curA, m_curB, m_curK,
        m_curMinU, m_curMaxU, m_curMinY, m_curMaxY,
        m_curNoise, m_curLimitsOn);
    }

    pushARXParamsToService();

    ui->StatusBar->showMessage("Otrzymano i zaktualizowano: PARAMETRY ARX", 3000);
}

void MainWindow::odbierzGen(GeneratorWartosci gen) {
    m_genTryb = gen.getTryb();
    m_genOkresRzecz = gen.getOkresRzecz();
    m_genAmplituda = gen.getAmplituda();
    m_genOffset = gen.getOffset();
    m_genWypelnienie = gen.getWypelnienie();

    updateGenUI();

    updateParameters();

    ui->StatusBar->showMessage("Otrzymano i zaktualizowano: PARAMETRY GENERATORA", 3000);
}

void MainWindow::updatePIDUI() {
    // Hermetyzacja aktualizacji UI. Blokowanie sygnałów w jednym miejscu.
    const QSignalBlocker blockerK(ui->spinK);
    ui->spinK->setValue(m_pidK);

    const QSignalBlocker blockerTi(ui->spinTi);
    ui->spinTi->setValue(m_pidTi);

    const QSignalBlocker blockerTd(ui->spinTd);
    ui->spinTd->setValue(m_pidTd);

    const QSignalBlocker blockerMethod(ui->comboPIDMethod);
    ui->comboPIDMethod->setCurrentIndex(static_cast<int>(m_pidMethod));
}

void MainWindow::updateGenUI() {
    // Hermetyzacja aktualizacji UI generatora. Blokowanie sygnałów w jednym miejscu.

    const QSignalBlocker blockerTryb(ui->comboGenType);
    ui->comboGenType->setCurrentIndex(static_cast<int>(m_genTryb));

    const QSignalBlocker blockerOkres(ui->spinOkres);
    ui->spinOkres->setValue(m_genOkresRzecz);

    const QSignalBlocker blockerAmpl(ui->spinAmp);
    ui->spinAmp->setValue(m_genAmplituda);

    const QSignalBlocker blockerOff(ui->spinOffset);
    ui->spinOffset->setValue(m_genOffset);

    const QSignalBlocker blockerWyp(ui->spinFill);
    ui->spinFill->setValue(m_genWypelnienie);
}
