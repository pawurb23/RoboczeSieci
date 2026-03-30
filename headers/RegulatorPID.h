#pragma once
#include "Regulator.h" 

class RegulatorPID : public Regulator { //Dziedziczymy po regulatorze
public:
   
    enum class tryb_calki {
        stala_przed_suma,
        StalaPodSuma
    };
    //Kontruktory
    RegulatorPID();
    RegulatorPID(double i_k);
    RegulatorPID(double i_k, double i_ti);
    RegulatorPID(double i_k, double i_ti, double i_td);
    virtual ~RegulatorPID() {} 

    // Symulacja kroku
    virtual double symuluj(double uchyb) override; 

    // Reset pami�� regulatora
    virtual void reset();

    void set_nastawy(double k, double ti, double td);
    void setStalaCalk(double i_ti);
    void set_tryb_calki(tryb_calki tryb);

 
    void set_ograniczenia(double u_min, double u_max);
    void set_anti_windup(bool wl);
    //getery
    double get_wzmocnienie() const
    {
        return m_ostatnia_skladowa_i;
    }
    double get_calka() const
    {
        return m_ostatnia_skladowa_d;
    }
    double get_td() const
    {
        return m_ostatnia_skladowa_p;
    }


private:
    // Nastawy regulatora
    double m_k; //wzmocnienie 
    double m_ti; //czas calkowania
    double m_td; //czas roniczkowania

    // Pamiec regulatora
    double m_suma_uchybow;
    double m_poprzedni_uchyb;
    double m_poprzednia_i; // Potrzebne do anti-windup i trybow

    // Tryb pracy
    tryb_calki m_tryb_calki;

    // Ograniczenia wyjscia, anti-windup
    double m_u_min;
    double m_u_max;
    bool m_anti_windup_wl;

    //dla skladowych-dla geterow
    double m_ostatnia_skladowa_p = 0.0;
    double m_ostatnia_skladowa_i = 0.0;
    double m_ostatnia_skladowa_d = 0.0;

};
