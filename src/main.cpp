//#include "mainwindow.h"
#include <QCoreApplication>
#include <QDebug>
#include "myTCPclient.h"
#include "myTCPserwer.h"
#include "ConfigManager.h"

#include <QApplication>

#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //MainWindow w;
    //w.show();

    std::cout << "Wybierz tryb:" << std::endl;
    std::cout << "1. Uruchom jako SERWER" << std::endl;
    std::cout << "2. Uruchom jako KLIENT" << std::endl;
    std::cout << "Twoj wybor (1 lub 2): ";

    int wybor = 0;
    std::cin >> wybor;

    if (wybor == 1) {

        qInfo() << "\nTryb serwera";
        myTCPserwer *serwer = new myTCPserwer(&a);

        if(serwer->uruchomSerwer(5555)) { qInfo() << "Czekam na klienta..."; }
        else { qCritical() << "Blad"; return -1; }

        QObject::connect(serwer, &myTCPserwer::odebranoDane, [](quint8 typ, QByteArray dane){

            if (typ == 1) {

                qInfo() << "\nOtrzymano dane!";

                ConfigData odebranyConfig = ConfigManager::deserializacja(dane);

                qInfo() << "PID: Kp =" << odebranyConfig.regulator.pid.k << "| Ti =" << odebranyConfig.regulator.pid.ti;
                qInfo() << "Model ARX opoznienie =" << odebranyConfig.model.opoznienie;
                qInfo() << "JSON: " << QString::fromUtf8(dane);
            }
        });

    }
    else if (wybor == 2) {

        std::string ip_str;
        std::cout << "Podaj adres IP serwera: ";
        std::cin >> ip_str;

        QString ip = QString::fromStdString(ip_str);

        qInfo() << "Laczenie z" << ip << "...";

        myTCPclient *klient = new myTCPclient(&a);
        klient->polaczZSerwerem(ip, 5555);

        QObject::connect(klient, &myTCPclient::polaczono, [klient](){

            qInfo() << "Polaczono!";

            ConfigData configDoWyslania;
            configDoWyslania.regulator.pid.k = 4.5;
            configDoWyslania.regulator.pid.ti = 12.0;
            configDoWyslania.regulator.pid.td = 0.5;
            configDoWyslania.model.A = {1.0, -0.5};
            configDoWyslania.model.B = {0.2, 0.1};
            configDoWyslania.model.opoznienie = 3;

            QByteArray bajtyJSON = ConfigManager::serializacja(configDoWyslania);
            klient->wyslijDane(1, bajtyJSON);

            qInfo() << "Konfiguracja wysłana";
        });
    }
    else { std::cout << "Zly wybor!" << std::endl; return 0; }

    return a.exec();
}
