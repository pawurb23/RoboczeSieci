#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "arx.h"
#include <QtCharts/QLineSeries>
#include <cmath>
#include <QFileDialog>
#include <QMessageBox>
#include "ConfigManager.h"
#include "Kontroler.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QButtonGroup>

// Konstruktor
MainWindow::MainWindow(bool regulatorMode, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , isRegulatorMode(regulatorMode)
    , lastReceivedY(0.0)
{
    ui->setupUi(this);
    this->setWindowTitle(regulatorMode ? "Symulator UAR - REGULATOR" : "Symulator UAR - OBIEKT");

    kontroler = new Kontroler();

    // Domyślne parametry modelu
    if(kontroler && kontroler->get_uar() && kontroler->get_uar()->get_model()) {
        auto* model = kontroler->get_uar()->get_model();
        model->set_wspolczynniki_a({-0.4});
        model->set_wspolczynniki_b({0.6});
        model->set_opoznienie(1);
        model->set_odchylenie_standardowe(0.0);
    }

    updateControllerParams();

    // Grupowanie przycisków
    groupRegulator = new QButtonGroup(this);
    groupRegulator->addButton(ui->ONOFF_radioButton);
    groupRegulator->addButton(ui->PID_radioButton);

    groupIntegral = new QButtonGroup(this);
    groupIntegral->addButton(ui->przed_radioButton);
    groupIntegral->addButton(ui->w_sumie_radioButton);

    groupSignal = new QButtonGroup(this);
    groupSignal->addButton(ui->sqrt_syg_radioButton);
    groupSignal->addButton(ui->sin_syg_radioButton);

    ui->fill_square_doubleSpinBox_2->setRange(0, 100);
    is_running = false;
    time_step = 0.1;

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateSimulation);

    ui->sterowanie_doubleSpinBox_5->setValue(-10.0);
    ui->sterowanie_doubleSpinBox_6->setValue(10.0);

    //Tworzenie wykresów
    series_zadana = new QLineSeries(); series_zadana->setName("Zadana");
    series_regulowana = new QLineSeries(); series_regulowana->setName("Regulowana");
    QChart *chart1 = new QChart();
    chart1->addSeries(series_zadana); chart1->addSeries(series_regulowana);
    chart1->createDefaultAxes();
    chart1->axes(Qt::Horizontal).first()->setRange(0.0, ui->szer_okna_spinBox_2->value());
    chart1->axes(Qt::Vertical).first()->setRange(-10, 10);
    chart1->setTitle("Wartość zadana i regulowana");
    QChartView *view1 = new QChartView(chart1);
    view1->setRenderHint(QPainter::Antialiasing);
    ui->verticalLayout->addWidget(view1);

    series_uchyb = new QLineSeries();
    QChart *chart2 = new QChart();
    chart2->addSeries(series_uchyb);
    chart2->createDefaultAxes();
    chart2->axes(Qt::Horizontal).first()->setRange(0.0, ui->szer_okna_spinBox_2->value());
    chart2->axes(Qt::Vertical).first()->setRange(-10, 10);
    chart2->setTitle("Uchyb");
    QChartView *view2 = new QChartView(chart2);
    ui->horizontalLayout->addWidget(view2);

    series_sterowanie = new QLineSeries();
    QChart *chart3 = new QChart();
    chart3->addSeries(series_sterowanie);
    chart3->createDefaultAxes();
    chart3->axes(Qt::Horizontal).first()->setRange(0.0, ui->szer_okna_spinBox_2->value());
    chart3->axes(Qt::Vertical).first()->setRange(-10, 10);
    chart3->setTitle("Sterowanie");
    QChartView *view3 = new QChartView(chart3);
    ui->horizontalLayout->addWidget(view3);

    series_P = new QLineSeries(); series_P->setName("Proporcjonalny"); series_P->setColor(Qt::red);
    series_I = new QLineSeries(); series_I->setName("Całkujący"); series_I->setColor(Qt::green);
    series_D = new QLineSeries(); series_D->setName("Różniczkujący"); series_D->setColor(Qt::blue);
    QChart *chart4 = new QChart();
    chart4->addSeries(series_P); chart4->addSeries(series_I); chart4->addSeries(series_D);
    chart4->createDefaultAxes();
    chart4->axes(Qt::Horizontal).first()->setRange(0, ui->szer_okna_spinBox_2->value());
    chart4->axes(Qt::Vertical).first()->setRange(-10, 10);
    chart4->setTitle("Składowe sterowania PID");
    QChartView *view4 = new QChartView(chart4);
    ui->horizontalLayout->addWidget(view4);

    //Połączenia sygnałów
    connect(ui->StartStop_button_2, &QPushButton::clicked, this, &MainWindow::on_start_stop_clicked);
    connect(ui->reset_button_2, &QPushButton::clicked, this, &MainWindow::on_reset_clicked);
    ui->przed_radioButton->setChecked(true);

    // Połączenia dla aktualizacji parametrów
    connect(ui->amp_square_doubleSpinBox_2, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->period_square_doubleSpinBox_2, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->sklad_stal_sqrt_doubleSpinBox, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->fill_square_doubleSpinBox_2, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->doubleSpinBox_8, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->wzmocnienie_doubleSpinBox_3, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->calka_doubleSpinBox_3, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->rozniczka_doubleSpinBox_3, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->histereza_doubleSpinBox_3, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->sterowanie_doubleSpinBox_5, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->sterowanie_doubleSpinBox_6, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->doubleSpinBox_6, &QDoubleSpinBox::editingFinished, this, &MainWindow::updateControllerParams);
    connect(ui->antiwindup_checkBox_3, &QCheckBox::clicked, this, &MainWindow::updateControllerParams);
    connect(ui->checkBox_3, &QCheckBox::clicked, this, &MainWindow::updateControllerParams);
    connect(groupRegulator, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &MainWindow::updateControllerParams);
    connect(groupIntegral, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &MainWindow::updateControllerParams);
    connect(groupSignal, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &MainWindow::updateControllerParams);
    connect(ui->memory_reset_d_button_2, &QPushButton::clicked, this, &MainWindow::updateControllerParams);
    connect(ui->memory_reset_i_button_2, &QPushButton::clicked, this, &MainWindow::updateControllerParams);
    connect(ui->del_syg_button, &QPushButton::clicked, this, &MainWindow::updateControllerParams);
    connect(ui->interwal_doubleSpinBox_2, &QDoubleSpinBox::editingFinished, this, [this](){
        if(is_running) {
            double interval = ui->interwal_doubleSpinBox_2->value();
            time_step = interval / 1000.0;
            timer->setInterval(interval);
        }
    });

    //Inicjalizacja sieci
    klient = new myTCPclient(this);
    serwer = new myTCPserwer(this);
    stats = new PacketStats(this);

    // Połączenia sygnałów dla przycisków (UI)
    connect(ui->btnPolacz, &QPushButton::clicked, this, &MainWindow::on_btnPolacz_clicked);
    connect(ui->btnSerwer, &QPushButton::clicked, this, &MainWindow::on_btnSerwer_clicked);
    connect(ui->btnRozlacz, &QPushButton::clicked, this, &MainWindow::on_btnRozlacz_clicked);

    // Połączenia stanów połączenia
    connect(klient, &myTCPclient::polaczono, this, &MainWindow::onTcpConnected);
    connect(serwer, &myTCPserwer::klientPodlaczony, this, &MainWindow::onTcpConnected);
    connect(klient, &myTCPclient::rozlaczono, this, &MainWindow::onTcpDisconnected);
    connect(serwer, &myTCPserwer::klientOdlaczony, this, &MainWindow::onTcpDisconnected);

    // Połączenia odbioru danych (osobne dla klienta i serwera)
    connect(klient, &myTCPclient::odebranoDane, this, &MainWindow::onClientDataReceived);
    connect(serwer, &myTCPserwer::odebranoDane, this, &MainWindow::onServerDataReceived);

    // Połączenie statystyk
    connect(stats, &PacketStats::statsUpdated, this, &MainWindow::updateNetworkStats);

    // Ustawienie domyślnych wartości w UI
    ui->poleIP->setText("127.0.0.1");
    ui->polePort->setValue(5555);
    ustawStanRozlaczony();

    // Inicjalizacja zmiennych dla trybu sieciowego
    currentPacketId = 0;
    lastReceivedY = 0.0;
    networkTimer = new QTimer(this);
    connect(networkTimer, &QTimer::timeout, this, &MainWindow::onNetworkTimer);

    // Konfiguracja trybu (regulator/obiekt)
    if (isRegulatorMode)
        setupRegulatorMode();
    else
        setupObiektMode();

    updateControllerParams();
}

MainWindow::~MainWindow()
{
    delete kontroler;
    delete ui;
}

//Metody dla modelu ARX
void MainWindow::on_ARX_model_button_clicked()
{
    ARX *secWindow = new ARX(this);
    if(kontroler && kontroler->get_uar() && kontroler->get_uar()->get_model()) {
        secWindow->set_parametry(
            kontroler->get_uar()->get_model()->get_wspolczynniki_a(),
            kontroler->get_uar()->get_model()->get_wspolczynniki_b(),
            kontroler->get_uar()->get_model()->get_opoznienie(),
            kontroler->get_uar()->get_model()->get_odchylenie()
            );
    }
    connect(secWindow, &ARX::s_update_model_params, this, &MainWindow::on_model_update);
    secWindow->show();
}

void MainWindow::on_model_update(std::vector<double> A, std::vector<double> B, int k, double szum)
{
    if(kontroler && kontroler->get_uar() && kontroler->get_uar()->get_model()) {
        kontroler->get_uar()->get_model()->set_wspolczynniki_a(A);
        kontroler->get_uar()->get_model()->set_wspolczynniki_b(B);
        kontroler->get_uar()->get_model()->set_opoznienie(k);
        kontroler->get_uar()->get_model()->set_odchylenie_standardowe(szum);
    }
}

//Sterowanie symulacją offline
void MainWindow::on_start_stop_clicked()
{
    if(is_running) {
        timer->stop();
        is_running = false;
    } else {
        if (series_zadana->count() == 0) {
            double t0 = 0.0;
            series_zadana->append(t0, 0.0);
            series_regulowana->append(t0, 0.0);
            series_uchyb->append(t0, 0.0);
            series_sterowanie->append(t0, 0.0);
            series_P->append(t0, 0.0);
            series_I->append(t0, 0.0);
            series_D->append(t0, 0.0);
        }
        double intervalMs = ui->interwal_doubleSpinBox_2->value();
        time_step = intervalMs / 1000.0;
        timer->start(intervalMs);
        is_running = true;
    }
}

void MainWindow::on_reset_clicked()
{
    is_running = false;
    timer->stop();
    if(kontroler) kontroler->reset();
    series_zadana->clear();
    series_regulowana->clear();
    series_uchyb->clear();
    series_sterowanie->clear();
    series_P->clear();
    series_I->clear();
    series_D->clear();
}

void MainWindow::updateSimulation()
{
    if(!kontroler) return;
    kontroler->symuluj_krok(time_step);
    double setpoint = kontroler->get_wartosc_zadana();
    double y = kontroler->get_wyjscie();
    double e = kontroler->get_uchyb();
    double u = kontroler->get_sterowanie();
    double sk_p = kontroler->get_skladowa_p();
    double sk_i = kontroler->get_skladowa_i();
    double sk_d = kontroler->get_skladowa_d();
    updateCharts(setpoint, y, e, u, sk_p, sk_i, sk_d);
}

//Aktualizacja parametrów
void MainWindow::updateControllerParams()
{
    if(!kontroler) return;
    ProstyUAR* uar = kontroler->get_uar();
    Generator* gen = kontroler->get_generator();

    if(ui->sin_syg_radioButton->isChecked()) {
        gen->ustaw_typ(Generator::SINUS);
        gen->ustaw_parametry_sinus(
            ui->amp_sinus_doubleSpinBox_2->value(),
            ui->period_sinus_doubleSpinBox_2->value(),
            ui->sklad_stal_sin_doubleSpinBox->value()
            );
        gen->set_czas_aktywacji(ui->doubleSpinBox_8->value());
    } else if(ui->sqrt_syg_radioButton->isChecked()) {
        gen->ustaw_typ(Generator::PROSTOKAT);
        gen->ustaw_parametry_prostokat(
            ui->amp_square_doubleSpinBox_2->value(),
            ui->period_square_doubleSpinBox_2->value(),
            ui->sklad_stal_sqrt_doubleSpinBox->value(),
            ui->fill_square_doubleSpinBox_2->value()
            );
        gen->set_czas_aktywacji(ui->doubleSpinBox_8->value());
    } else {
        gen->ustaw_typ(Generator::BRAK);
    }

    if(ui->PID_radioButton->isChecked()) {
        uar->ustaw_aktywny_regulator(ProstyUAR::TypRegulatora::PID);
        double k = ui->wzmocnienie_doubleSpinBox_3->value();
        double ti = ui->calka_doubleSpinBox_3->value();
        double td = ui->rozniczka_doubleSpinBox_3->value();
        bool antiwindup = ui->antiwindup_checkBox_3->isChecked();
        double u_min = ui->sterowanie_doubleSpinBox_5->value();
        double u_max = ui->sterowanie_doubleSpinBox_6->value();
        RegulatorPID* pid = uar->get_pid();
        if(pid) {
            pid->set_nastawy(k, ti, td);
            if(ui->checkBox_3->isChecked())
                pid->set_ograniczenia(u_min, u_max);
            else
                pid->set_ograniczenia(-10.0, 10.0);
            pid->set_anti_windup(antiwindup);
            if(ui->przed_radioButton->isChecked())
                pid->set_tryb_calki(RegulatorPID::tryb_calki::stala_przed_suma);
            else if(ui->w_sumie_radioButton->isChecked())
                pid->set_tryb_calki(RegulatorPID::tryb_calki::StalaPodSuma);
        }
    } else if(ui->ONOFF_radioButton->isChecked()) {
        uar->ustaw_aktywny_regulator(ProstyUAR::TypRegulatora::ONOFF);
        double u_on = ui->doubleSpinBox_6->value();
        double hyst = ui->histereza_doubleSpinBox_3->value();
        RegulatorONOFF* onoff = uar->get_onoff();
        if(onoff) onoff->set_nastawy(hyst, u_on);
    } else {
        uar->ustaw_aktywny_regulator(ProstyUAR::TypRegulatora::BRAK);
    }

    double u_min = ui->sterowanie_doubleSpinBox_5->value();
    double u_max = ui->sterowanie_doubleSpinBox_6->value();
    if(uar->get_model()) {
        uar->get_model()->set_u_min(u_min);
        uar->get_model()->set_u_max(u_max);
    }
}

//Zapis / odczyt konfiguracji
void MainWindow::on_save_button_2_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Wybierz katalog zapisu", ".",
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(dir.isEmpty()) return;
    ConfigData config;
    config.main.interwal_ms = ui->interwal_doubleSpinBox_2->value();
    config.main.szerokosc_okna = ui->szer_okna_spinBox_2->value();
    if(ui->PID_radioButton->isChecked()) config.regulator.typ = "PID";
    else if(ui->ONOFF_radioButton->isChecked()) config.regulator.typ = "ON_OFF";
    else config.regulator.typ = "Nieznany";
    config.regulator.onoff.u_on = ui->doubleSpinBox_6->value();
    config.regulator.onoff.histereza = ui->histereza_doubleSpinBox_3->value();
    config.regulator.pid.k = ui->wzmocnienie_doubleSpinBox_3->value();
    config.regulator.pid.ti = ui->calka_doubleSpinBox_3->value();
    config.regulator.pid.td = ui->rozniczka_doubleSpinBox_3->value();
    config.regulator.pid.antiwindup = ui->antiwindup_checkBox_3->isChecked();
    config.regulator.pid.limits.active = ui->checkBox_3->isChecked();
    config.regulator.pid.limits.min = ui->sterowanie_doubleSpinBox_5->value();
    config.regulator.pid.limits.max = ui->sterowanie_doubleSpinBox_6->value();
    if(ui->przed_radioButton->isChecked()) config.regulator.pid.tryb_calki = "stala_przed_suma";
    else if(ui->w_sumie_radioButton->isChecked()) config.regulator.pid.tryb_calki = "stala_w_sumie";
    else config.regulator.pid.tryb_calki = "nieznany";
    ProstyUAR* uar = kontroler ? kontroler->get_uar() : nullptr;
    if(uar && uar->get_model()) {
        config.model.A = uar->get_model()->get_wspolczynniki_a();
        config.model.B = uar->get_model()->get_wspolczynniki_b();
        config.model.opoznienie = uar->get_model()->get_opoznienie();
        config.model.szumy = uar->get_model()->get_odchylenie();
    }
    if(ui->sin_syg_radioButton->isChecked()) config.signal.typ = "sinusoidalny";
    else config.signal.typ = "prostokatny";
    config.signal.rect.amp = ui->amp_square_doubleSpinBox_2->value();
    config.signal.rect.period = ui->period_square_doubleSpinBox_2->value();
    config.signal.rect.offset = ui->sklad_stal_sqrt_doubleSpinBox->value();
    config.signal.rect.fill = ui->fill_square_doubleSpinBox_2->value();
    config.signal.rect.activation_time = ui->doubleSpinBox_8->value();
    config.signal.sin.amp = ui->amp_sinus_doubleSpinBox_2->value();
    config.signal.sin.period = ui->period_sinus_doubleSpinBox_2->value();
    config.signal.sin.offset = ui->sklad_stal_sin_doubleSpinBox->value();
    config.signal.sin.activation_time = ui->doubleSpinBox_8->value();
    if(ConfigManager::zapiszKonfiguracje(config, dir))
        QMessageBox::information(this, "Sukces", "Konfiguracja zapisana pomyslnie!");
    else
        QMessageBox::critical(this, "Blad", "Nie udalo sie zapisac konfiguracji.");
}

void MainWindow::on_read_button_2_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Wybierz plik konfiguracji", ".", "Pliki JSON (*.json)");
    if(fileName.isEmpty()) return;
    ConfigData config = ConfigManager::wczytajKonfiguracje(fileName);
    ui->interwal_doubleSpinBox_2->setValue(config.main.interwal_ms);
    ui->szer_okna_spinBox_2->setValue(config.main.szerokosc_okna);
    if(config.regulator.typ == "PID") ui->PID_radioButton->setChecked(true);
    else if(config.regulator.typ == "ON_OFF") ui->ONOFF_radioButton->setChecked(true);
    ui->doubleSpinBox_6->setValue(config.regulator.onoff.u_on);
    ui->histereza_doubleSpinBox_3->setValue(config.regulator.onoff.histereza);
    ui->wzmocnienie_doubleSpinBox_3->setValue(config.regulator.pid.k);
    ui->calka_doubleSpinBox_3->setValue(config.regulator.pid.ti);
    ui->rozniczka_doubleSpinBox_3->setValue(config.regulator.pid.td);
    ui->antiwindup_checkBox_3->setChecked(config.regulator.pid.antiwindup);
    ui->checkBox_3->setChecked(config.regulator.pid.limits.active);
    ui->sterowanie_doubleSpinBox_5->setValue(config.regulator.pid.limits.min);
    ui->sterowanie_doubleSpinBox_6->setValue(config.regulator.pid.limits.max);
    if(config.regulator.pid.tryb_calki == "stala_przed_suma") ui->przed_radioButton->setChecked(true);
    else if(config.regulator.pid.tryb_calki == "stala_w_sumie") ui->w_sumie_radioButton->setChecked(true);
    if(kontroler && kontroler->get_uar() && kontroler->get_uar()->get_model()) {
        kontroler->get_uar()->get_model()->set_wspolczynniki_a(config.model.A);
        kontroler->get_uar()->get_model()->set_wspolczynniki_b(config.model.B);
        kontroler->get_uar()->get_model()->set_odchylenie_standardowe(config.model.szumy);
    }
    if(config.signal.typ == "sinusoidalny") ui->sin_syg_radioButton->setChecked(true);
    else ui->sqrt_syg_radioButton->setChecked(true);
    ui->amp_square_doubleSpinBox_2->setValue(config.signal.rect.amp);
    ui->period_square_doubleSpinBox_2->setValue(config.signal.rect.period);
    ui->sklad_stal_sqrt_doubleSpinBox->setValue(config.signal.rect.offset);
    ui->fill_square_doubleSpinBox_2->setValue(config.signal.rect.fill);
    ui->amp_sinus_doubleSpinBox_2->setValue(config.signal.sin.amp);
    ui->period_sinus_doubleSpinBox_2->setValue(config.signal.sin.period);
    ui->sklad_stal_sin_doubleSpinBox->setValue(config.signal.sin.offset);
    if(config.signal.typ == "sinusoidalny")
        ui->doubleSpinBox_8->setValue(config.signal.sin.activation_time);
    else
        ui->doubleSpinBox_8->setValue(config.signal.rect.activation_time);
    QMessageBox::information(this, "Sukces", "Konfiguracja wczytana pomyslnie!");
    updateControllerParams();
}

//Sygnały
void MainWindow::on_add_syg_button_clicked()
{
    if(ui->sin_syg_radioButton->isChecked()){
        double amp=ui->amp_sinus_doubleSpinBox_2->value();
        double okres=ui->period_sinus_doubleSpinBox_2->value();
        double skladowa=ui->sklad_stal_sin_doubleSpinBox->value();
        QString sin_opis = QString("Amp=%1, T=%2, Stała=%3").arg(amp).arg(okres).arg(skladowa);
        ui->sygn_sinus_l_2->append(sin_opis);
    } else if(ui->sqrt_syg_radioButton->isChecked()){
        double amp=ui->amp_square_doubleSpinBox_2->value();
        double okres=ui->period_square_doubleSpinBox_2->value();
        double skladowa=ui->sklad_stal_sqrt_doubleSpinBox->value();
        double fill =ui->fill_square_doubleSpinBox_2->value();
        QString sqrt_opis = QString("Amp=%1, T=%2, Stała=%3, Wyp=%4").arg(amp).arg(okres).arg(skladowa).arg(fill);
        ui->sygn_square_l_2->append(sqrt_opis);
    }
}

void MainWindow::on_del_syg_button_clicked()
{
    if(ui->sin_syg_radioButton->isChecked()){
        ui->amp_sinus_doubleSpinBox_2->setValue(0.0);
        ui->period_sinus_doubleSpinBox_2->setValue(0.0);
        ui->sklad_stal_sin_doubleSpinBox->setValue(0.0);
    } else if(ui->sqrt_syg_radioButton->isChecked()){
        ui->amp_square_doubleSpinBox_2->setValue(0.0);
        ui->sklad_stal_sqrt_doubleSpinBox->setValue(0.0);
        ui->fill_square_doubleSpinBox_2->setValue(0.5);
        ui->period_square_doubleSpinBox_2->setValue(0.0);
    }
}

//Metody dla przycisków sieciowych
void MainWindow::on_btnPolacz_clicked()
{
    klient->polaczZSerwerem(ui->poleIP->text(), ui->polePort->value());
}

void MainWindow::on_btnSerwer_clicked()
{
    if(serwer->uruchomSerwer(ui->polePort->value())) {
        ui->btnSerwer->setEnabled(false);
        ui->btnPolacz->setEnabled(false);
        ui->lblStatus->setText("Status: OCZEKIWANIE...");
        ui->lblStatus->setStyleSheet("color: orange; font-weight: bold;");
    }
}

void MainWindow::on_btnRozlacz_clicked()
{
    klient->rozlaczZSerwerem();
    serwer->zatrzymajSerwer();
    ustawStanRozlaczony();
}

//Stany połączenia
void MainWindow::ustawStanPolaczony()
{
    ui->poleIP->setEnabled(false);
    ui->polePort->setEnabled(false);
    ui->btnPolacz->setEnabled(false);
    ui->btnSerwer->setEnabled(false);
    ui->btnRozlacz->setEnabled(true);
    ui->lblStatus->setText("Status: POLACZONO");
    ui->lblStatus->setStyleSheet("color: green; font-weight: bold;");
    zablokujKontrolkiSymulacji(false);
}

void MainWindow::ustawStanRozlaczony()
{
    ui->poleIP->setEnabled(true);
    ui->polePort->setEnabled(true);
    ui->btnPolacz->setEnabled(true);
    ui->btnSerwer->setEnabled(true);
    ui->btnRozlacz->setEnabled(false);
    ui->lblStatus->setText("Status: ROZLACZONO");
    ui->lblStatus->setStyleSheet("color: red; font-weight: bold;");
    zablokujKontrolkiSymulacji(true);
}

void MainWindow::zablokujKontrolkiSymulacji(bool zablokowane)
{
    (void)zablokowane;
    // Tu można zablokować elementy GUI w zależności od trybu
}

void MainWindow::onTcpConnected()
{
    ustawStanPolaczony();
    if (isRegulatorMode) {
        sendConfig();
        double intervalMs = ui->interwal_doubleSpinBox_2->value();
        networkTimer->start(intervalMs);
    }
}

void MainWindow::onTcpDisconnected()
{
    ustawStanRozlaczony();
    if (networkTimer) networkTimer->stop();
    pendingPackets.clear();
}

//Odbiór danych od klienta
void MainWindow::onClientDataReceived(quint8 typ, QByteArray data)
{
    if (typ == PKT_OUTPUT) {
        int id; double y;
        if (OutputPacket::fromJson(data, id, y)) {
            if (pendingPackets.contains(id)) {
                qint64 latency = pendingPackets[id].msecsTo(QDateTime::currentDateTime());
                stats->recordLatency(latency);
                pendingPackets.remove(id);
            }
            stats->packetReceived();
            lastReceivedY = y;

            double czas = kontroler->get_aktualny_czas();
            double setpoint = kontroler->get_generator()->oblicz_wartosc(czas);
            double error = setpoint - y;
            updateCharts(setpoint, y, error, 0.0, 0.0, 0.0, 0.0);
            kontroler->symuluj_krok(time_step);
        }
    } else if (typ == PKT_CONFIG) {
        ConfigData cfg = ConfigManager::deserializacja(data);
    }
}

//Odbiór danych od serwera
void MainWindow::onServerDataReceived(quint8 typ, QByteArray data)
{
    if (typ == PKT_CONTROL) {
        int id; double u;
        if (ControlPacket::fromJson(data, id, u)) {
            double y = kontroler->get_uar()->get_model()->symuluj(u);
            QByteArray outData = OutputPacket::toJson(id, y);
            serwer->wyslijDane(PKT_OUTPUT, outData);
            stats->packetSent();

            updateCharts(0.0, y, 0.0, u, 0.0, 0.0, 0.0);
            kontroler->symuluj_krok(time_step);
        }
    } else if (typ == PKT_CONFIG) {
        ConfigData cfg = ConfigManager::deserializacja(data);
        if (kontroler && kontroler->get_uar() && kontroler->get_uar()->get_model()) {
            kontroler->get_uar()->get_model()->set_wspolczynniki_a(cfg.model.A);
            kontroler->get_uar()->get_model()->set_wspolczynniki_b(cfg.model.B);
            kontroler->get_uar()->get_model()->set_opoznienie(cfg.model.opoznienie);
        }
    }
}

//Timer symulacji dla regulatora
void MainWindow::onNetworkTimer()
{
    if (!isRegulatorMode) return;
    if (!klient->isConnected()) return;

    double czas = kontroler->get_aktualny_czas();
    double setpoint = kontroler->get_generator()->oblicz_wartosc(czas);
    double error = setpoint - lastReceivedY;

    double u = 0.0;
    if (ui->PID_radioButton->isChecked()) {
        u = kontroler->get_uar()->get_pid()->symuluj(error);
    } else if (ui->ONOFF_radioButton->isChecked()) {
        u = kontroler->get_uar()->get_onoff()->symuluj(error);
    }

    sendControl(u);
    pendingPackets[currentPacketId] = QDateTime::currentDateTime();
    stats->packetSent();

    QTimer::singleShot(500, this, [this, id = currentPacketId]() {
        if (pendingPackets.contains(id)) {
            pendingPackets.remove(id);
        }
    });

    currentPacketId++;
}

void MainWindow::sendControl(double u)
{
    QByteArray data = ControlPacket::toJson(currentPacketId, u);
    klient->wyslijDane(PKT_CONTROL, data);
}

//Wysyłanie konfiguracji
void MainWindow::sendConfig()
{
    ConfigData cfg;
    cfg.main.interwal_ms = ui->interwal_doubleSpinBox_2->value();
    cfg.main.szerokosc_okna = ui->szer_okna_spinBox_2->value();
    if(ui->PID_radioButton->isChecked()) cfg.regulator.typ = "PID";
    else if(ui->ONOFF_radioButton->isChecked()) cfg.regulator.typ = "ON_OFF";
    cfg.regulator.pid.k = ui->wzmocnienie_doubleSpinBox_3->value();
    cfg.regulator.pid.ti = ui->calka_doubleSpinBox_3->value();
    cfg.regulator.pid.td = ui->rozniczka_doubleSpinBox_3->value();
    cfg.regulator.pid.antiwindup = ui->antiwindup_checkBox_3->isChecked();
    cfg.regulator.pid.limits.active = ui->checkBox_3->isChecked();
    cfg.regulator.pid.limits.min = ui->sterowanie_doubleSpinBox_5->value();
    cfg.regulator.pid.limits.max = ui->sterowanie_doubleSpinBox_6->value();
    if(ui->przed_radioButton->isChecked()) cfg.regulator.pid.tryb_calki = "stala_przed_suma";
    else cfg.regulator.pid.tryb_calki = "stala_w_sumie";
    ProstyUAR* uar = kontroler ? kontroler->get_uar() : nullptr;
    if(uar && uar->get_model()) {
        cfg.model.A = uar->get_model()->get_wspolczynniki_a();
        cfg.model.B = uar->get_model()->get_wspolczynniki_b();
        cfg.model.opoznienie = uar->get_model()->get_opoznienie();
    }
    if(ui->sin_syg_radioButton->isChecked()) cfg.signal.typ = "sinusoidalny";
    else cfg.signal.typ = "prostokatny";

    QByteArray json = ConfigManager::serializacja(cfg);
    if (isRegulatorMode && klient)
        klient->wyslijDane(PKT_CONFIG, json);
    else if (!isRegulatorMode && serwer)
        serwer->wyslijDane(PKT_CONFIG, json);
}

//Aktualizacja statystyk w UI
void MainWindow::updateNetworkStats(int pps, double avgLat, int lost)
{
    ui->lblPPS->setText(QString("Pakiety/s: %1").arg(pps));
    ui->lblLatency->setText(QString("Opóźnienie śr.: %1 ms").arg(avgLat, 0, 'f', 1));
    ui->lblLost->setText(QString("Straty: %1").arg(lost));
}

//Tryby pracy
void MainWindow::setupRegulatorMode()
{
    isRegulatorMode = true;
    ui->ARX_model_button->setEnabled(false);
    ui->btnPolacz->setEnabled(false);
}

void MainWindow::setupObiektMode()
{
    isRegulatorMode = false;
    ui->PID_radioButton->setEnabled(false);
    ui->ONOFF_radioButton->setEnabled(false);
    ui->wzmocnienie_doubleSpinBox_3->setEnabled(false);
    ui->calka_doubleSpinBox_3->setEnabled(false);
    ui->rozniczka_doubleSpinBox_3->setEnabled(false);
    ui->histereza_doubleSpinBox_3->setEnabled(false);
    ui->antiwindup_checkBox_3->setEnabled(false);
    ui->checkBox_3->setEnabled(false);
    ui->sterowanie_doubleSpinBox_5->setEnabled(false);
    ui->sterowanie_doubleSpinBox_6->setEnabled(false);
    ui->doubleSpinBox_6->setEnabled(false);
    ui->przed_radioButton->setEnabled(false);
    ui->w_sumie_radioButton->setEnabled(false);
    ui->sqrt_syg_radioButton->setEnabled(false);
    ui->sin_syg_radioButton->setEnabled(false);
    ui->amp_square_doubleSpinBox_2->setEnabled(false);
    ui->period_square_doubleSpinBox_2->setEnabled(false);
    ui->sklad_stal_sqrt_doubleSpinBox->setEnabled(false);
    ui->fill_square_doubleSpinBox_2->setEnabled(false);
    ui->amp_sinus_doubleSpinBox_2->setEnabled(false);
    ui->period_sinus_doubleSpinBox_2->setEnabled(false);
    ui->sklad_stal_sin_doubleSpinBox->setEnabled(false);
    ui->doubleSpinBox_8->setEnabled(false);
    ui->add_syg_button->setEnabled(false);
    ui->del_syg_button->setEnabled(false);
    ui->sygn_sinus_l_2->setEnabled(false);
    ui->sygn_square_l_2->setEnabled(false);
    ui->ARX_model_button->setEnabled(true);
    ui->btnSerwer->setEnabled(false);
}

//Wykresy
void MainWindow::updateCharts(double setpoint, double cv, double error, double control,
                              double p, double i, double d)
{
    double current_time = kontroler->get_aktualny_czas();
    series_zadana->append(current_time, setpoint);
    series_regulowana->append(current_time, cv);
    series_uchyb->append(current_time, error);
    series_sterowanie->append(current_time, control);
    series_P->append(current_time, p);
    series_I->append(current_time, i);
    series_D->append(current_time, d);

    double x_max_limit = ui->szer_okna_spinBox_2->value();
    double x_min = 0.0;
    double x_max = x_max_limit;
    if(current_time > x_max_limit) {
        x_max = current_time;
        x_min = current_time - x_max_limit;
    }

    auto cleanupSeries = [x_min](QLineSeries* s) {
        if (!s) return;
        while(!s->points().isEmpty() && s->points().first().x() < x_min)
            s->remove(0);
    };
    cleanupSeries(series_zadana);
    cleanupSeries(series_regulowana);
    cleanupSeries(series_uchyb);
    cleanupSeries(series_sterowanie);
    cleanupSeries(series_P);
    cleanupSeries(series_I);
    cleanupSeries(series_D);

    auto setAxisX = [x_min, x_max](QLineSeries* s){
        if(s->chart() && !s->chart()->axes(Qt::Horizontal).isEmpty())
            s->chart()->axes(Qt::Horizontal).first()->setRange(x_min, x_max);
    };
    setAxisX(series_zadana);
    setAxisX(series_uchyb);
    setAxisX(series_sterowanie);
    setAxisX(series_P);

    auto scaleChart = [](QList<QLineSeries*> seriesList) {
        if(seriesList.isEmpty()) return;
        double y_min = std::numeric_limits<double>::max();
        double y_max = std::numeric_limits<double>::lowest();
        for(QLineSeries* s : seriesList) {
            for(const QPointF &point : s->points()) {
                if(point.y() < y_min) y_min = point.y();
                if(point.y() > y_max) y_max = point.y();
            }
        }
        double margin = (y_max - y_min) * 0.1;
        if(margin == 0) margin = 1.0;
        y_min -= margin;
        y_max += margin;
        QChart* chart = seriesList.first()->chart();
        if(chart && !chart->axes(Qt::Vertical).isEmpty())
            chart->axes(Qt::Vertical).first()->setRange(y_min, y_max);
    };
    scaleChart({series_zadana, series_regulowana});
    scaleChart({series_uchyb});
    scaleChart({series_sterowanie});
    scaleChart({series_P, series_I, series_D});
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("PK", "ARX");
    settings.clear();
    event->accept();
}
