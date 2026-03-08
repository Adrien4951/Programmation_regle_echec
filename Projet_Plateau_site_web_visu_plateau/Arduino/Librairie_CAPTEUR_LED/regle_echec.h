#ifndef REGLES_ECHECS_H
#define REGLES_ECHECS_H

#include <Arduino.h>
#include "config.h" // Assure-toi que Piece, TypePiece et Couleur y sont définis



//-----------class-------------------//
enum CoupSPE { Rien,
               PetitRoque,
               GrandRoque,
               EnPassant,
               Promotion };


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

// --- Variables globales partagées avec le .ino ---
// Le mot-clé extern permet d'utiliser les variables créées dans le fichier principal
extern Piece plateau[8][8];
extern CoupSPE SPE;
extern uint8_t coordSPE[4];
extern bool tourDesBlancs;
extern int enPassantCol;
extern int enPassantRow;
extern uint8_t coupPossible[28];

// --- Fonctions de la librairie ---
int genererCoupsPossibles(Piece &p, int X[], int Y[]);
void calculerDeplacements(Piece &p); 

// Générateurs internes
int genererCoupsPion(Piece &p, int X[], int Y[]);
int genererCoupsFou(Piece &p, int X[], int Y[]);
int genererCoupsTour(Piece &p, int X[], int Y[]);
int genererCoupsDame(Piece &p, int X[], int Y[]);
int genererCoupsCavalier(Piece &p, int X[], int Y[]);
int genererCoupsRoi(Piece &p, int X[], int Y[]);

// Helpers
int ajouterCoupsDirections(Piece &p, const int dir[][2], int nbDir, int X[], int Y[]);
bool estCaseAttaquee(int cx, int cy, Couleur parQui);
void filtrerCoupsEnEchec(Piece &p, int X[], int Y[], int &nb);
void setEnPassantTarget(int col, int row);
void clearEnPassantTarget();


#endif