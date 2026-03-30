#pragma once

class Regulator {
public:
   //virualny destruktor
    virtual ~Regulator() {}

    virtual double symuluj(double uchyb) = 0; // wirtualna funkcja

    virtual void reset() = 0;
};