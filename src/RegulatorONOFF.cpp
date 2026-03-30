#include "RegulatorONOFF.h"

RegulatorONOFF::RegulatorONOFF(double i_u_on, double i_h)
    : m_stan_on(false)
{
    set_nastawy(i_h, i_u_on);
}
//Reset regulatora
void RegulatorONOFF::reset()
{
    m_stan_on = false;
}

void RegulatorONOFF::set_nastawy(double h, double u_on)
{
    m_h = h;
    m_u_on = u_on;
}

double RegulatorONOFF::symuluj(double uchyb)
{
    // Reguly przelaczania
    if (m_stan_on == false && uchyb > m_h) {
        m_stan_on = true;
    } else if (m_stan_on == true && uchyb < -m_h) {
        m_stan_on = false;
    }

    if (m_stan_on) {
        return m_u_on;
    } else {
        return 0.0;
    }
}
