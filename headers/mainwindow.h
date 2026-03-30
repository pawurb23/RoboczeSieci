#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <vector>

#include "Kontroler.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_ARX_model_button_clicked();
    void on_model_update(std::vector<double> A, std::vector<double> B, int k, double szum);
    void upc();
    void updateControllerParams();
    void on_start_stop_clicked();
    void on_reset_clicked();
    
    void updateSimulation();
    
    void on_save_button_2_clicked();
    void on_read_button_2_clicked();

//    void on_memory_reset_d_button_2_clicked();

//    void on_memory_reset_i_button_2_clicked();

    void on_add_syg_button_clicked();

    void on_del_syg_button_clicked();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;

    // Kontroler aplikacji
    Kontroler* kontroler;

    // Połaczenaia radio Buttonow
    QButtonGroup *groupRegulator;
    QButtonGroup *groupIntegral;
    QButtonGroup *groupSignal;

    // Simulation Loop
    QTimer *timer;
    bool is_running;
    double time_step;

    // Seires dla wykresow
    QLineSeries *series_zadana;
    QLineSeries *series_regulowana;
    QLineSeries *series_uchyb;
    QLineSeries *series_sterowanie;
    QLineSeries *series_P;
    QLineSeries *series_I;
    QLineSeries *series_D;



    void updateCharts(double setpoint, double cv, double error, double control, double p,double i,double d);
};

#endif // MAINWINDOW_H
