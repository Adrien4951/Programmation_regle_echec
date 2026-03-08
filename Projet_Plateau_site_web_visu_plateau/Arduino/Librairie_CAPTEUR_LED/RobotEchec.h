#ifndef ROBOTECHEC_H
#define ROBOTECHEC_H

#include <Arduino.h>
#include "regle_echec.h"

struct CoupRobot {
    int x1, y1, x2, y2;
    int score;
};

// Fonctions principales
CoupRobot calculerMeilleurCoup(Couleur camp);
int evaluerPosition();
void suggererCoupRobot();

#endif