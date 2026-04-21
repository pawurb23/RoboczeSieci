#ifndef NETWORK_H
#define NETWORK_H

#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

// Typy pakietów
const quint8 PKT_CONFIG     = 0x01;
const quint8 PKT_CONTROL    = 0x02;
const quint8 PKT_OUTPUT     = 0x03;
const quint8 PKT_COMMAND    = 0x04;

// Struktura pomocnicza do pakietów CONTROL
struct ControlPacket {
    int id;
    double u;
    static QByteArray toJson(int id, double u) {
        QJsonObject obj;
        obj["id"] = id;
        obj["u"] = u;
        return QJsonDocument(obj).toJson();
    }
    static bool fromJson(const QByteArray &data, int &id, double &u) {
        QJsonObject obj = QJsonDocument::fromJson(data).object();
        if (obj.contains("id") && obj.contains("u")) {
            id = obj["id"].toInt();
            u = obj["u"].toDouble();
            return true;
        }
        return false;
    }
};

// Struktura pomocnicza do pakietów OUTPUT
struct OutputPacket {
    int id;
    double y;
    static QByteArray toJson(int id, double y) {
        QJsonObject obj;
        obj["id"] = id;
        obj["y"] = y;
        return QJsonDocument(obj).toJson();
    }
    static bool fromJson(const QByteArray &data, int &id, double &y) {
        QJsonObject obj = QJsonDocument::fromJson(data).object();
        if (obj.contains("id") && obj.contains("y")) {
            id = obj["id"].toInt();
            y = obj["y"].toDouble();
            return true;
        }
        return false;
    }
};

#endif // NETWORK_H
