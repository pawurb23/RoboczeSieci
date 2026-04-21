#include <QApplication>
#include <QDebug>
#include <iostream>
#include <string>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    std::cout << "Wybierz tryb pracy:" << std::endl;
    std::cout << "1. Regulator" << std::endl;
    std::cout << "2. Obiekt" << std::endl;
    std::cout << "Twoj wybor (1 lub 2): ";
    int tryb;
    std::cin >> tryb;

    bool regulatorMode = (tryb == 1);
    MainWindow w(regulatorMode);
    w.show();

    return a.exec();
}
