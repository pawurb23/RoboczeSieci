#include "RegulatorPID.h"
#include <cmath>
#include <limits>

RegulatorPID::RegulatorPID()
    : m_k(0.5)
    , m_ti(0.0)
    , m_td(0.0)
    , m_suma_uchybow(0.0)
    , m_poprzedni_uchyb(0.0)
    , m_tryb_calki(tryb_calki::StalaPodSuma)
    , m_u_min(-std::numeric_limits<double>::infinity())
    , m_u_max(std::numeric_limits<double>::infinity())
    , m_anti_windup_wl(true)
    , m_poprzednia_i(0.0)
{}

RegulatorPID::RegulatorPID(double i_k)
    : m_k(i_k)
    , m_ti(0.0)
    , m_td(0.0)
    , m_suma_uchybow(0.0)
    , m_poprzedni_uchyb(0.0)
    , m_tryb_calki(tryb_calki::StalaPodSuma)
    , m_u_min(-std::numeric_limits<double>::infinity())
    , m_u_max(std::numeric_limits<double>::infinity())
    , m_anti_windup_wl(true)
    , m_poprzednia_i(0.0)
{}

RegulatorPID::RegulatorPID(double i_k, double i_ti)
    : m_k(i_k)
    , m_ti(i_ti)
    , m_td(0.0)
    , m_suma_uchybow(0.0)
    , m_poprzedni_uchyb(0.0)
    , m_tryb_calki(tryb_calki::StalaPodSuma)
    , m_u_min(-std::numeric_limits<double>::infinity())
    , m_u_max(std::numeric_limits<double>::infinity())
    , m_anti_windup_wl(true)
    , m_poprzednia_i(0.0)
{}

RegulatorPID::RegulatorPID(double i_k, double i_ti, double i_td)
    : m_k(i_k)
    , m_ti(i_ti)
    , m_td(i_td)
    , m_suma_uchybow(0.0)
    , m_poprzedni_uchyb(0.0)
    , m_tryb_calki(tryb_calki::StalaPodSuma)
    , m_u_min(-std::numeric_limits<double>::infinity())
    , m_u_max(std::numeric_limits<double>::infinity())
    , m_anti_windup_wl(true)
    , m_poprzednia_i(0.0)
{}

void RegulatorPID::reset()
{
    m_suma_uchybow = 0.0;
    m_poprzedni_uchyb = 0.0;
    m_poprzednia_i = 0.0;
}

void RegulatorPID::set_nastawy(double k, double ti, double td)
{
   m_k = k;
    setStalaCalk(ti);
    m_td = td;
}

void RegulatorPID::setStalaCalk(double i_ti)
{
    if (i_ti == 0.0 && m_ti != 0.0) {
        if (m_tryb_calki == tryb_calki::StalaPodSuma) {
            m_suma_uchybow = 0.0;
        } else {
            m_suma_uchybow = 0.0;
            m_poprzednia_i = 0.0;
        }
    }

    m_ti = i_ti;
}

void RegulatorPID::set_tryb_calki(tryb_calki tryb)
{
    if (tryb == m_tryb_calki) {
        return;
    }

    if (tryb == tryb_calki::StalaPodSuma) {
        if (m_ti != 0.0) {
            m_suma_uchybow = m_poprzednia_i * m_ti;
        } else {
            m_suma_uchybow = 0.0;
        }
    } else {
        m_suma_uchybow = m_poprzednia_i;
    }
    m_tryb_calki = tryb;
}

void RegulatorPID::set_ograniczenia(double u_min, double u_max)
{
    m_u_min = u_min;
    m_u_max = u_max;
}

void RegulatorPID::set_anti_windup(bool wl)
{
    m_anti_windup_wl = wl;
}


double RegulatorPID::symuluj(double uchyb)
{
    //Obliczanie składowej (P)
    double skladowa_p = m_k * uchyb;
    //zapamietanie skoladowej
    m_ostatnia_skladowa_p = skladowa_p;

    // Obliczanie składowej (D)
    double skladowa_d = m_td * (uchyb - m_poprzedni_uchyb);
    m_poprzedni_uchyb = uchyb; // Zapamiętuje uchyb na następny krok
    //zapamietanie skoladowej
    m_ostatnia_skladowa_d = skladowa_d;

    //Obliczanie składowej (I) z Anti-Windup
    double i = m_poprzednia_i; // Domyślnie jak w poprzednim kroku
    //zapamietanie skoladowej

    // Logika Anti-Windup
    double u_bez_nowej_calki = skladowa_p + m_poprzednia_i + skladowa_d;

    // Sprawdzamy nasycenie:

    bool nasycone_gora = (u_bez_nowej_calki >= m_u_max) && (uchyb > 0);

    bool nasycone_dol = (u_bez_nowej_calki <= m_u_min) && (uchyb < 0);

    //Aktualizacja I if(anti-windup is off)
    if (!m_anti_windup_wl || (!nasycone_gora && !nasycone_dol)) {


      //________________________ TUTAJ EJST COS NIE TAK ___________________

        if (m_ti > 0.0) { // Dzielenie przez zero (m_ti=0) całka off

            if (m_tryb_calki == tryb_calki::stala_przed_suma) {
                m_suma_uchybow += (1.0 / m_ti) * uchyb;
                i = m_suma_uchybow;

            } else {
                m_suma_uchybow += uchyb;
                i = (1.0 / m_ti) * m_suma_uchybow;

            }
        } else {
            // Jeśli m_ti = 0, całka off
            i = 0.0;
            //m_suma_uchybow = 0.0; //  Reset pamieci całki <----Tutaj
            // Błąd wzgldem instrukcji "nie zerować pamięci części całkującej"
        }//______

    m_poprzednia_i = i; // Save aktualna wartość I na next cykl
    m_ostatnia_skladowa_i = i;

    // Suma składowych
    double u_wyjscie = skladowa_p + i + skladowa_d;

    // Zastosowanie nasycenia (ograniczenia)
    if (u_wyjscie > m_u_max) {
        u_wyjscie = m_u_max;
    } else if (u_wyjscie < m_u_min) {
        u_wyjscie = m_u_min;
    }

    return u_wyjscie;
}
