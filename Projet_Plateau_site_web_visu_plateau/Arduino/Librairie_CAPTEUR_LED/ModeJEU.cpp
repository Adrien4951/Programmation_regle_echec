#include "ModeJEU.h"


void initPiece() {


  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      plateau[x][y].vider();
    }
  }
  for (int i = 0; i < 8; i++) {
    plateau[i][1].reset(PION, BLANC, i, 1);
    plateau[i][6].reset(PION, NOIR, i, 6);
  }
  plateau[0][0].reset(TOUR, BLANC, 0, 0);
  plateau[1][0].reset(CAVALIER, BLANC, 1, 0);
  plateau[2][0].reset(FOU, BLANC, 2, 0);
  plateau[3][0].reset(DAME, BLANC, 3, 0);
  plateau[4][0].reset(ROI, BLANC, 4, 0);
  plateau[5][0].reset(FOU, BLANC, 5, 0);
  plateau[6][0].reset(CAVALIER, BLANC, 6, 0);
  plateau[7][0].reset(TOUR, BLANC, 7, 0);

  plateau[0][7].reset(TOUR, NOIR, 0, 7);
  plateau[1][7].reset(CAVALIER, NOIR, 1, 7);
  plateau[2][7].reset(FOU, NOIR, 2, 7);
  plateau[3][7].reset(DAME, NOIR, 3, 7);
  plateau[4][7].reset(ROI, NOIR, 4, 7);
  plateau[5][7].reset(FOU, NOIR, 5, 7);
  plateau[6][7].reset(CAVALIER, NOIR, 6, 7);
  plateau[7][7].reset(TOUR, NOIR, 7, 7);

  for (uint8_t k = 0; k < 8; k++) {
    for (uint8_t j = 0; j < 8; j++) {
      uint8_t i = j + (k * 8);
      uint8_t etatActuel = 0;
      //plateauN1[i] =
      // 1. Détection de l'état actuel
      if (presence_pion_blanc(i)) {
        setuLED(i, strip.Color(255, 255, 255));
        plateauN2[i] = 1;
      } else if (presence_pion_noir(i)) {
        setuLED(i, strip.Color(255, 255, 0));
        plateauN2[i] = 2;
      } else {
        setuLED(i, strip.Color(0, 0, 0));
        plateauN2[i] = 0;
      }
    }
  }
}