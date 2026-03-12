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
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
using namespace websockets;

const char* WIFI_SSID = "PC_Leni";
const char* WIFI_PASSWORD = "1234445CCCCC";


const char* SERVER_IP = "192.168.137.1";
const uint16_t SERVER_PORT = 5678;

WebsocketsClient ws;  // client WebSocket global

//----------------bouton------------------//
// Définition des pins pour les 5 boutons
const int pinsBoutons[5] = { 12, 4, 16, 17, 39 };  //12 = gauche, 4 ok, 16 droite

// Variables de flags (volatile car modifiées dans une interruption)
// Temps anti-rebond en millisecondes (30ms à 50ms suffisent généralement)
const unsigned long DEBOUNCE_DELAY = 250;
volatile unsigned long dernierTempsInterruption[5] = { 0, 0, 0, 0, 0 };
volatile bool boutonPresse[5] = { false, false, false, false, false };


enum Etape { MENU_PRINCIPAL,
             SOUS_MENU,
             EN_JEU,
             TIMER };
Etape etapeActuelle = MENU_PRINCIPAL;

// Navigation
int indexMode = 0;   // 0:1J, 1:2J, 2:Calib, 3:EnLigne, 4:Exo
int indexParam = 0;  // Curseur dans le sous-menu
bool modeEdition = false;

const char* modes[] = { "1 Joueur", "2 Joueurs", "Calibration", "En Ligne", "Exercice" };
int profondeur = 3;
// Paramètres (on utilise des variables globales pour les stocker)
int paramDiff = 1;      // 1 à 3
int paramTemps = 10;    // en minutes
bool paramAide = true;  // Aide couleur

// Fonctions d'interruption (une par bouton ou une générique)
// IRAM_ATTR place la fonction dans la RAM rapide de l'ESP32
void IRAM_ATTR isrBouton0() {
  unsigned long tempsActuel = millis();
  if (tempsActuel - dernierTempsInterruption[0] > DEBOUNCE_DELAY) {
    boutonPresse[0] = true;
    dernierTempsInterruption[0] = tempsActuel;
  }
}
void IRAM_ATTR isrBouton1() {
  unsigned long tempsActuel = millis();
  if (tempsActuel - dernierTempsInterruption[1] > DEBOUNCE_DELAY) {
    boutonPresse[1] = true;
    dernierTempsInterruption[1] = tempsActuel;
  }
}
void IRAM_ATTR isrBouton2() {
  unsigned long tempsActuel = millis();
  if (tempsActuel - dernierTempsInterruption[2] > DEBOUNCE_DELAY) {
    boutonPresse[2] = true;
    dernierTempsInterruption[2] = tempsActuel;
  }
}
void IRAM_ATTR isrBouton3() {
  boutonPresse[3] = true;
}
void IRAM_ATTR isrBouton4() {
  boutonPresse[4] = true;
}

//-----------variables globales------------//
//-----bouton-----//






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

  for (int i = 0; i < 5; i++) {
    pinMode(pinsBoutons[i], INPUT_PULLUP);
  }

  // Attachement des interruptions
  attachInterrupt(digitalPinToInterrupt(pinsBoutons[0]), isrBouton0, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinsBoutons[1]), isrBouton1, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinsBoutons[2]), isrBouton2, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinsBoutons[3]), isrBouton3, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinsBoutons[4]), isrBouton4, FALLING);



  Wire.begin();
  lcd.begin();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Bienvenue");
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) {
    Serial.println("pas de serial");
  }
  Serial.println("Setup");

  strip.begin();             // Initialise la communication avec les LEDs
  strip.show();              // Éteint tout au démarrage
  strip.setBrightness(250);  // Luminosité à environ 20% pour 50 (pour économiser le courant via USB)

  initPiece();

  // Initialiser la mémoire selon les pièces posées

  /* for (uint8_t i = 0; i < 64; i++) {
    if (presence_pion_blanc(i)) memoire_plateau[i] = 1;
    else if (presence_pion_noir(i)) memoire_plateau[i] = 2;
    else memoire_plateau[i] = 0;
  }
*/
  clearEnPassantTarget();
}
int plateauN1[64];
int plateauN2[64];
/*

enum Etape { MENU_PRINCIPAL,
             Calibration,
             TwoPlayer,
             OnePlayer,
             OnLigne,
             Exercice,
             Lancer };
uint8_t menu = 5;
uint8_t Oneplayer = 5;  // difficulté bot, les blanc commence, aide couleur , go, retour
uint8_t Twoplayer = 5;  // difficulté bot, temps, les blanc commence, aide couleur , go, retour
uint8_t Onligne = 1;    // attende  de connexionmettre connecté
uint8_t Exo = 7;        // attende  de connexionmettre connecté
uint8_t Lance = 3, stop,
        Etape etapeActuelle = MENU_PRINCIPAL;*/
bool lancementJEU = false;
bool online = false;
int departOnline;
int arriveOnline;

int x11;
int y11;

int x22;
int y22;
void loop() {

  traiterBoutons();
  if (etapeActuelle == EN_JEU) {  //"1 Joueur", "2 Joueurs", "Calibration", "En Ligne", "Exercice" };

    if (indexMode == 2) {
      // --- MODE CALIBRATION ---//

      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Enlever les");
      lcd.setCursor(5, 1);
      lcd.print("pieces");
      delay(1000);
      for (uint8_t i = 0; i < 64; i++) {
        offset(i);
      }
      lcd.clear();
      lcd.setCursor(8, 1);
      lcd.print("OK");
      delay(1000);
      etapeActuelle = MENU_PRINCIPAL;
    } else if (indexMode == 3) {
      // --- MODE EN LIGNE ---
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Connexion ");
      lcd.setCursor(5, 1);
      lcd.print("server");
      WiFi.mode(WIFI_STA);

      WiFi.disconnect(true, true);
      delay(300);
      WiFi.setSleep(false);

      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
      }
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Connecte");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP());
      delay(1000);
      bool ok = ws.connect(SERVER_IP, SERVER_PORT, "/");
      if (ok) {
        Serial.println("WebSocket CONNECTE");
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Connecte");
        lcd.setCursor(0, 1);
        lcd.print("WebSocket");
      } else {
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Impossible");
        lcd.setCursor(0, 1);
        lcd.print("WebSocket");
      }

      //Fonction Rx WS : affiche les messages reçus du serveur

      ws.onMessage([](WebsocketsMessage message) {
        // 1. On récupère le texte brut du message
        String jsonString = message.data();
        Serial.print("RX WS Brut : ");
        Serial.println(jsonString);

        // 2. On crée un document JSON pour lire les données
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonString);

        // Sécurité : si le serveur envoie un truc bizarre, on ignore
        if (error) {
          Serial.print("Erreur de lecture JSON : ");
          Serial.println(error.c_str());
          return;
        }

        // 3. On regarde quel type de message on a reçu
        String type = doc["type"];

        // --- CAS 1 : On reçoit un coup à jouer (de Lichess ou de la page Web) ---
        if (type == "move") {
          String from = doc["from"];  // ex: "e2"
          String to = doc["to"];      // ex: "e4"

          // 4. LA CONVERSION INVERSE (Texte -> Coordonnées)
          // from[0] est la lettre ('e'), from[1] est le chiffre ('2')
          x11 = from[0] - 'a';
          y11 = from[1] - '1';

          x22 = to[0] - 'a';
          y22 = to[1] - '1';

          Serial.print("♟️ Ordre du serveur reçu ! Déplacer la pièce de (");
          Serial.print(x11);
          Serial.print(",");
          Serial.print(y11);
          Serial.print(") vers (");
          Serial.print(x22);
          Serial.print(",");
          Serial.print(y22);
          Serial.println(")");
        }
      });


      //Ligne de code pour envoyer un message hello au serveur
      String helloMsg = R"({"type":"hello","device":"esp32"})";
      Serial.print("Envoi : ");
      Serial.println(helloMsg);
      ws.send(helloMsg);
      online = true;
      lancementJEU = true;
      etapeActuelle = TIMER;



      //maFonctionEnLigne();
    } else if (indexMode == 4) {
      // --- MODE Exercice ---
      // Exemple : Initialisation assistée
      initExercice(1);
      /*for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
          if (plateau[x][y].estActive()) {
            validerPlacementPiece(x, y);
          }
        }
      }*/
      lancementJEU = true;
      etapeActuelle = TIMER;

    } else if (indexMode == 0) {
      // --- MODE 1 joueur ---
      reinitialiserPartie();
      lancementJEU = true;
      etapeActuelle = TIMER;
    } else if (indexMode == 1) {
      // --- MODE 2 joueur ---
      reinitialiserPartie();
      lancementJEU = true;
      etapeActuelle = TIMER;
    }

    else {
      // --- MODES DE JEU (1J, 2J, Exo) ---
      // Ici ton code habituel de lecture des 64 capteurs
      //lecture64Capteurs();
    }

  } else {
    // On est dans les menus, on peut ajouter un petit delay
    // pour ne pas faire chauffer l'ESP32 inutilement
    delay(10);
  }
  if (etapeActuelle == TIMER) {
    if (lancementJEU) {
      UpdateLED();
      UpdateCapteur();
    }
  }
}


void UpdateCapteur() {
  for (uint8_t i = 0; i < 64; i++) {
    traiterBoutons();
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
          if (caseSoulevee != i) {
            int x = caseSoulevee % 8;  // Le reste donne la colonne (X)
            int y = caseSoulevee / 8;  // Le quotient donne la ligne (Y)

            int x2 = i % 8;  // Le reste donne la colonne (X)
            int y2 = i / 8;  // Le quotient donne la ligne (Y)
            envoi_coups_server(7 - x, y, 7 - x2, y2);
          }
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
          if (caseSoulevee != i) {
            int x = caseSoulevee % 8;  // Le reste donne la colonne (X)
            int y = caseSoulevee / 8;  // Le quotient donne la ligne (Y)

            int x2 = i % 8;  // Le reste donne la colonne (X)
            int y2 = i / 8;  // Le quotient donne la ligne (Y)
            envoi_coups_server(7 - x, y, 7 - x2, y2);
          }
          plateauN2[i] = plateauN1[i];  // La mémoire enregistre la nouvelle couleur
          caseSoulevee = -1;
          coupPossible[0] = 0;
          Serial.println("Piece mangee !");
          tourDesBlancs = !tourDesBlancs;
        }
      }
    }

    if (caseSoulevee == -1 && tourDesBlancs == false && indexMode == 0) {
      CoupRobot decision = calculerMeilleurCoup(NOIR);
      if (decision.x1 != -1) {
        setuLED(coordVersIndex(decision.x1, decision.y1), strip.Color(0, 255, 255));
        setuLED(coordVersIndex(decision.x2, decision.y2), strip.Color(0, 255, 255));
        strip.show();
      }
    }
    if (caseSoulevee == -1 && tourDesBlancs == false && indexMode == 3) {
      if (ws.available()) {
        ws.poll();
        setuLED(departOnline, strip.Color(0, 255, 255));
        setuLED(arriveOnline, strip.Color(0, 255, 255));
      }

      setuLED((7-x11) + (y11 * 8), strip.Color(0, 255, 255));
      setuLED((7-x22) + (y22 * 8), strip.Color(0, 255, 255));
      strip.show();
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
      reinitialiserPartie();
    }
    if (cmd == "offset") {
      for (uint8_t i = 0; i < 64; i++) {
        offset(i);
      }
    }
  }
}



void reinitialiserPartie() {
  Serial.println("Reinitialisation de la partie...");

  // 1. Remettre les pièces à leurs positions initiales (logique)
  //GOinitPiece();

  // 2. Réinitialiser les variables de contrôle du jeu
  caseSoulevee = -1;
  tourDesBlancs = true;  // On commence par les blancs
  clearEnPassantTarget();
  // Vider le tableau des coups possibles (si applicable dans votre regle_echec.h)
  // for(int i=0; i<64; i++) coupPossible[i] = 0;

  // 3. Mettre à jour la mémoire du plateau (capteurs)
  for (uint8_t i = 0; i < 64; i++) {
    if (presence_pion_blanc(i)) memoire_plateau[i] = 1;
    else if (presence_pion_noir(i)) memoire_plateau[i] = 2;
    else memoire_plateau[i] = 0;

    // On synchronise aussi plateauN2 pour éviter des mouvements fantômes au départ
    plateauN2[i] = memoire_plateau[i];
  }
  initPiece();

  // 4. Nettoyer les LEDs et l'affichage
  strip.clear();
  strip.show();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nouvelle Partie");
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print("Blancs jouent  ");

  Serial.println("Partie Reinitialisee !");
}

void traiterBoutons() {
  bool g = false, o = false, d = false;
  if (boutonPresse[0]) {
    g = true;
    boutonPresse[0] = false;
  }
  if (boutonPresse[1]) {
    o = true;
    boutonPresse[1] = false;
  }
  if (boutonPresse[2]) {
    d = true;
    boutonPresse[2] = false;
  }
  if (!g && !o && !d) return;
  delay(400);
  switch (etapeActuelle) {
    case MENU_PRINCIPAL:
      if (g) indexMode = (indexMode > 0) ? indexMode - 1 : 4;
      if (d) indexMode = (indexMode < 4) ? indexMode + 1 : 0;
      if (o) {
        // Calibration (2) et En Ligne (3) n'ont pas de sous-menu
        if (indexMode == 2 || indexMode == 3) etapeActuelle = EN_JEU;
        else {
          etapeActuelle = SOUS_MENU;
          indexParam = 0;
        }
      }
      break;

    case SOUS_MENU:
      if (!modeEdition) {
        // Navigation entre les paramètres (Diff, Temps, Aide, GO, Retour)
        int maxParam = (indexMode == 1) ? 3 : 4;  // 2J a un paramètre de moins (pas de difficulté)
        if (g) indexParam = (indexParam > 0) ? indexParam - 1 : maxParam;
        if (d) indexParam = (indexParam < maxParam) ? indexParam + 1 : 0;

        if (o) {
          if (indexParam == maxParam - 1) etapeActuelle = EN_JEU;           // Bouton GO
          else if (indexParam == maxParam) etapeActuelle = MENU_PRINCIPAL;  // Bouton Retour
          else if (indexMode == 4 && indexParam == 3) {                     // Mode Exercice + Bouton GO
            //initExercice(1);                                        // paramDiff contient le numéro de l'exo (1 à 5)
            profondeur = 2;
            etapeActuelle = EN_JEU;
          } else modeEdition = true;
        }
      } else {
        // Modification des valeurs
        modifierValeur(g, d);
        if (o) modeEdition = false;
      }
      break;

    case EN_JEU:
      if (o) etapeActuelle = MENU_PRINCIPAL;
      break;
  }
  rafraichirAffichage();
}

// Fonction utilitaire pour modifier les paramètres
void modifierValeur(bool g, bool d) {
  // On adapte selon le paramètre pointé par indexParam
  if (indexParam == 0 && (indexMode == 0 || indexMode == 4)) {  // Difficulté
    if (g && paramDiff > 1) paramDiff--;
    if (d && paramDiff < 3) paramDiff++;
  } else if (indexParam == 1 || (indexMode == 1 && indexParam == 0)) {  // Temps
    if (g && paramTemps > 1) paramTemps -= 1;
    if (d) paramTemps += 1;
  } else if (indexParam == 2 || (indexMode == 1 && indexParam == 1)) {  // Aide
    if (g || d) paramAide = !paramAide;
  }
}



void rafraichirAffichage() {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (etapeActuelle == MENU_PRINCIPAL) {
    lcd.print("MENU PRINCIPAL");
    lcd.setCursor(0, 1);
    lcd.print("> ");
    lcd.print(modes[indexMode]);
  }

  else if (etapeActuelle == SOUS_MENU) {
    lcd.print(modes[indexMode]);  // Rappel du mode en ligne 1
    lcd.setCursor(0, 1);

    // Logique d'affichage dynamique des lignes du sous-menu
    if (indexMode == 0 || indexMode == 4) {  // 1J ou Exo
      const char* pNames[] = { "Diff", "Temps", "Aide", "LANCER GO", "RETOUR" };
      afficherLigneParam(pNames[indexParam]);
    } else if (indexMode == 1) {  // 2J
      const char* pNames[] = { "Temps", "Aide", "LANCER GO", "RETOUR" };
      afficherLigneParam(pNames[indexParam]);
    }
  }

  else if (etapeActuelle == EN_JEU) {
    lcd.print("PARTIE LANCER");
    lcd.setCursor(0, 1);
    lcd.print("OK pour quitter");
  }
}

void afficherLigneParam(const char* nom) {
  lcd.print(nom);
  if (modeEdition) lcd.print(": < >");
  else lcd.print(": ");

  // Affichage des valeurs selon le nom
  if (strcmp(nom, "Diff") == 0) lcd.print(paramDiff);
  if (strcmp(nom, "Temps") == 0) {
    lcd.print(paramTemps);
    lcd.print("m");
  }
  if (strcmp(nom, "Aide") == 0) lcd.print(paramAide ? "OUI" : "NON");
}


/*void afficherMenu() {
  // Exemple avec LCD ou Serial
  Serial.println("-----------------------");
  Serial.println("   JOUER AUX ECHECS    ");  // Ligne 1 fixe
  Serial.print("> ");
  Serial.println(modes[modeActuel]);  // Ligne 2 défilante
  Serial.println("-----------------------");
}*/
void envoi_coups_server(int x1, int y1, int x2, int y2) {

  // --- 1. CONVERSION DES COORDONNÉES EN TEXTE (ex: 4,1 -> "e2") ---
  // On additionne notre coordonnée au caractère de base ('a' ou '1')
  char lettre_depart = 'a' + x1;
  char chiffre_depart = '1' + y1;
  String from = String(lettre_depart) + String(chiffre_depart);

  char lettre_arrivee = 'a' + x2;
  char chiffre_arrivee = '1' + y2;
  String to = String(lettre_arrivee) + String(chiffre_arrivee);
  // ---------------------------------------------------------------

  // 2. On crée le document JSON
  JsonDocument doc;
  doc["source"] = "ESP32";
  doc["type"] = "move";
  doc["from"] = from;
  doc["to"] = to;

  // 3. On génère le texte JSON
  String jsonPayload;
  serializeJson(doc, jsonPayload);

  // 4. Envoi au serveur Python via WebSocket
  ws.send(jsonPayload);

  // Trace de débogage pour vérifier sur le Moniteur Série
  Serial.print("♟️ Mouvement detecte (");
  Serial.print(x1);
  Serial.print(",");
  Serial.print(y1);
  Serial.print(") -> (");
  Serial.print(x2);
  Serial.print(",");
  Serial.print(y2);
  Serial.print(") ===> JSON envoye : ");
  Serial.println(jsonPayload);
}