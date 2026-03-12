#include "mouvement_piece.h"
#include "Arduino.h"
#include <Arduino.h>
#include <LiquidCrystal_I2C.h> // INDISPENSABLE ici
#include "regle_echec.h"       // Pour TypePiece et Piece
extern bool paramAide;
extern void traiterBoutons();
// Dans mouvement_piece.cpp, ne mettez PAS les paramètres (0x3F, 16, 2)
extern LiquidCrystal_I2C lcd;
enum Etape {
    INITIALISATION,
    EN_JEU,
    FIN_DE_PARTIE
};
extern Etape etapeActuelle;
//extern int etapeActuelle; // Remplacez int par le type réel (enum ?)
//extern const int EN_JEU;  // Ou la valeur correspondante
// Définition des variables extern
Piece plateau[8][8];
uint8_t coupPossible[28];
bool tourDesBlancs = true;
CoupSPE SPE = Rien;
int8_t caseSoulevee = -1;

void setuLED(uint8_t addr_led, uint32_t color) {
  strip.setPixelColor(tab_LED[addr_led] - 1, color);
}

uint8_t coordVersIndex(uint8_t x, uint8_t y) {
  return x + (y * 8);
}

bool estDansCoupPossible(uint8_t index) {
  for (int c = 1; c <= coupPossible[0]; c++) {
    if (coupPossible[c] == index) return true;
  }
  return false;
}

void UpdateLED() {
  for (uint8_t i = 0; i < 64; i++) {
    traiterBoutons();
    uint8_t etatCapteur = 0;
    if (presence_pion_blanc(i)) etatCapteur = 1;
    else if (presence_pion_noir(i)) etatCapteur = 2;
    // --- LOGIQUE D'AFFICHAGE DES LEDS coup possible ---

    //------ pas de piece presente et casesoulevee-----//
    if (etatCapteur == 0) {
      if (estDansCoupPossible(i) && caseSoulevee != -1 && coupPossible[0] != 100) {
        if(paramAide){
          setuLED(i, strip.Color(0, 0, 255));  // Bleu
        }
        
      } else {
        setuLED(i, strip.Color(0, 0, 0));
      }
      if (coupPossible[0] == 100 && caseSoulevee == i) setuLED(i, strip.Color(255, 0, 0));           // Ce n'est pas a la bonne couleur de jouer --> rouge
      else if (coupPossible[0] != 100 && caseSoulevee == i) setuLED(i, strip.Color(255, 255, 255));  // Ce n'est pas a la bonne couleur de jouer --> rouge
    }
    //------ piece presente et casesoulevee-----//
    else if (etatCapteur != 0 && caseSoulevee != -1) {
      if (estDansCoupPossible(i) && coupPossible[0] != 100) {
         if(paramAide){
          setuLED(i, strip.Color(0, 0, 255));  // Bleu
        }
      } else {
        setuLED(i, strip.Color(25, 25, 25));
      }
    }
    //------ piece presente et pas de casesoulevee-----//
    else if (etatCapteur != 0 && caseSoulevee == -1) {
      //Serial.println("ok");
      if (etatCapteur == 1 && tourDesBlancs == true) {
        setuLED(i, strip.Color(255, 255, 255));
        //Serial.println("mettre en blanc");
      } else if (etatCapteur == 2 && tourDesBlancs == false) {
        setuLED(i, strip.Color(255, 255, 255));
      } else setuLED(i, strip.Color(25, 25, 25));
    } else setuLED(i, strip.Color(0, 0, 0));
  }
}

void afficherPlateauSerial() {
  Serial.println("\n    0    1    2    3    4    5    6    7   (X)");
  Serial.println("  +----+----+----+----+----+----+----+----+");
  for (int y = 0; y < 8; y++) {
    Serial.print(y);
    Serial.print(" | ");
    for (int x = 0; x < 8; x++) {
      Piece &p = plateau[x][y];
      if (p.getType() == AUCUN) {
        Serial.print("  ");
      } else {
        char c;
        switch (p.getType()) {
          case PION: c = 'P'; break;
          case CAVALIER: c = 'C'; break;
          case FOU: c = 'F'; break;
          case TOUR: c = 'T'; break;
          case DAME: c = 'D'; break;
          case ROI: c = 'R'; break;
          default: c = ' '; break;
        }
        if (p.getCouleur() == NOIR) c = c + 32;
        Serial.print(c);
        Serial.print((p.getCouleur() == BLANC) ? "b" : "n");
      }
      Serial.print(" | ");
    }
    Serial.println();
    Serial.println("  +----+----+----+----+----+----+----+----+");
  }
}

void gererLeveePiece(uint8_t index) {
  int x = index % 8;
  int y = index / 8;
  if (plateau[x][y].getType() != AUCUN) {
    calculerDeplacements(plateau[x][y]);
  }
}

void gererPosePiece(int8_t depart, uint8_t arrivee, uint8_t couleurPosee) {
  int x1 = depart % 8;
  int y1 = depart / 8;
  int x2 = arrivee % 8;
  int y2 = arrivee / 8;

  // On vérifie si la couleur détectée par le capteur
  // correspond bien à la couleur de la pièce qu'on a soulevée
  if (depart == arrivee) return;

  if (couleurPosee == plateau[x1][y1].getCouleur()) {
    // Transfert des données
    plateau[x2][y2] = plateau[x1][y1];
    plateau[x2][y2].setPosition(x2, y2);
    //plateau[x1][y1].vider();
    Serial.println("Mouvement synchronisé !");
    //----gerer le en passant------//
    Piece &p = plateau[x1][y1];
    if (p.getType() == PION && ((plateau[x2][y2].getY() == 7 && plateau[x2][y2].getCouleur() == BLANC) || (plateau[x2][y2].getY() == 0 && plateau[x2][y2].getCouleur() == NOIR))) SPE = Promotion;
    if (p.getType() == PION && (y2 - y1 == 2 || y2 - y1 == -2)) {
      setEnPassantTarget(x2, (y1 + y2) / 2);
    } else {
      clearEnPassantTarget();
    }

    if (SPE != Rien) {
      int BN;  //sens BLANC ou NOIR
      switch (SPE) {
        case PetitRoque:
          if (plateau[x1][y1].getCouleur() == BLANC) BN = 0;
          else BN = 7;
          Serial.println("inverser le ROI et la tour");
          setuLED(coordVersIndex(7, BN), strip.Color(255, 128, 0));
          //setuLED(coordVersIndex(5, BN), strip.Color(255, 255, 255));
          strip.show();
          while (presence_pion_blanc(coordVersIndex(7, BN)) || presence_pion_noir(coordVersIndex(7, BN)))
            ;  //la condition ici ne passe pas a voir pk ?
          setuLED(coordVersIndex(5, BN), strip.Color(255, 128, 0));
          setuLED(coordVersIndex(7, BN), strip.Color(0, 0, 0));
          strip.show();
          while (!presence_pion_blanc(coordVersIndex(5, BN)) && !presence_pion_noir(coordVersIndex(5, BN)))
            ;
          plateau[5][BN] = plateau[7][BN];
          plateau[5][BN].setPosition(5, BN);
          plateau[7][BN].vider();
          break;

        case GrandRoque:
          Serial.println("inverser le ROI et la tour");

          if (plateau[x1][y1].getCouleur() == BLANC) BN = 0;
          else BN = 7;
          Serial.println("inverser le ROI et la tour");
          setuLED(coordVersIndex(0, BN), strip.Color(255, 128, 0));
          //setuLED(coordVersIndex(5, BN), strip.Color(255, 255, 255));
          strip.show();
          while (presence_pion_blanc(coordVersIndex(0, BN)) || presence_pion_noir(coordVersIndex(0, BN)))
            ;
          setuLED(coordVersIndex(0, BN), strip.Color(255, 128, 0));
          strip.show();
          while (!presence_pion_blanc(coordVersIndex(3, BN)) && !presence_pion_noir(coordVersIndex(3, BN)))
            ;
          plateau[3][BN] = plateau[0][BN];
          plateau[3][BN].setPosition(5, BN);
          plateau[0][BN].vider();
          break;

        case EnPassant:
          Serial.println("En Passant");
          Serial.println("index:");
          Serial.println(coordVersIndex(x2, y2 + BN));
          if (plateau[x1][y1].getCouleur() == BLANC) BN = -1;
          else BN = 1;
          setuLED(coordVersIndex(x2, y2 + BN), strip.Color(255, 0, 0));
          strip.show();
          delay(2000);
          while (presence_pion_blanc(coordVersIndex(x2, y2 + BN)) || presence_pion_noir(coordVersIndex(x2, y2 + BN)))
            ;
          Serial.println("Next");
          plateau[x2][y2 + BN].vider();
          plateauN2[coordVersIndex(x2, y2 + BN)] = 0;
          break;
        case Promotion:

          Serial.println("Promotion choisir la piece : 1DAME | 2FOU | 3tour");
          uint16_t teinte = 0;  // La teinte va de 0 à 65535
          while (Serial.available() == 0) {
            uint32_t couleurArcEnCiel = strip.ColorHSV(teinte);
            // On utilise ta fonction setuLED sur la case 5
            setuLED(coordVersIndex(x2, y2), couleurArcEnCiel);
            strip.show();   // Indispensable pour rafraîchir la couleur
            teinte += 256;  // Vitesse de changement (plus c'est haut, plus c'est rapide)
            delay(10);      // Petit délai pour que l'œil puisse voir l'effet
          }
          String cmd = Serial.readStringUntil('\n');
          cmd.trim();
          if (cmd == "1") {
            plateau[x2][y2].reset(DAME, plateau[x2][y2].getCouleur(), x2, y2);
          } else if (cmd == "2") {
            plateau[x2][y2].reset(FOU, plateau[x2][y2].getCouleur(), x2, y2);
          } else if (cmd == "3") {
            plateau[x2][y2].reset(TOUR, plateau[x2][y2].getCouleur(), x2, y2);
          }
          break;

          break;
      }
      SPE = Rien;
    }
    //verif echec
    Couleur prochainCamp = !tourDesBlancs ? BLANC : NOIR;
    EtatJeu etat = verifierEtatDuJeu(prochainCamp);

    if (etat == MAT) {
      // Faire clignoter toutes les LEDs en rouge ou afficher un message [cite: 26]
      for (int i = 0; i < 64; i++) setuLED(i, strip.Color(255, 0, 0));
      strip.show();
      delay(20000);
    }
  } else {
    Serial.println("Erreur : La couleur ne correspond pas !");
  }
  plateau[x1][y1].vider();
}

//-------------mode echec----------------//


EtatJeu verifierEtatDuJeu(Couleur campAuTrait) {
  int roiX = -1, roiY = -1;
  Couleur adversaire = (campAuTrait == BLANC) ? NOIR : BLANC;

  // 1. Trouver la position du Roi du camp au trait
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      if (plateau[x][y].getType() == ROI && plateau[x][y].getCouleur() == campAuTrait) {
        roiX = x;
        roiY = y;
        break;
      }
    }
  }

  // 2. Vérifier si le Roi est attaqué (Échec)
  bool enEchec = estCaseAttaquee(roiX, roiY, adversaire);

  // 3. Vérifier s'il existe au moins UN coup légal pour ce camp
  bool auMoinsUnCoupPossible = false;
  int tempX[40], tempY[40];

  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 8; y++) {
      if (plateau[x][y].getCouleur() == campAuTrait) {
        // Cette fonction filtre déjà les coups qui laissent le roi en échec
        int nb = genererCoupsPossibles(plateau[x][y], tempX, tempY);
        if (nb > 0) {
          auMoinsUnCoupPossible = true;
          break;
        }
      }
    }
    if (auMoinsUnCoupPossible) break;
  }

  // 4. Synthèse des résultats
  if (enEchec) {
    if (!auMoinsUnCoupPossible) {
      Serial.println(F("ECHEC ET MAT !"));
      return MAT;
    } else {
      Serial.println(F("ECHEC AU ROI !"));
      return ECHEC;
    }
  } else {
    if (!auMoinsUnCoupPossible) {
      Serial.println(F("PAT (Egalité) !"));
      return PAT;
    }
  }

  return NORMAL;
}
// --- Helper function (doit être EN DEHORS de validerPlacementPiece) ---
const char* nomTypePiece(TypePiece t) {
    switch (t) {
        case PION:     return "Pion";
        case CAVALIER: return "Cavalier";
        case FOU:      return "Fou";
        case TOUR:     return "Tour";
        case DAME:     return "Dame";
        case ROI:      return "Roi";
        default:       return "Vide";
    }
}
void validerPlacementPiece(int x, int y) {
  int index = x + (y * 8);
  Piece &p = plateau[x][y]; // Référence à la pièce dans ton tableau

  // 1. Vérifier s'il y a une pièce prévue à cet endroit
  if (!p.estActive()) {
    return; // Pas de pièce prévue, on sort
  }

  // 2. Affichage sur le LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Placez: ");
  lcd.print(nomTypePiece(p.getType())); // Utilise une petite fonction helper
  lcd.setCursor(0, 1);
  lcd.print(p.getCouleur() == BLANC ? "BLANC" : "NOIR");
  lcd.print(" en ");
  lcd.print((char)('A' + x)); // Convertit 0 en 'A', 1 en 'B'...
  lcd.print(y + 1);

  // 3. Boucle d'attente (Blocker)
  bool correct = false;
  while (!correct) {
    // Mettre la LED en ROUGE pour signaler l'attente
    setuLED(index, strip.Color(255, 0, 0));
    strip.show();

    // Lecture des capteurs physiques
    bool blancPresent = presence_pion_blanc(index);
    bool noirPresent = presence_pion_noir(index);

    // Vérification de la correspondance couleur
    if (p.getCouleur() == BLANC && blancPresent) {
      correct = true;
    } 
    else if (p.getCouleur() == NOIR && noirPresent) {
      correct = true;
    }

    // IMPORTANT : On appelle traiterBoutons pour pouvoir annuler/quitter le menu
    traiterBoutons();
    //if (etapeActuelle != EN_JEU) return; 

    delay(100); // Petite pause pour ne pas surcharger le processeur
  }

  // 4. Validation visuelle : on passe la LED en VERT puis couleur normale
  setuLED(index, strip.Color(0, 255, 0));
  strip.show();
  delay(500);
  
  // Remettre la couleur d'origine (Blanc ou Jaune)
  if (p.getCouleur() == BLANC) setuLED(index, strip.Color(255, 255, 255));
  else setuLED(index, strip.Color(255, 255, 0));
  strip.show();
}


void synchroniserPhysiqueEtAide() {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (plateau[x][y].estActive()) {
        // Cette fonction qu'on a fait avant va allumer la LED en rouge
        // et attendre que le joueur pose la bonne pièce au bon endroit.
        validerPlacementPiece(x, y); 
      } else {
        // Éteindre les cases vides
        int index = x + (y * 8);
        setuLED(index, strip.Color(0, 0, 0));
        plateauN2[index] = 0;
      }
    }
  }
  strip.show();
}
