#include "ModeJEU.h"
#include "mouvement_piece.h"

extern void synchroniserPhysiqueEtAide();
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



void initExercice(int numero) {
  // 1. On vide tout le plateau par sécurité
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) plateau[x][y].vider();
  }

  // 2. Chargement de la configuration selon le numéro d'exercice
  switch (numero) {
    case 1: // MAT DU BERGER (Apprendre à défendre)
      plateau[4][0].reset(ROI, BLANC, 4, 0);   // Roi blanc en E1
      plateau[4][7].reset(ROI, NOIR, 4, 7);    // Roi noir en E8
      plateau[3][0].reset(DAME, BLANC, 3, 0);  // Dame blanche en D1
      plateau[5][0].reset(FOU, BLANC, 5, 0);   // Fou blanc en F1
      plateau[2][4].reset(DAME, BLANC, 2, 4);  // Dame blanche "agressive" en C5
      break;

    case 2: // FINALE ROI + PION (Apprendre l'opposition)
      plateau[4][4].reset(ROI, BLANC, 4, 4);   // Roi blanc en E5
      plateau[4][6].reset(ROI, NOIR, 4, 6);    // Roi noir en E7
      plateau[4][3].reset(PION, BLANC, 4, 3);  // Pion blanc en E4 (doit être promu)
      break;

    case 3: // LE BAISER DE LA MORT (Mat avec la Dame)
      plateau[6][6].reset(ROI, NOIR, 6, 6);    // Roi noir coincé en G7
      plateau[5][5].reset(ROI, BLANC, 5, 5);   // Roi blanc en F6
      plateau[5][6].reset(DAME, BLANC, 5, 6);  // Dame blanche en F7 (Echec et mat !)
      break;

    case 4: // MAT DU COULOIR (Attention à la dernière ligne)
      plateau[6][7].reset(ROI, NOIR, 6, 7);    // Roi noir en G8
      plateau[5][7].reset(PION, NOIR, 5, 7);   // Pions bloquants en F7, G7, H7
      plateau[6][6].reset(PION, NOIR, 6, 6);
      plateau[7][6].reset(PION, NOIR, 7, 6);
      plateau[0][7].reset(TOUR, BLANC, 0, 7);  // Tour blanche en A8 (Mat !)
      break;

    case 5: // FOURCHETTE DE CAVALIER
      plateau[4][4].reset(CAVALIER, BLANC, 4, 4); // Cavalier blanc en E5
      plateau[2][3].reset(ROI, NOIR, 2, 3);       // Roi noir en C4
      plateau[6][3].reset(DAME, NOIR, 6, 3);      // Dame noire en G4 (Fourchette !)
      break;
  }

  // 3. Mise à jour physique (LEDs et assistance de placement)
  synchroniserPhysiqueEtAide();
}