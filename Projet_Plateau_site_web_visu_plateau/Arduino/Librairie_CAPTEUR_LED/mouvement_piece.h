#ifndef MOUVEMENT_PIECE_H
#define MOUVEMENT_PIECE_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "regle_echec.h"
#include "RobotEchec.h"
#include "mouvement_piece.h"
#include "A31301.h"
// Types et énumérations
enum EtatJeu { NORMAL, ECHEC, MAT, PAT };

// Variables globales partagées
extern Piece plateau[8][8];
extern uint8_t coupPossible[28];
extern bool tourDesBlancs;
extern CoupSPE SPE;
extern int8_t caseSoulevee;
extern Adafruit_NeoPixel strip;
extern int plateauN2[64];

// Prototypes des fonctions demandées
void UpdateLED();
void afficherPlateauSerial();
void gererLeveePiece(uint8_t index);
void gererPosePiece(int8_t depart, uint8_t arrivee, uint8_t couleurPosee);
bool estDansCoupPossible(uint8_t index);
uint8_t coordVersIndex(uint8_t x, uint8_t y);
void validerPlacementPiece(int x, int y);
EtatJeu verifierEtatDuJeu(Couleur campAuTrait);

// Fonctions utilitaires LED
void setuLED(uint8_t addr_led, uint32_t color);

#endif