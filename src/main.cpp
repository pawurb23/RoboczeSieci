//#include "mainwindow.h"
#include <QCoreApplication>
#include <QDebug>
#include "myTCPclient.h"
#include "myTCPserwer.h"
#include "ConfigManager.h"

#include <QApplication>

#include <iostream>
#include <string>

QString wektorDoStringa(const std::vector<double>& wektor) {

    QString wynik = "[ ";
    for(double v : wektor) wynik += QString::number(v) + " ";
    wynik += "]";
    return wynik;
}

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

        myTCPserwer *serwer = new myTCPserwer(&a);

        if(serwer->uruchomSerwer(5555)) { qInfo().noquote() << "\nCzekam na klienta..."; }

        QObject::connect(serwer, &myTCPserwer::odebranoDane, [](quint8 typ, QByteArray dane){

            if (typ == 1) {

                ConfigData odbConfig = ConfigManager::deserializacja(dane);

                qInfo().noquote() << "Konfiguracja JSON:";

                qInfo().noquote() << "[REGULATOR PID]";
                qInfo().noquote() << "  Wzmocnienie (Kp)     : " << odbConfig.regulator.pid.k;
                qInfo().noquote() << "  Czas calkowania (Ti) : " << odbConfig.regulator.pid.ti;
                qInfo().noquote() << "  Czas rozniczk. (Td)  : " << odbConfig.regulator.pid.td;

                qInfo().noquote() << "\n[MODEL ARX]";
                qInfo().noquote() << "  Wielomian A          : " << wektorDoStringa(odbConfig.model.A);
                qInfo().noquote() << "  Wielomian B          : " << wektorDoStringa(odbConfig.model.B);
                qInfo().noquote() << "  Opoznienie (k)       : " << odbConfig.model.opoznienie;

                qInfo().noquote() << "\nJSON przeslany przez siec:";
                qInfo().noquote() << QString::fromUtf8(dane);
            }
        });

    }
    else if (wybor == 2) {

        std::string ip_str;
        std::cout << "Podaj adres IP serwera: ";
        std::cin >> ip_str;

        QString ip = QString::fromStdString(ip_str);

        qInfo().noquote() << "\nLaczenie z" << ip << "...";

        myTCPclient *klient = new myTCPclient(&a);
        klient->polaczZSerwerem(ip, 5555);

        QObject::connect(klient, &myTCPclient::polaczono, [klient](){
            qInfo().noquote() << "Polaczono!";

            ConfigData configDoWyslania;
            configDoWyslania.regulator.typ = 0;

            configDoWyslania.regulator.pid.k = 4.5;
            configDoWyslania.regulator.pid.ti = 12.0;
            configDoWyslania.regulator.pid.td = 0.5;

            configDoWyslania.model.A = {1.0, -0.6, 0.1};
            configDoWyslania.model.B = {0.2, 0.05};
            configDoWyslania.model.opoznienie = 3;

            QByteArray bajtyJSON = ConfigManager::serializacja(configDoWyslania);

            klient->wyslijDane(1, bajtyJSON);

            qInfo().noquote() << "Konfiguracja zostala wyslana pomyslnie.";
        });
    }

    return a.exec();
}
