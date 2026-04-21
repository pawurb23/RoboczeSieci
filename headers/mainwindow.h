#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <vector>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QScrollArea>
#include <QHash>
#include <QDateTime>

#include "Kontroler.h"
#include "mytcpclient.h"
#include "mytcpserwer.h"
#include "Network.h"
#include "PakietLicz.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(bool regulatorMode, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Istniejące sloty
    void on_ARX_model_button_clicked();
    void on_model_update(std::vector<double> A, std::vector<double> B, int k, double szum);
    void updateControllerParams();
    void on_start_stop_clicked();
    void on_reset_clicked();
    void updateSimulation();
    void on_save_button_2_clicked();
    void on_read_button_2_clicked();
    void on_add_syg_button_clicked();
    void on_del_syg_button_clicked();

    // Sloty dla przycisków sieciowych (nazwy zgodne z UI)
    void on_btnPolacz_clicked();
    void on_btnSerwer_clicked();
    void on_btnRozlacz_clicked();

    // Sloty dla stanu połączenia
    void ustawStanPolaczony();
    void ustawStanRozlaczony();

    // Sloty dla komunikacji TCP
    void onTcpConnected();
    void onTcpDisconnected();
    void onClientDataReceived(quint8 typ, QByteArray data);
    void onServerDataReceived(quint8 typ, QByteArray data);

    // Timer symulacji sieciowej (dla regulatora)
    void onNetworkTimer();

    // Aktualizacja statystyk
    void updateNetworkStats(int pps, double avgLat, int lost);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    Kontroler* kontroler;


    QButtonGroup *groupRegulator;
    QButtonGroup *groupIntegral;
    QButtonGroup *groupSignal;

    // Symulacja offline 
    QTimer *timer;
    bool is_running;
    double time_step;

    // Serie wykresów
    QLineSeries *series_zadana;
    QLineSeries *series_regulowana;
    QLineSeries *series_uchyb;
    QLineSeries *series_sterowanie;
    QLineSeries *series_P;
    QLineSeries *series_I;
    QLineSeries *series_D;

    // Obiekty sieciowe
    myTCPclient *klient;
    myTCPserwer *serwer;
    PacketStats *stats;

    // Tryb pracy
    bool isRegulatorMode;

    // Dla trybu regulatora
    QTimer *networkTimer;
    int currentPacketId;
    QHash<int, QDateTime> pendingPackets;
    double lastReceivedY;

    void setupRegulatorMode();
    void setupObiektMode();
    void sendConfig();
    void sendControl(double u);
    void updateCharts(double setpoint, double cv, double error, double control,
                      double p, double i, double d);
    void zablokujKontrolkiSymulacji(bool zablokowane);
};

#endif // MAINWINDOW_H
