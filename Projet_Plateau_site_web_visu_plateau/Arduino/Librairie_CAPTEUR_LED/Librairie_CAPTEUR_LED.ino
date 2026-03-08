// Auteur Adrien et léni
//programme test regle jeu echec
//config : speed serial 115200 / PIN_LED : a modifer en fonction arduino uno et esp32

//A FAIRE Problème
//
// Le plateau n'est pas dans le bon sens de Rotation
// FAIT OK : il n'y a pas de mode echec
// FAIT OK le mode en passant ne marche pas
// Le mode roque petit et grand
// FAIT OK le mode Promotion
// ajouter la double verification entre le tableau et les case avant de commencé
// ajouter un modde offset pour les capteurs
// MODE luminosité led
// MODE on commence par les blanc
// mode exercice on peut retrouver des placements interesent sur internet et la proposer a jouer
// mode fin de partie rouge
// ajouter quand le roi est simplement en echec mettre la case du roi en orange
// mode/fonction eccran
// Le mode de jeu contre le robot permet d'ajouter +/- de profondeur dans le jeu 1,2,3 voir 4 en fonction de la ram cela serait comme la difficulté du jeu
// activer ou non horloge
// activé un mode timer mode deconte et mode compte
// mode piece touché, piece jouée
//le calcule du robot est fait en permanance calculer une fois et copier...
//mode Robot obligatoir et pas obligatoir (jouer le coup proposé par le robot)
//petit probleme quand je leve il me propose en case bleu les coups mais quand le leve le pion adverse pour le manger ca case s'eteint...

#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "A31301.h"
#include "config.h"
#include "regle_echec.h"
#include "RobotEchec.h"


//-----------variables globales------------//

#define LED_PIN A4    // Broche GPIO de l'ESP32 --> A4 et Arduino UNO --> 2
#define LED_COUNT 64  // Nombre de leds par module


Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Fonction pour remplir les pixels un par un
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
    //strip.show();
    delay(wait);
  }
}

void setuLED(uint8_t addr_led, uint32_t color) {
  strip.setPixelColor(tab_LED[addr_led] - 1, color);
  //strip.show();
  //delay(10);
}

extern void calculerDeplacements(Piece &p);
void afficherPlateauSerial();
Piece plateau[8][8];
uint8_t memoire_plateau[64];  // État précédent (0:vide, 1:blanc, 2:noir)
int8_t caseSoulevee = -1;     // -1 si rien n'est levé, sinon 0-63
// Prototypes pour que l'Arduino reconnaisse les fonctions partout
bool estDansCoupPossible(uint8_t index);
uint8_t coordVersIndex(uint8_t x, uint8_t y);
void gererLeveePiece(uint8_t index);
void gererPosePiece(int8_t depart, uint8_t arrivee, uint8_t couleurDetectee);
void initPiece();
uint8_t coupPossible[28];
bool tourDesBlancs = true;  // Les blancs commencent toujours
CoupSPE SPE;
uint8_t coordSPE[4];
extern void setEnPassantTarget(int col, int row);
extern void clearEnPassantTarget();
enum EtatJeu { NORMAL,
               ECHEC,
               MAT,
               PAT };
EtatJeu verifierEtatDuJeu(Couleur campAuTrait);

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) {
    Serial.println("pas de serial");
  }
  Serial.println("Setup");
  Wire.begin();
  strip.begin();             // Initialise la communication avec les LEDs
  strip.show();              // Éteint tout au démarrage
  strip.setBrightness(100);  // Luminosité à environ 20% pour 50 (pour économiser le courant via USB)
  initPiece();

  // Initialiser la mémoire selon les pièces posées
  for (uint8_t i = 0; i < 64; i++) {
    if (presence_pion_blanc(i)) memoire_plateau[i] = 1;
    else if (presence_pion_noir(i)) memoire_plateau[i] = 2;
    else memoire_plateau[i] = 0;
  }

  clearEnPassantTarget();
}
int plateauN1[64];
int plateauN2[64];


void loop() {

  //---------Gestion des cases du plateau--------
  //Serial.println("-----------------");
  Serial.write(0xAA);
  Serial.write(0xBB);
  uint8_t checksum = 0;
  int16_t ValeurZ = 0;
  //Etat = 0 (Noir), 1 (Blanc), 2 (Rien)
  uint8_t etat = 0;

  //Serial.println("-----------------");

  //Serial.write(checksum);
  bool changementDetecte = false;
  int8_t caseAction = -1;
  uint8_t typeAction = 0;  // 0:rien, 1:levé, 2:posé

  UpdateLED();
  for (uint8_t i = 0; i < 64; i++) {
    uint8_t etatCapteur = 0;
    if (presence_pion_blanc(i)) etatCapteur = 1;
    else if (presence_pion_noir(i)) etatCapteur = 2;

    plateauN1[i] = etatCapteur;

    // --- LOGIQUE DE MOUVEMENT ---
    if (plateauN1[i] != plateauN2[i]) {

      // CAS 1 : ON SOULEVE UNE PIÈCE
      // (Le capteur devient vide ET la mémoire était pleine ET on n'a rien en main)
      if (plateauN1[i] == 0 && plateauN2[i] != 0 && caseSoulevee == -1) {
        caseSoulevee = i;
        gererLeveePiece(i);
        if (i == caseSoulevee && coupPossible[0] == 0) setuLED(i, strip.Color(255, 165, 0));       // on est sur la case soulevée cependant aucun coup n'est possible mettre en orange
        else if (i == caseSoulevee && coupPossible[0] == 100) setuLED(i, strip.Color(255, 0, 0));  // on est sur la case soulevée cependant ce n'est pas a lui de jouer mettre en rouge
        plateauN2[i] = plateauN1[i];
      } else if (plateauN1[i] == 0 && plateauN2[i] != 0 && caseSoulevee != -1) {
        setuLED(i, strip.Color(0, 0, 0));
      }

      // CAS 2 : ON POSE UNE PIÈCE SUR UNE CASE VIDE
      // (Le capteur devient plein ET la mémoire était vide ET on a une pièce en main)
      else if (plateauN1[i] != 0 && plateauN2[i] == 0 && caseSoulevee != -1) {
        if ((estDansCoupPossible(i) && coupPossible[0] != 100) || i == caseSoulevee) {
          gererPosePiece(caseSoulevee, i, etatCapteur);
          plateauN2[i] = plateauN1[i];
          caseSoulevee = -1;
          //coupPossible[0] = 0;
          if (i != caseSoulevee) {
            if (estDansCoupPossible(i) && coupPossible[0] != 100) {
              tourDesBlancs = !tourDesBlancs;  // ALTERNANCE
            }
            Serial.print("Tour suivant : ");
            Serial.println(tourDesBlancs ? "BLANC" : "NOIR");
          }
        } else setuLED(i, strip.Color(255, 0, 0));
      } else if (plateauN1[i] != 0 && plateauN2[i] == 0 && caseSoulevee == -1) {  // si on pase une piece et aucune n'est soulevé alors rouge ( en mode ajoute de piece)
        setuLED(i, strip.Color(255, 0, 0));
      }

      // CAS 3 : ON MANGE UNE PIÈCE (CAPTURE)
      // (Le capteur change de couleur ET on a une pièce en main ET c'est un coup possible)
      else if (plateauN1[i] != plateauN2[i] && plateauN1[i] != 0 && plateauN2[i] != 0 && caseSoulevee != -1) {
        if (estDansCoupPossible(i)) {
          // On appelle la même fonction : elle va écraser l'ancienne pièce par la nouvelle
          gererPosePiece(caseSoulevee, i, etatCapteur);
          plateauN2[i] = plateauN1[i];  // La mémoire enregistre la nouvelle couleur
          caseSoulevee = -1;
          coupPossible[0] = 0;
          Serial.println("Piece mangee !");
          tourDesBlancs = !tourDesBlancs;
        }
      }
    }
    // Dans le loop(), au moment où vous changez de tour [cite: 25, 31]
    if (caseSoulevee == -1 && tourDesBlancs == false) {

      //Serial.println(F("Au tour du Robot (Noir)..."));
      //delay(1000); // Petit délai pour le réalisme

      CoupRobot decision = calculerMeilleurCoup(NOIR);

      if (decision.x1 != -1) {
        /*Serial.print(F("Robot suggère : "));
        Serial.print(decision.x1);
        Serial.print(",");
        Serial.print(decision.y1);
        Serial.print(F(" -> "));
        Serial.print(decision.x2);
        Serial.print(",");
        Serial.println(decision.y2);*/

        // --- AFFICHAGE VISUEL ---
        // LED Violette sur la pièce à bouger [cite: 44]
        setuLED(coordVersIndex(decision.x1, decision.y1), strip.Color(0, 255, 255));
        // LED Cyan sur la case de destination [cite: 44]
        setuLED(coordVersIndex(decision.x2, decision.y2), strip.Color(0, 255, 255));
        strip.show();
      }
    }
  }
  strip.show();


  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // INTERROGER (ex: "1,6")
    if (cmd.length() == 3) {
      int x = cmd.substring(0, 1).toInt();
      int y = cmd.substring(2, 3).toInt();
      if (plateau[x][y].getType() == AUCUN) {
        Serial.println("Case vide.");
      } else {
        Serial.print("Selection: ");
        calculerDeplacements(plateau[x][y]);
      }
    }
    // --- COMMANDE VISUELLE ---
    if (cmd == "?") {
      afficherPlateauSerial();
    }
    // DEPLACER (ex: "1,6 1,4")
    else if (cmd.length() >= 7) {
      int x1 = cmd.substring(0, 1).toInt();
      int y1 = cmd.substring(2, 3).toInt();
      int x2 = cmd.substring(4, 5).toInt();
      int y2 = cmd.substring(6, 7).toInt();

      if (plateau[x1][y1].getType() != AUCUN) {
        plateau[x2][y2] = plateau[x1][y1];
        plateau[x2][y2].setPosition(x2, y2);
        plateau[x1][y1].vider();
        Serial.println("Coup effectue.");
      }
    }
    if (cmd == "GO") {
      plateau[2][4].reset(PION, NOIR, 2, 4);
    }
  }
}

void UpdateLED() {
  for (uint8_t i = 0; i < 64; i++) {
    uint8_t etatCapteur = 0;
    if (presence_pion_blanc(i)) etatCapteur = 1;
    else if (presence_pion_noir(i)) etatCapteur = 2;
    // --- LOGIQUE D'AFFICHAGE DES LEDS coup possible ---

    //------ pas de piece presente et casesoulevee-----//
    if (etatCapteur == 0) {
      if (estDansCoupPossible(i) && caseSoulevee != -1 && coupPossible[0] != 100) {
        setuLED(i, strip.Color(0, 0, 255));  // Bleu
      } else {
        setuLED(i, strip.Color(0, 0, 0));
      }
      if (coupPossible[0] == 100 && caseSoulevee == i) setuLED(i, strip.Color(255, 0, 0));           // Ce n'est pas a la bonne couleur de jouer --> rouge
      else if (coupPossible[0] != 100 && caseSoulevee == i) setuLED(i, strip.Color(255, 255, 255));  // Ce n'est pas a la bonne couleur de jouer --> rouge
    }
    //------ piece presente et casesoulevee-----//
    else if (etatCapteur != 0 && caseSoulevee != -1) {
      if (estDansCoupPossible(i) && coupPossible[0] != 100) {
        setuLED(i, strip.Color(0, 0, 255));  // Bleu
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

void initPiece() {

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

void afficherPlateauSerial() {
  Serial.println("\n    0    1    2    3    4    5    6    7   (X)");
  Serial.println("  +----+----+----+----+----+----+----+----+");

  for (int y = 0; y < 8; y++) {
    Serial.print(y);
    Serial.print(" | ");  // Indice de ligne Y
    for (int x = 0; x < 8; x++) {
      Piece &p = plateau[x][y];
      if (p.getType() == AUCUN) {
        Serial.print("  ");
      } else {
        // Initiale de la pièce (P, C, F, T, D, R)
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
        // Minuscule pour Noir, Majuscule pour Blanc
        if (p.getCouleur() == NOIR) c = c + 32;
        Serial.print(c);
        Serial.print((p.getCouleur() == BLANC) ? "b" : "n");  // b pour blanc, n pour noir
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
  Serial.print("Levee en: ");
  Serial.println(index);
  Serial.print("Levee en: ");
  Serial.print("X");
  Serial.print(x);
  Serial.print("Y");
  Serial.print(y);

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
          while (presence_pion_blanc(coordVersIndex(7 , BN)) || presence_pion_noir(coordVersIndex(7 , BN))); la condition ici ne passe pas a voir pk ?
          setuLED(coordVersIndex(7, BN), strip.Color(255, 128, 0));
          strip.show();
          while (!presence_pion_blanc(coordVersIndex(5 , BN)) && !presence_pion_noir(coordVersIndex(5 , BN)));
          modifier la case de la tour ca nouvelle coordonéé
          //while ();
          //plateau[x1][y1].vider();
          break;
        case GrandRoque:
          Serial.println("inverser le ROI et la tour");
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
          while (presence_pion_blanc(coordVersIndex(x2, y2 + BN)) || presence_pion_noir(coordVersIndex(x2, y2 + BN)));
          Serial.println("Next");
          plateau[x2][y2 + BN].vider();
          plateauN2[coordVersIndex(x2, y2 + BN)] = 0;
          break;
        case Promotion:

          Serial.println("Promotion choisir la piece : 1DAME | 2FOU | 3tour");
          while (Serial.available() == 0) {
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
      delay(2000);
    }
  } else {
    Serial.println("Erreur : La couleur ne correspond pas !");
  }
  plateau[x1][y1].vider();
}

uint8_t coordVersIndex(uint8_t x, uint8_t y) {
  // Formule : Index = x + (y * 8)
  // Exemple pour [3][1] : 3 + (1 * 8) = 11
  return x + (y * 8);
}


bool estDansCoupPossible(uint8_t index) {
  for (int c = 1; c <= coupPossible[0]; c++) {
    if (coupPossible[c] == index) return true;
  }
  return false;
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