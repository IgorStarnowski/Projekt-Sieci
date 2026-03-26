#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_service(new UARService(this))
{
    ui->setupUi(this);

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

    // Automatyczna aktualizacja parametrów
    QList<QDoubleSpinBox*> spins = this->findChildren<QDoubleSpinBox*>();
    for(auto spin : spins) {
        connect(spin, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateParameters);
    }

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
    DialogARX dlg(this);

    dlg.setData(m_curA, m_curB, m_curK,
                m_curMinU, m_curMaxU, m_curMinY, m_curMaxY, m_curNoise,
                m_curLimitsOn);

    if (dlg.exec() == QDialog::Accepted) {
        m_curA = dlg.getA();
        m_curB = dlg.getB();
        m_curK = dlg.getK();

        m_curMinU = dlg.getMinU();
        m_curMaxU = dlg.getMaxU();
        m_curMinY = dlg.getMinY();
        m_curMaxY = dlg.getMaxY();
        m_curNoise = dlg.getNoise();
        m_curLimitsOn = dlg.getLimityWlaczone();

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
