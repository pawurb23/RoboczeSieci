#include "Kontroler.h"

Kontroler::Kontroler()
{
    m_uar = new ProstyUAR();
    m_generator = new Generator();
}

Kontroler::~Kontroler()
{
    delete m_uar;
    delete m_generator;
}

void Kontroler::symuluj_krok(double krok_czasu)
{
    double czas = m_uar->get_aktualny_czas();
    double zadana = m_generator->oblicz_wartosc(czas);

    // ProstyUAR zostanie zaktualizowany, aby przyjmowac zadana
    m_uar->symuluj_krok(krok_czasu, zadana);
}

void Kontroler::reset()
{
    if (m_uar)
        m_uar->reset();
    // Generator nie wymaga resetu stanu, bo jest bezstanowy (zalezy od czasu przekazywanego z zewnatrz/UAR)
}

double Kontroler::get_aktualny_czas() const
{
    if (m_uar)
        return m_uar->get_aktualny_czas();
    return 0.0;
}

double Kontroler::get_wyjscie() const
{
    if (m_uar)
        return m_uar->get_wyjscie();
    return 0.0;
}

double Kontroler::get_sterowanie() const
{
    if (m_uar)
        return m_uar->get_sterowanie();
    return 0.0;
}

double Kontroler::get_uchyb() const
{
    if (m_uar)
        return m_uar->get_uchyb();
    return 0.0;
}

double Kontroler::get_wartosc_zadana() const
{
    if (m_uar)
        return m_uar->get_wartosc_zadana();
    return 0.0;
}

double Kontroler::get_skladowa_p() const
{
    if (m_uar)
        return m_uar->get_skladowa_p();
    return 0.0;
}

double Kontroler::get_skladowa_i() const
{
    if (m_uar)
        return m_uar->get_skladowa_i();
    return 0.0;
}

double Kontroler::get_skladowa_d() const
{
    if (m_uar)
        return m_uar->get_skladowa_d();
    return 0.0;
}
