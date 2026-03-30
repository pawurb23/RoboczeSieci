#pragma once

#include "ModelARX.h"
#include "RegulatorPID.h"
#include "RegulatorONOFF.h"

class ProstyUAR {
public:
    enum class TypRegulatora {
        PID,
        ONOFF,
        BRAK
    };

    ProstyUAR();
    ~ProstyUAR();

    void symuluj_krok(double krok_czasu, double wartosc_zadana);

    double get_wyjscie() const { return m_aktualne_y; }
    double get_sterowanie() const { return m_aktualne_u; }
    double get_uchyb() const { return m_aktualne_e; }
    double get_wartosc_zadana() const { return m_aktualna_zadana; }
    
    double get_skladowa_p() const;
    double get_skladowa_i() const;
    double get_skladowa_d() const;

    ModelARX* get_model() const { return m_model; }
    RegulatorPID* get_pid() const { return m_pid; }
    RegulatorONOFF* get_onoff() const { return m_onoff; }

    void ustaw_aktywny_regulator(TypRegulatora typ);
    TypRegulatora get_aktywny_regulator_typ() const { return m_typ_regulatora; }

    void reset();
    
    double get_aktualny_czas() const { return m_aktualny_czas; }
    void reset_czas() { m_aktualny_czas = 0.0; }

private:
    ModelARX* m_model;
    RegulatorPID* m_pid;
    RegulatorONOFF* m_onoff;

    TypRegulatora m_typ_regulatora;
    double m_aktualne_y;
    double m_aktualne_u;
    double m_aktualne_e;
    double m_aktualna_zadana;
    double m_aktualny_czas;
};
