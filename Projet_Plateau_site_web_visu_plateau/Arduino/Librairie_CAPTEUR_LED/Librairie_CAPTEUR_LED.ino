// Auteur Adrien et léni
//programme test regle jeu echec
//config : speed serial 115200 / PIN_LED : a modifer en fonction arduino uno et esp32


//test
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "A31301.h"
#include "config.h"



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

enum Couleur { VIDE,
               BLANC,
               NOIR };

enum TypePiece { AUCUN,
                 PION,
                 CAVALIER,
                 FOU,
                 TOUR,
                 DAME,
                 ROI };

class Piece {
private:
  TypePiece type;
  Couleur couleur;
  bool active;
  int x, y;            // Coordonnées (0 à 7)
  int nbDeplacements;  // Pour gérer le premier pas du pion et le roque

public:
  // Constructeur par défaut
  Piece()
    : type(AUCUN), couleur(VIDE), active(false), x(-1), y(-1), nbDeplacements(0) {}

  // Constructeur complet
  Piece(TypePiece t, Couleur c, int posX, int posY) {
    type = t;
    couleur = c;
    x = posX;
    y = posY;
    active = true;
    nbDeplacements = 0;
  }

  // --- Getters ---
  TypePiece getType() {
    return type;
  }
  Couleur getCouleur() {
    return couleur;
  }
  bool estActive() {
    return active;
  }
  int getX() {
    return x;
  }
  int getY() {
    return y;
  }
  int getNbDeplacements() {
    return nbDeplacements;
  }

  // --- Setters ---
  void setPosition(int newX, int newY) {
    x = newX;
    y = newY;
    nbDeplacements++;
  }

  void setActive(bool etat) {
    active = etat;
  }

  // Utile pour le reset du jeu
  void reset(TypePiece t, Couleur c, int posX, int posY) {
    type = t;
    couleur = c;
    x = posX;
    y = posY;
    active = true;
    nbDeplacements = 0;
  }
  void vider() {
    type = AUCUN;
    couleur = VIDE;
    active = false;
    nbDeplacements = 0;
    // x et y peuvent rester ou être mis à -1
  }
};
void calculerDeplacements(Piece &p);
void afficherPlateauSerial();
Piece plateau[8][8];
uint8_t memoire_plateau[64];  // État précédent (0:vide, 1:blanc, 2:noir)
int8_t caseSoulevee = -1;     // -1 si rien n'est levé, sinon 0-63
// Prototypes pour que l'Arduino reconnaisse les fonctions partout
bool estDansCoupPossible(uint8_t index);
uint8_t coordVersIndex(uint8_t x, uint8_t y);
void gererLeveePiece(uint8_t index);
void gererPosePiece(int8_t depart, uint8_t arrivee, uint8_t couleurDetectee);

uint8_t coupPossible[28];

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


  for (uint8_t i = 0; i < 64; i++) {
    uint8_t etatCapteur = 0;
    if (presence_pion_blanc(i)) etatCapteur = 1;
    else if (presence_pion_noir(i)) etatCapteur = 2;

    plateauN1[i] = etatCapteur;

    // --- LOGIQUE D'AFFICHAGE DES LEDS ---
    if (etatCapteur == 0) {
      if (i == caseSoulevee && coupPossible[0] == 0 ) setuLED(i, strip.Color(255, 165, 0)); // on est sur la case soulevée cependant aucun coup n'est possible mettre en orange   
      else if (estDansCoupPossible(i) && caseSoulevee != -1) {
        setuLED(i, strip.Color(0, 0, 255)); // Bleu 
      } else {
        setuLED(i, strip.Color(0, 0, 0)); 
      }
    } 
    else {
      if (caseSoulevee != -1) { // on est en mode piece soulevee

        if (i == caseSoulevee || estDansCoupPossible(i) || plateauN2[i] != 0) {
          uint32_t couleur = (etatCapteur == 1) ? strip.Color(255, 255, 255) : strip.Color(255, 255, 0);
          if (estDansCoupPossible(i)) couleur = strip.Color(0, 0, 255);
          Serial.println("coup possible : "+ String(coupPossible[0]));
          setuLED(i, couleur);
        } else {
          setuLED(i, strip.Color(255, 0, 0)); // ROUGE (Erreur de pose)
        }
      } else {
        uint32_t couleur = (etatCapteur == 1) ? strip.Color(255, 255, 255) : strip.Color(255, 255, 0);
        setuLED(i, couleur);
      }
    }

// --- LOGIQUE DE MOUVEMENT ---
if (plateauN1[i] != plateauN2[i]) {

  // CAS 1 : ON SOULEVE UNE PIÈCE
  if (plateauN1[i] == 0 && plateauN2[i] != 0 && caseSoulevee == -1) {
    
    // VERIFICATION DU TOUR
    // plateauN2[i] contient la couleur de la pièce qui était là (1=Blanc, 2=Noir)
    if ((tourDesBlancs && plateauN2[i] == 1) || (!tourDesBlancs && plateauN2[i] == 2)) {
      caseSoulevee = i;
      gererLeveePiece(i); // Calcule les coups normalement
    } else {
      caseSoulevee = i;
      coupPossible[0] = 0; // TOUR ADVERSE : on vide les coups (la case passera en Orange)
      Serial.println("Ce n'est pas votre tour !");
    }
    plateauN2[i] = plateauN1[i]; 
  } 

  // CAS 2 : ON POSE UNE PIÈCE (VIDE)
  else if (plateauN1[i] != 0 && plateauN2[i] == 0 && caseSoulevee != -1) {
    if (estDansCoupPossible(i) || i == caseSoulevee) {
      
      // Si on a réellement bougé (pas une annulation), on change de tour
      if (i != caseSoulevee) {
        tourDesBlancs = !tourDesBlancs; // ALTERNANCE
        Serial.print("Tour suivant : "); Serial.println(tourDesBlancs ? "BLANC" : "NOIR");
      }

      gererPosePiece(caseSoulevee, i, etatCapteur);
      plateauN2[i] = plateauN1[i];
      caseSoulevee = -1;
      coupPossible[0] = 0; // Je le décommente pour nettoyer l'affichage
    }
  }

  // CAS 3 : ON MANGE UNE PIÈCE (CAPTURE)
  else if (plateauN1[i] != plateauN2[i] && plateauN1[i] != 0 && plateauN2[i] != 0 && caseSoulevee != -1) {
    if (estDansCoupPossible(i)) {
      
      tourDesBlancs = !tourDesBlancs; // ALTERNANCE après capture
      
      gererPosePiece(caseSoulevee, i, etatCapteur); 
      plateauN2[i] = plateauN1[i];
      caseSoulevee = -1;
      coupPossible[0] = 0;
      Serial.println("Piece mangee ! Changement de tour.");
    }
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

void calculerDeplacements(Piece &p) {
  int x = p.getX();
  int y = p.getY();
  int nbrDeplacment = p.getNbDeplacements();
  TypePiece t = p.getType();
  Couleur maCouleur = p.getCouleur();

  // Directions : {dx, dy}
  int dirCavalier[8][2] = { { -2, 1 }, { -1, 2 }, { 1, 2 }, { 2, 1 }, { 2, -1 }, { 1, -2 }, { -1, -2 }, { -2, -1 } };
  int dirRook[4][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } };
  int dirBishop[4][2] = { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };


  uint8_t cassePossible = 0;
  int X[40];
  int Y[40];
  // --- PION ---
  // --- PION ---
  if (t == PION) {
    int dirY = (maCouleur == BLANC) ? 1 : -1;

    // Avance simple
    if (y + dirY >= 0 && y + dirY <= 7 && plateau[x][y + dirY].getType() == AUCUN) {
      X[cassePossible] = x;
      Y[cassePossible] = y + dirY;
      cassePossible++;

      if (nbrDeplacment == 0 && plateau[x][y + 2 * dirY].getType() == AUCUN) {
        X[cassePossible] = x;
        Y[cassePossible] = y + 2 * dirY;
        cassePossible++;
      }
    }
    if (y + dirY >= 0 && y + dirY <= 7 && x + dirY >= 0 && x + dirY <= 7 && plateau[x + dirY][y + dirY].getType() != AUCUN && plateau[x + dirY][y + dirY].getCouleur() != maCouleur) {
      X[cassePossible] = x + dirY;
      Y[cassePossible] = y + dirY;
      cassePossible++;
    }

    if (y + dirY >= 0 && y + dirY <= 7 && x - dirY >= 0 && x - dirY <= 7 && plateau[x - dirY][y + dirY].getType() != AUCUN && plateau[x - dirY][y + dirY].getCouleur() != maCouleur) {
      X[cassePossible] = x - dirY;
      Y[cassePossible] = y + dirY;
      cassePossible++;
    }
  }

  if (t == FOU) {
    int directions[4][2] = { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };
    for (int d = 0; d < 4; d++) {
      for (int i = 1; i < 8; i++) {
        int nx = x + i * directions[d][0];
        int ny = y + i * directions[d][1];

        // 1. Vérifier si on sort du plateau
        if (nx < 0 || nx > 7 || ny < 0 || ny > 7) break;

        // 2. Case vide : on ajoute et on continue
        if (plateau[nx][ny].getType() == AUCUN) {
          X[cassePossible] = nx;
          Y[cassePossible] = ny;
          cassePossible++;
        }
        // 3. Pièce adverse : on ajoute la capture puis on s'arrête
        else if (plateau[nx][ny].getCouleur() != maCouleur) {
          X[cassePossible] = nx;
          Y[cassePossible] = ny;
          cassePossible++;
          break;
        }
        // 4. Pièce alliée : on s'arrête immédiatement
        else {
          break;
        }
      }
    }
  }

  Serial.println("Coup possibleeeee:");
  Serial.println(cassePossible);
  //strip.show();
  coupPossible[0] = cassePossible;
  for (int i = 0; i < cassePossible; i++) {
    Serial.print("X: ");
    Serial.print(X[i]);
    Serial.print(" | Y: ");
    Serial.println(Y[i]);
    setuLED((X[i] + (Y[i] * 8)), strip.Color(255, 0, 0));
    coupPossible[i + 1] = coordVersIndex(X[i], Y[i]);
  }
  strip.show();
}

void initPiece() {

  plateau[3][1].reset(FOU, BLANC, 3, 1);
  plateau[3][7].reset(FOU, NOIR, 3, 7);
  plateau[4][6].reset(PION, BLANC, 4, 6);
  plateau[5][2].reset(PION, NOIR, 5, 2);

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

  if (plateau[x][y].getType() != AUCUN) {
    calculerDeplacements(plateau[x][y]);
  }
}
void gererPosePiece(int8_t depart, uint8_t arrivee, uint8_t couleurPosee) {
  int x1 = depart % 8; int y1 = depart / 8;
  int x2 = arrivee % 8; int y2 = arrivee / 8;

  // On vérifie si la couleur détectée par le capteur 
  // correspond bien à la couleur de la pièce qu'on a soulevée
  if (couleurPosee == plateau[x1][y1].getCouleur()) {
      // Transfert des données
      plateau[x2][y2] = plateau[x1][y1];
      plateau[x2][y2].setPosition(x2, y2);
      plateau[x1][y1].vider();
      Serial.println("Mouvement synchronisé !");
  } else {
      Serial.println("Erreur : La couleur ne correspond pas !");
  }
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
