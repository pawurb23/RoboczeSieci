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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Symulator UAR");

    kontroler = new Kontroler();

    //Wpisanie parametrow do modelu ARX
    if(kontroler && kontroler->get_uar() && kontroler->get_uar()->get_model()) {
        auto* model = kontroler->get_uar()->get_model();
        model->set_wspolczynniki_a({-0.4});          // aktualnyWektorA
        model->set_wspolczynniki_b({0.6});           // aktualnyWektorB
        model->set_opoznienie(1);                    // aktualneOpoznienie
        model->set_odchylenie_standardowe(0.0);      // aktualnySzum
    }

    updateControllerParams();

    //grupowanie button
    QButtonGroup *g_reg=new QButtonGroup (this);
    g_reg->addButton(ui->ONOFF_radioButton);
    g_reg->addButton(ui->PID_radioButton);

    QButtonGroup *g_calka =new QButtonGroup(this);
    g_calka->addButton(ui->przed_radioButton);
    g_calka->addButton(ui->w_sumie_radioButton);

    QButtonGroup *g_syg =new QButtonGroup(this);
    g_syg->addButton(ui->sqrt_syg_radioButton);
    g_syg->addButton(ui->sin_syg_radioButton);

    ui->fill_square_doubleSpinBox_2->setRange(0, 100);
    is_running = false;
    time_step = 0.1;

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateSimulation);

    //wartosci domyslne dla ograniczen bo sie wykresy nie rysuja
    ui->sterowanie_doubleSpinBox_5->setValue(-10.0);
    ui->sterowanie_doubleSpinBox_6->setValue(10.0);

    //wykresy

    //zadsana i regul
    series_zadana = new QLineSeries();
    series_zadana->setName("Zadana");
    series_regulowana = new QLineSeries();
    series_regulowana->setName("Regulowana");

    QChart *chart1 = new QChart();
    chart1->addSeries(series_zadana);
    chart1->addSeries(series_regulowana);

    chart1->createDefaultAxes();
    chart1->axes(Qt::Horizontal).first()->setRange(0.0, ui->szer_okna_spinBox_2->value());
    chart1->axes(Qt::Vertical).first()->setRange(-10, 10);
    chart1->setTitle("Wartość zadana i regulowana");
    if(!chart1->axes(Qt::Horizontal).isEmpty()) chart1->axes(Qt::Horizontal).first()->setTitleText("Czas [s]");
    if(!chart1->axes(Qt::Vertical).isEmpty()) chart1->axes(Qt::Vertical).first()->setTitleText("Wartość [-]");


    QChartView *view1 = new QChartView(chart1);
    view1->setRenderHint(QPainter::Antialiasing);
    ui->verticalLayout->addWidget(view1);

    //uchyb
    series_uchyb = new QLineSeries();
    QChart *chart2 = new QChart();
    chart2->addSeries(series_uchyb);

    chart2->createDefaultAxes();
    chart2->axes(Qt::Horizontal).first()->setRange(0.0, ui->szer_okna_spinBox_2->value());
    chart2->axes(Qt::Vertical).first()->setRange(-10, 10);
    chart2->setTitle("Uchyb");
    if(!chart2->axes(Qt::Horizontal).isEmpty()) chart2->axes(Qt::Horizontal).first()->setTitleText("Czas [s]");
    if(!chart2->axes(Qt::Vertical).isEmpty()) chart2->axes(Qt::Vertical).first()->setTitleText("Uchyb [-]");


    QChartView *view2 = new QChartView(chart2);
    ui->horizontalLayout->addWidget(view2);

    //sterowanie
    series_sterowanie = new QLineSeries();
    QChart *chart3 = new QChart();
    chart3->addSeries(series_sterowanie);

    chart3->createDefaultAxes();
    chart3->axes(Qt::Horizontal).first()->setRange(0.0, ui->szer_okna_spinBox_2->value());
    chart3->axes(Qt::Vertical).first()->setRange(-10, 10);
    chart3->setTitle("Sterowanie");
    if(!chart3->axes(Qt::Horizontal).isEmpty()) chart3->axes(Qt::Horizontal).first()->setTitleText("Czas [s]");
    if(!chart3->axes(Qt::Vertical).isEmpty()) chart3->axes(Qt::Vertical).first()->setTitleText("Sterowanie [-]");


    QChartView *view3 = new QChartView(chart3);
    ui->horizontalLayout->addWidget(view3);

    //Składowe sterowania

    //P
    series_P = new QLineSeries();
    series_P->setName("Proporcjonalnt");
    series_P->setColor(Qt::red);

    //I
    series_I = new QLineSeries();
    series_I->setName("Całkujący");
    series_I->setColor(Qt::green);

    //D
    series_D = new QLineSeries();
    series_D->setName("Różniczkujący");
    series_D->setColor(Qt::blue);

    QChart *chart4 = new QChart();

    chart4->addSeries(series_P);
    chart4->addSeries(series_I);
    chart4->addSeries(series_D);
    chart4->createDefaultAxes();

    chart4->axes(Qt::Horizontal).first()->setRange(0, ui->szer_okna_spinBox_2->value());
    chart4->axes(Qt::Vertical).first()->setRange(-10, 10);
    chart4->setTitle("Składowe sterowania PID");
    if(!chart4->axes(Qt::Horizontal).isEmpty()) chart4->axes(Qt::Horizontal).first()->setTitleText("Czas [s]");
    if(!chart4->axes(Qt::Vertical).isEmpty()) chart4->axes(Qt::Vertical).first()->setTitleText("Wartość [-]");


    QChartView *view4 = new QChartView(chart4);
    ui->horizontalLayout->addWidget(view4);


    connect(ui->StartStop_button_2, &QPushButton::clicked, this, &MainWindow::on_start_stop_clicked);
    connect(ui->reset_button_2, &QPushButton::clicked, this, &MainWindow::on_reset_clicked);


    ui->przed_radioButton->setChecked(true);

    //connect aby nie aktualizowalo wykresow w trakcie wpisuwania

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

    connect(g_reg, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &MainWindow::updateControllerParams);
    connect( g_calka, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &MainWindow::updateControllerParams);
    connect(g_syg, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &MainWindow::updateControllerParams);

    connect(ui->memory_reset_d_button_2, &QPushButton::clicked, this, &MainWindow::updateControllerParams);
    connect(ui->memory_reset_i_button_2, &QPushButton::clicked, this, &MainWindow::updateControllerParams);
    connect(ui->del_syg_button, &QPushButton::clicked,this, &MainWindow::updateControllerParams);
    //!dodane
    // Dynamiczna zmiana interwału
    connect(ui->interwal_doubleSpinBox_2, &QDoubleSpinBox::editingFinished, this, [this](){
        if(is_running) {
            double interval = ui->interwal_doubleSpinBox_2->value();
            time_step = interval / 1000.0;
            timer->setInterval(interval);
        }
    });
    //?

    updateControllerParams();

    //sk
    klient = new myTCPclient(this);
    serwer = new myTCPserwer(this);

    grupaSieciowa = new QGroupBox("Ustawienia Polaczenia TCP", this);
    poleIP = new QLineEdit("192.168.0.1");
    polePort = new QSpinBox();
    polePort->setRange(1024, 65535);
    polePort->setValue(5555);
    btnPolacz = new QPushButton("Polacz (Klient)");
    btnSerwer = new QPushButton("Uruchom Serwer");
    btnRozlacz = new QPushButton("Rozlacz");
    lblStatus = new QLabel("Status: Rozlaczono");
    lblStatus->setStyleSheet("color: red; font-weight: bold;");

    QGridLayout *siecLayout = new QGridLayout();
    siecLayout->addWidget(new QLabel("IP:"), 0, 0);
    siecLayout->addWidget(poleIP, 0, 1);
    siecLayout->addWidget(new QLabel("Port:"), 1, 0);
    siecLayout->addWidget(polePort, 1, 1);
    siecLayout->addWidget(btnSerwer, 2, 0, 1, 2);
    siecLayout->addWidget(btnPolacz, 3, 0, 1, 2);
    siecLayout->addWidget(btnRozlacz, 4, 0, 1, 2);
    siecLayout->addWidget(lblStatus, 5, 0, 1, 2);
    grupaSieciowa->setLayout(siecLayout);

    zawartoscSymulacji = new QWidget();
    zawartoscSymulacji->setLayout(Ui_MainWindow::all_2);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(zawartoscSymulacji);

    QVBoxLayout *ostatecznyPrawyLayout = new QVBoxLayout();
    ostatecznyPrawyLayout->addWidget(grupaSieciowa);
    ostatecznyPrawyLayout->addWidget(scrollArea);

    connect(btnPolacz, &QPushButton::clicked, this, &MainWindow::on_przyciskPolacz_clicked);
    connect(btnSerwer, &QPushButton::clicked, this, &MainWindow::on_przyciskUruchomSerwer_clicked);
    connect(btnRozlacz, &QPushButton::clicked, this, &MainWindow::on_przyciskRozlacz_clicked);

    connect(klient, &myTCPclient::polaczono, this, &MainWindow::ustawStanPolaczony);
    connect(serwer, &myTCPserwer::klientPodlaczony, this, &MainWindow::ustawStanPolaczony);

    connect(klient, &myTCPclient::rozlaczono, this, &MainWindow::ustawStanRozlaczony);
    connect(serwer, &myTCPserwer::klientOdlaczony, this, &MainWindow::ustawStanRozlaczony);

    ustawStanRozlaczony();
}

MainWindow::~MainWindow()
{
    delete kontroler;
    delete ui;
}

//model arx button
void MainWindow::on_ARX_model_button_clicked()
{

    ARX *secWindow = new ARX(this);

    if(kontroler && kontroler->get_uar() && kontroler->get_uar()->get_model()) {

        secWindow->set_parametry
            (
                kontroler->get_uar()->get_model()->get_wspolczynniki_a(),
                kontroler->get_uar()->get_model()->get_wspolczynniki_b(),

                kontroler->get_uar()->get_model()->get_opoznienie(),
                kontroler->get_uar()->get_model()->get_odchylenie()
                );
    }

    connect(secWindow, &ARX::s_update_model_params, this, &MainWindow::on_model_update);
    secWindow->show();
}
//set wspolczynnikow w arx
void MainWindow::on_model_update(std::vector<double> A, std::vector<double> B, int k, double szum)
{
    if(kontroler && kontroler->get_uar() && kontroler->get_uar()->get_model()) {
        kontroler->get_uar()->get_model()->set_wspolczynniki_a(A);
        kontroler->get_uar()->get_model()->set_wspolczynniki_b(B);
        kontroler->get_uar()->get_model()->set_opoznienie(k);
        kontroler->get_uar()->get_model()->set_odchylenie_standardowe(szum);
    }
}

//start/stop
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

//resetowanie wykresow
void MainWindow::on_reset_clicked()
{
    is_running = false;
    timer->stop();

    if(kontroler) {
        kontroler->reset();
    }

    series_zadana->clear();
    series_regulowana->clear();
    series_uchyb->clear();
    series_sterowanie->clear();
    series_P->clear();
    series_I->clear();
    series_D->clear();
}


void MainWindow::upc(){
    ui->calka_doubleSpinBox_3->setValue(0);

    updateControllerParams();
}


//update parametrow+to jest to do connectw
void MainWindow::updateControllerParams() {
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
            if(ui->checkBox_3->isChecked()) {
                pid->set_ograniczenia(u_min, u_max);
            } else {
                if(ui->checkBox_3->isChecked()) {
                    pid->set_ograniczenia(u_min, u_max);
                } else {
                    ui->sterowanie_doubleSpinBox_5->setValue(-10.0);
                    ui->sterowanie_doubleSpinBox_6->setValue(10.0);
                    pid->set_ograniczenia(-10.0, 10.0);
                }
            }
            pid->set_anti_windup(antiwindup);

            if(ui->przed_radioButton->isChecked()) pid->set_tryb_calki(RegulatorPID::tryb_calki::stala_przed_suma);
            else if(ui->w_sumie_radioButton->isChecked()) pid->set_tryb_calki(RegulatorPID::tryb_calki::StalaPodSuma);
        }

    } else if(ui->ONOFF_radioButton->isChecked()) {
        uar->ustaw_aktywny_regulator(ProstyUAR::TypRegulatora::ONOFF);

        double u_on = ui->doubleSpinBox_6->value();
        double hyst = ui->histereza_doubleSpinBox_3->value();
        RegulatorONOFF* onoff = uar->get_onoff();
        if(onoff) {
            onoff->set_nastawy(hyst, u_on);
        }
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
//update symulacji
void MainWindow::updateSimulation()
{
    if(!kontroler) return;

    kontroler->symuluj_krok(time_step);

    double t = kontroler->get_aktualny_czas();

    double setpoint = kontroler->get_wartosc_zadana();
    double y = kontroler->get_wyjscie();
    double e = kontroler->get_uchyb();
    double u = kontroler->get_sterowanie();

    double sk_p = kontroler->get_skladowa_p();
    double sk_i = kontroler->get_skladowa_i();
    double sk_d = kontroler->get_skladowa_d();

    updateCharts(setpoint, y, e, u, sk_p, sk_i, sk_d);//, sk_d
}
//ustawia wartosci na chartach
void MainWindow::updateCharts(double setpoint, double cv, double error, double control, double p,double i,double d)
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
    //gora
    while(!series_regulowana->points().isEmpty()&&series_regulowana->points().first().x()<x_min){
        series_regulowana->remove(0);
    }

    while(!series_zadana->points().isEmpty()&&series_zadana->points().first().x()<x_min){
        series_zadana->remove(0);
    }
    //dol l
    while(!series_uchyb->points().isEmpty()&&series_uchyb->points().first().x()<x_min){
        series_uchyb->remove(0);
    }
    //dol sr
    while(!series_sterowanie->points().isEmpty()&&series_sterowanie->points().first().x()<x_min){
        series_sterowanie->remove(0);
    }
    //dol r (PID)
    while(!series_P->points().isEmpty()&&series_P->points().first().x()<x_min){
        series_P->remove(0);
    }

    while(!series_I->points().isEmpty()&&series_I->points().first().x()<x_min){
        series_I->remove(0);
    }
    while(!series_D->points().isEmpty()&&series_D->points().first().x()<x_min){
        series_D->remove(0);
    }





    auto setAxisX = [x_min, x_max](QLineSeries* s){
        if(s->chart() && !s->chart()->axes(Qt::Horizontal).isEmpty()) {
            s->chart()->axes(Qt::Horizontal).first()->setRange(x_min, x_max);
        }
    };

    setAxisX(series_zadana);
    setAxisX(series_uchyb);
    setAxisX(series_sterowanie);
    setAxisX(series_P);


    double y_min = std::numeric_limits<double>::max();
    double y_max = std::numeric_limits<double>::lowest();

    //zakres serii-sprawdzenie
    auto checkRange = [&](QLineSeries* s) {
        for(const QPointF &point : s->points()) {

            if (point.y() < y_min) y_min = point.y();
            if (point.y() > y_max) y_max = point.y();

        }
    };


    checkRange(series_zadana);
    checkRange(series_regulowana);
    checkRange(series_uchyb);
    checkRange(series_sterowanie);
    checkRange(series_P);
    checkRange(series_I);
    checkRange(series_D);

    //skalowanie osi Y w sposób niezalezny dla każdego wykresu
    auto ScaleCharts=[](QList<QLineSeries*>SeriesList){
        if(SeriesList.isEmpty())return;

        //ustawienie wartości najmniejszej-najwiekszej
        double y_min=std::numeric_limits<double>::max();
        double y_max=std::numeric_limits<double>::lowest();
        //aktualizacja zmiennych y_min oraz y_max
        for(QLineSeries* S:SeriesList)
        {
            for(const QPointF &point:S->points())
            {
                if(point.y()<y_min)y_min=point.y();
                if(point.y()>y_max)y_max=point.y();
            }
        }
        //ustawienie marginesów dla wykresów
        double margin=(y_max-y_min)*0.1;

        //margines dla linii prostej
        if(margin==0)margin=1.0;

        y_min-=margin;
        y_max+=margin;

        //Wlasciwe ustawienie zakresu dla oY
        QChart* chart=SeriesList.first()->chart();
        if(chart && !chart->axes(Qt::Vertical).isEmpty())
        {
            chart->axes(Qt::Vertical).first()->setRange(y_min,y_max);
        }
    };

    //Wywołanie ustawienia nowego zakresu dla osi Y
    //wykres zadana-regulowana
    ScaleCharts({series_zadana,series_regulowana});
    //wykres uchyb
    ScaleCharts({series_uchyb});
    //wykres wartosc sterowana
    ScaleCharts({series_sterowanie});
    //wykres PID
    ScaleCharts({series_P,series_I,series_D});



    auto cleanupSeries = [x_min](QLineSeries* s) {
        if (!s) return;

        int countToRemove = 0;
        QList<QPointF> points = s->points();
        for(const QPointF& p : points) {
            if(p.x() < x_min) {
                countToRemove++;
            } else {
                break;
            }
        }
        if(countToRemove > 0) {
            s->remove(0, countToRemove);
        }
    };

    cleanupSeries(series_zadana);
    cleanupSeries(series_regulowana);
    cleanupSeries(series_uchyb);
    cleanupSeries(series_sterowanie);
    cleanupSeries(series_P);
    cleanupSeries(series_I);
    cleanupSeries(series_D);
}
//? dodane
//zapis
void MainWindow::on_save_button_2_clicked() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Wybierz katalog zapisu"),
                                                    ".",
                                                    QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks);
    if(dir.isEmpty()) return;

    ConfigData config;

    // Main
    config.main.interwal_ms = ui->interwal_doubleSpinBox_2->value();
    config.main.szerokosc_okna = ui->szer_okna_spinBox_2->value();

    // Regulator
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

    // Model
    ProstyUAR* uar = kontroler ? kontroler->get_uar() : nullptr;
    if(uar && uar->get_model()) {
        config.model.A = uar->get_model()->get_wspolczynniki_a();
        config.model.B = uar->get_model()->get_wspolczynniki_b();
        config.model.opoznienie = uar->get_model()->get_opoznienie();
        config.model.szumy = uar->get_model()->get_odchylenie();
    }

    // Signals
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

    if(ConfigManager::zapiszKonfiguracje(config, dir)) {
        QMessageBox::information(this, "Sukces", "Konfiguracja zapisana pomyslnie!");
    } else {
        QMessageBox::critical(this, "Blad", "Nie udalo sie zapisac konfiguracji.");
    }
}
//odczyt zapisu
void MainWindow::on_read_button_2_clicked() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Wybierz plik konfiguracji"),
                                                    ".",
                                                    tr("Pliki JSON (*.json)"));
    if(fileName.isEmpty()) return;

    ConfigData config = ConfigManager::wczytajKonfiguracje(fileName);

    // Main
    ui->interwal_doubleSpinBox_2->setValue(config.main.interwal_ms);
    ui->szer_okna_spinBox_2->setValue(config.main.szerokosc_okna);

    // Regulator
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

    // Model
    if(kontroler && kontroler->get_uar() && kontroler->get_uar()->get_model()) {
        kontroler->get_uar()->get_model()->set_wspolczynniki_a(config.model.A);
        kontroler->get_uar()->get_model()->set_wspolczynniki_b(config.model.B);
        kontroler->get_uar()->get_model()->set_odchylenie_standardowe(config.model.szumy);
    }

    // Signals
    if(config.signal.typ == "sinusoidalny") ui->sin_syg_radioButton->setChecked(true);
    else ui->sqrt_syg_radioButton->setChecked(true);

    ui->amp_square_doubleSpinBox_2->setValue(config.signal.rect.amp);
    ui->period_square_doubleSpinBox_2->setValue(config.signal.rect.period);
    ui->sklad_stal_sqrt_doubleSpinBox->setValue(config.signal.rect.offset);
    ui->fill_square_doubleSpinBox_2->setValue(config.signal.rect.fill);

    ui->amp_sinus_doubleSpinBox_2->setValue(config.signal.sin.amp);
    ui->period_sinus_doubleSpinBox_2->setValue(config.signal.sin.period);
    ui->sklad_stal_sin_doubleSpinBox->setValue(config.signal.sin.offset);

    if(config.signal.typ == "sinusoidalny") {
        ui->doubleSpinBox_8->setValue(config.signal.sin.activation_time);
    } else {
        ui->doubleSpinBox_8->setValue(config.signal.rect.activation_time);
    }

    QMessageBox::information(this, "Sukces", "Konfiguracja wczytana pomyslnie!");

    // Aktualizacja parametrów kontrolera po wczytaniu danych do UI
    updateControllerParams();
}

//close bez zapisania wartosci dla arx- bo qsettings je wczytuja po calkowitym zamknieciu programu
void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("PK", "ARX");
    settings.clear();
    event->accept();
}

//dodaenie sygnalu aby byly widoczne poprzednie
void MainWindow::on_add_syg_button_clicked()
{
    if(ui->sin_syg_radioButton->isChecked()){

        double amp=ui->amp_sinus_doubleSpinBox_2->value();
        double okres=ui->period_sinus_doubleSpinBox_2->value();
        double skladowa=ui->sklad_stal_sin_doubleSpinBox->value();

        QString sin_opis =QString("Amp=%1, T=%2, Stała=%3").arg(amp).arg(okres).arg(skladowa);
        ui->sygn_sinus_l_2->append(sin_opis);

    }
    else if(ui->sqrt_syg_radioButton->isChecked()){


        double amp=ui->amp_square_doubleSpinBox_2->value();
        double okres=ui->period_square_doubleSpinBox_2->value();
        double skladowa=ui->sklad_stal_sqrt_doubleSpinBox->value();
        double fill =ui->fill_square_doubleSpinBox_2->value();

        QString sqrt_opis =QString("Amp=%1, T=%2, Stała=%3, Wyp=%4").arg(amp).arg(okres).arg(skladowa).arg(fill);
        ui->sygn_square_l_2->append(sqrt_opis);
    }


}
//usuwanie sygnlau

void MainWindow::on_del_syg_button_clicked()
{
    if(ui->sin_syg_radioButton->isChecked()){
        ui->amp_sinus_doubleSpinBox_2->setValue(0.0);
        ui->period_sinus_doubleSpinBox_2->setValue(0.0);
        ui->sklad_stal_sin_doubleSpinBox->setValue(0.0);
    }
    else if(ui->sqrt_syg_radioButton->isChecked()){
        ui->amp_square_doubleSpinBox_2->setValue(0.0);
        ui->sklad_stal_sqrt_doubleSpinBox->setValue(0.0);
        ui->fill_square_doubleSpinBox_2->setValue(0.5);
        ui->period_square_doubleSpinBox_2->setValue(0.0);
    }
}

//sk
void MainWindow::ustawStanPolaczony() {

    poleIP->setEnabled(false);
    polePort->setEnabled(false);
    btnPolacz->setEnabled(false);
    btnSerwer->setEnabled(false);
    btnRozlacz->setEnabled(true);

    lblStatus->setText("Status: POLACZONO");
    lblStatus->setStyleSheet("color: green; font-weight: bold;");

    zablokujKontrolkiSymulacji(false);
}

void MainWindow::ustawStanRozlaczony() {

    poleIP->setEnabled(true);
    polePort->setEnabled(true);
    btnPolacz->setEnabled(true);
    btnSerwer->setEnabled(true);
    btnRozlacz->setEnabled(false);

    lblStatus->setText("Status: ROZLACZONO");
    lblStatus->setStyleSheet("color: red; font-weight: bold;");

    zablokujKontrolkiSymulacji(true);
}

void MainWindow::zablokujKontrolkiSymulacji(bool zablokowane) {
    // Tutaj musisz wpisać nazwy wskaźników do Twoich zakładek lub suwaków
    // Przykład:
    // ui->tabARX->setEnabled(!zablokowane);
    // ui->tabPID->setEnabled(!zablokowane);
    // btnStartSymulacja->setEnabled(!zablokowane);
}

void MainWindow::on_przyciskPolacz_clicked() {

    klient->polaczZSerwerem(poleIP->text(), polePort->value());
}

void MainWindow::on_przyciskUruchomSerwer_clicked() {

    if(serwer->uruchomSerwer(polePort->value())) {

        btnSerwer->setEnabled(false);
        btnPolacz->setEnabled(false);
        lblStatus->setText("Status: OCZEKIWANIE...");
    }
}

void MainWindow::on_przyciskRozlacz_clicked() {

    klient->rozlaczZSerwerem();
    serwer->zatrzymajSerwer();
    ustawStanRozlaczony();
}
