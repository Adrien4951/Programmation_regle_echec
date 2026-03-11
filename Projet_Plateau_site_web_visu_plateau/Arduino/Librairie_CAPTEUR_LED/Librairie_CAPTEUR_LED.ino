// Auteur Adrien et léni
//programme test regle jeu echec
//config : speed serial 115200 / PIN_LED : a modifer en fonction arduino uno et esp32

//A FAIRE Problème
//
// Le plateau n'est pas dans le bon sens de Rotation
// FAIT OK : il n'y a pas de mode echec
// FAIT OK le mode en passant ne marche pas
// FAIT OK Le mode roque petit et grand
// FAIT OK le mode Promotion
// Faire une annimation pour le demarage
// ajouter la double verification entre le plateau et les cases avant de commencer
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
#include <LiquidCrystal_I2C.h>
#include "A31301.h"
#include "config.h"
#include "regle_echec.h"
#include "RobotEchec.h"
#include "mouvement_piece.h"
#include "ModeJEU.h"


//-----------variables globales------------//
//-----bouton-----//

int menuActuel = 0;
const int MAX_MENU = 3;  // Nombre d'options
String options[] = { "1. JOUER", "2. ROBOT", "3. TIMER", "4. EXIT" };




#define LED_PIN A4    // Broche GPIO de l'ESP32 --> A4 et Arduino UNO --> 2
#define LED_COUNT 64  // Nombre de leds par module
LiquidCrystal_I2C lcd(0x3F, 16, 2);

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Fonction pour remplir les pixels un par un
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
    //strip.show();
    delay(wait);
  }
}


uint8_t memoire_plateau[64];  // État précédent (0:vide, 1:blanc, 2:noir)
void initPiece();
uint8_t coordSPE[4];
extern void setEnPassantTarget(int col, int row);
extern void clearEnPassantTarget();

void setup() {
  // Initialisation de l'écran LCD

  Wire.begin();
  lcd.begin();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Echecs Ready");
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) {
    Serial.println("pas de serial");
  }
  Serial.println("Setup");

  strip.begin();             // Initialise la communication avec les LEDs
  strip.show();              // Éteint tout au démarrage
  strip.setBrightness(250);  // Luminosité à environ 20% pour 50 (pour économiser le courant via USB)
  delay(5000);
  Serial.println("offset");
  for (uint8_t i = 0; i < 64; i++) {
    offset(i);
  }

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

  // gererMenu();
  // fonction qui lance mode jeu tout y est
  UpdateLED();
  UpdateCapteur();
}


void UpdateCapteur() {
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

    if (caseSoulevee == -1 && tourDesBlancs == false) {
      CoupRobot decision = calculerMeilleurCoup(NOIR);
      if (decision.x1 != -1) {
        setuLED(coordVersIndex(decision.x1, decision.y1), strip.Color(0, 255, 255));
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
      initPiece();
      for (uint8_t i = 0; i < 64; i++) {
        if (presence_pion_blanc(i)) memoire_plateau[i] = 1;
        else if (presence_pion_noir(i)) memoire_plateau[i] = 2;
        else memoire_plateau[i] = 0;
      }

      clearEnPassantTarget();
    }
    if (cmd == "offset") {
      for (uint8_t i = 0; i < 64; i++) {
        offset(i);
      }
      Serial.println("Fin offset");
    }
  }
}

