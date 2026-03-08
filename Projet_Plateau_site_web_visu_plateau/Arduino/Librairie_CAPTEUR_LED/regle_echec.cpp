#include "regle_echec.h"

//------------------État global du plateau------------------

#define MAX_COUPS 40

// Prise en passant : case cible (où le pion atterrit en prenant en passant). -1 = aucune.
int enPassantCol = -1; // colonne de la case cible
int enPassantRow = -1; // ligne de la case cible

void calculerDeplacements(Piece &p);
int genererCoupsPossibles(Piece &p, int X[], int Y[]);
void afficherCoupsPossibles(int X[], int Y[], int nbCoups);
void afficherPlateauSerial();
void afficherPlateauLED();       // Met à jour les LEDs selon l'état du plateau
void syncPlateauDepuisCapteurs(); // Lit les 64 capteurs et met à jour plateau (PION blanc/noir)
void initPositionEnPassant();    // Réinit tour + en passant (plateau = capteurs)
void appliquerCoup(int x1, int y1, int x2, int y2);  // Met à jour en passant + tour (plateau = capteurs)
uint8_t coordVersIndex(int x, int y);

// À appeler par le binôme quand il applique un coup :
// - Après un double pas du pion : setEnPassantTarget(col, row) avec la case où un pion adverse peut atterrir en prenant en passant.
// - Sinon (coup normal ou tour adverse) : clearEnPassantTarget().
// - Prise en passant : quand le coup joué est une prise en passant, en plus du déplacement, retirer le pion capturé (à (colDest, rowDest - dirY) pour le camp qui vient de jouer).
// - Roque : si le roi va en (6, ligneRoi) ou (2, ligneRoi), déplacer aussi la tour (7,ligneRoi)->(5,ligneRoi) ou (0,ligneRoi)->(3,ligneRoi).
void setEnPassantTarget(int col, int row);
void clearEnPassantTarget();

//extern Piece plateau[8][8];  // plateau[x][y], x = colonne, y = ligne

//bool tourBlanc = true;
int fromX = -1, fromY = -1;  // Pièce en main : case d'origine (-1 = aucune)
int possibleMoveX[MAX_COUPS], possibleMoveY[MAX_COUPS], nbPossibleMoves = 0;
int captureRedCol = -1, captureRedRow = -1;  // Case à afficher en rouge (pièce mangée)
unsigned long captureRedUntil = 0;           // Jusqu'à quel moment (millis)
uint8_t prevType[64], prevColor[64];         // État précédent pour détecter soulèvement/pose
uint8_t prevNb[64];







// Implémentation de ta fonction de dispatch
void calculerDeplacements(Piece &p) {
  Couleur maCouleur = p.getCouleur();
    // 1. Vérifier si c'est le bon tour
  if ((tourDesBlancs && maCouleur == NOIR) || (!tourDesBlancs && maCouleur == BLANC)) {
      Serial.println("Coup INTERDIT : Ce n'est pas votre tour.");
      coupPossible[0] = 100;  //interdit
      return; 
  }
  int X[MAX_COUPS];
  int Y[MAX_COUPS];
  SPE = Rien; 
  // 2. Utiliser la logique propre du fichier de ton amie
  int nbCoups = genererCoupsPossibles(p, X, Y);
  coupPossible[0] = nbCoups;  //interdit
  afficherCoupsPossibles(X, Y, nbCoups);
}


// --- Logique de ton amie (extraite du fichier fourni) ---

int genererCoupsPossibles(Piece &p, int X[], int Y[]) {
    TypePiece t = p.getType(); 
    int nb = 0;
    switch (t) {
        case PION:     nb = genererCoupsPion(p, X, Y); break; 
        case FOU:      nb = genererCoupsFou(p, X, Y); break; 
        case TOUR:     nb = genererCoupsTour(p, X, Y); break; 
        case DAME:     nb = genererCoupsDame(p, X, Y); break; 
        case CAVALIER: nb = genererCoupsCavalier(p, X, Y); break; 
        case ROI:      nb = genererCoupsRoi(p, X, Y); break; 
        default: break;
    }
    filtrerCoupsEnEchec(p, X, Y, nb); 
    return nb;
}

// Pion : avance 1 ou 2, captures diagonales
int genererCoupsPion(Piece &p, int X[], int Y[]) {
  int x = p.getX(), y = p.getY(), n = p.getNbDeplacements();
  Couleur c = p.getCouleur();
  int dirY = (c == BLANC) ? 1 : -1;
  int nb = 0;

  if (y + dirY < 0 || y + dirY > 7) return 0;

  if (plateau[x][y + dirY].getType() == AUCUN) {
    X[nb] = x; Y[nb] = y + dirY; nb++;
    if (n == 0 && plateau[x][y + 2 * dirY].getType() == AUCUN) {
      X[nb] = x; Y[nb] = y + 2 * dirY; nb++;
    }
  }
  for (int d = 0; d < 2; d++) {
    int dx = (d == 0) ? 1 : -1;
    int nx = x + dx;
    if (nx >= 0 && nx <= 7 && plateau[nx][y + dirY].getType() != AUCUN && plateau[nx][y + dirY].getCouleur() != c) {
      X[nb] = nx; Y[nb] = y + dirY; nb++;
    }
  }
  // Prise en passant : possible si une cible est définie sur la rangée devant nous (case vide)
  // --- DEBUG PRÉCIS POUR LA PRISE EN PASSANT ---
Serial.println(F("--- Check En Passant ---"));
Serial.print(F("Cible (Col, Row): ")); Serial.print(enPassantCol); Serial.print(F(", ")); Serial.println(enPassantRow);
Serial.print(F("Pion actuel (x, y): ")); Serial.print(x); Serial.print(F(", ")); Serial.println(y);
Serial.print(F("Cible attendue (y + dirY): ")); Serial.println(y + dirY);

// Affichage des résultats des tests logiques
Serial.print(F("1. enPassantCol >= 0 : ")); Serial.println(enPassantCol >= 0 ? "OUI" : "NON");
Serial.print(F("2. enPassantRow == y + dirY : ")); Serial.println(enPassantRow == (y + dirY) ? "OUI" : "NON");
Serial.print(F("3. Voisinage (x+1 ou x-1) : ")); Serial.println((enPassantCol == x + 1 || enPassantCol == x - 1) ? "OUI" : "NON");

if (enPassantCol >= 0 && enPassantCol <= 7 && enPassantRow >= 0 && enPassantRow <= 7) {
    Serial.print(F("4. Case d'arrivée AUCUN : ")); 
    Serial.println(plateau[enPassantCol][enPassantRow].getType() == AUCUN ? "OUI" : "NON");
}
Serial.println(F("------------------------"));

// Votre condition d'origine
if (enPassantCol >= 0 && enPassantRow == y + dirY && (enPassantCol == x + 1 || enPassantCol == x - 1) && plateau[enPassantCol][enPassantRow].getType() == AUCUN) {
    X[nb] = enPassantCol; Y[nb] = enPassantRow; nb++;
}

//------------fin-----------------//
  return nb;
}

// Fou : déplacements en diagonale (réutilise ajouterCoupsDirection)
int genererCoupsFou(Piece &p, int X[], int Y[]) {
  const int dir[4][2] = { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };
  return ajouterCoupsDirections(p, dir, 4, X, Y);
}

// Tour : déplacements en ligne (réutilise ajouterCoupsDirection)
int genererCoupsTour(Piece &p, int X[], int Y[]) {
  const int dir[4][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } };
  return ajouterCoupsDirections(p, dir, 4, X, Y);
}

// Dame : Fou + Tour
int genererCoupsDame(Piece &p, int X[], int Y[]) {
  const int dir[8][2] = { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 }, { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } };
  return ajouterCoupsDirections(p, dir, 8, X, Y);
}

// Cavalier : 8 cases en L
int genererCoupsCavalier(Piece &p, int X[], int Y[]) {
  const int dir[8][2] = { { -2, 1 }, { -1, 2 }, { 1, 2 }, { 2, 1 }, { 2, -1 }, { 1, -2 }, { -1, -2 }, { -2, -1 } };
  int x = p.getX(), y = p.getY();
  Couleur c = p.getCouleur();
  int nb = 0;
  for (int d = 0; d < 8; d++) {
    int nx = x + dir[d][0], ny = y + dir[d][1];
    if (nx >= 0 && nx <= 7 && ny >= 0 && ny <= 7) {
      if (plateau[nx][ny].getType() == AUCUN || plateau[nx][ny].getCouleur() != c) {
        X[nb] = nx; Y[nb] = ny; nb++;
      }
    }
  }
  return nb;
}

// Roi : 8 cases autour + roque si possible
int genererCoupsRoi(Piece &p, int X[], int Y[]) {
  const int dir[8][2] = { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } };
  int x = p.getX(), y = p.getY();
  Couleur c = p.getCouleur();
  int nb = 0;
  for (int d = 0; d < 8; d++) {
    int nx = x + dir[d][0], ny = y + dir[d][1];
    if (nx >= 0 && nx <= 7 && ny >= 0 && ny <= 7) {
      if (plateau[nx][ny].getType() == AUCUN || plateau[nx][ny].getCouleur() != c) {
        X[nb] = nx; Y[nb] = ny; nb++;
      }
    }
  }
    // Roque : roi et tour jamais bougé, pas de pièce entre, roi et cases traversées non attaquées
  if (p.getNbDeplacements() == 0 && !estCaseAttaquee(x, y, (c == BLANC) ? NOIR : BLANC)) {
    int ligneRoi = (c == BLANC) ? 0 : 7;
    if (y != ligneRoi) { return nb; }
    // Petit roque (roi vers colonne 6, tour 7 -> 5)
    if (plateau[7][ligneRoi].getType() == TOUR && plateau[7][ligneRoi].getCouleur() == c && plateau[7][ligneRoi].getNbDeplacements() == 0) {
      bool voieLibre = true;
      for (int col = 5; col <= 6; col++) { if (plateau[col][ligneRoi].getType() != AUCUN) voieLibre = false; }
      if (voieLibre && !estCaseAttaquee(5, ligneRoi, (c == BLANC) ? NOIR : BLANC) && !estCaseAttaquee(6, ligneRoi, (c == BLANC) ? NOIR : BLANC)) {
        X[nb] = 6; Y[nb] = ligneRoi; nb++;
        SPE = PetitRoque;
      }
    }
    // Grand roque (roi vers colonne 2, tour 0 -> 3)
    if (plateau[0][ligneRoi].getType() == TOUR && plateau[0][ligneRoi].getCouleur() == c && plateau[0][ligneRoi].getNbDeplacements() == 0) {
      bool voieLibre = true;
      for (int col = 1; col <= 3; col++) { if (plateau[col][ligneRoi].getType() != AUCUN) voieLibre = false; }
      if (voieLibre && !estCaseAttaquee(2, ligneRoi, (c == BLANC) ? NOIR : BLANC) && !estCaseAttaquee(3, ligneRoi, (c == BLANC) ? NOIR : BLANC)) {
        X[nb] = 2; Y[nb] = ligneRoi; nb++;
        SPE = GrandRoque;
      }
    }
  }
  return nb;
}





int ajouterCoupsDirections(Piece &p, const int dir[][2], int nbDir, int X[], int Y[]) {
    int x = p.getX(), y = p.getY(); 
    Couleur c = p.getCouleur();
    int nb = 0;
    for (int d = 0; d < nbDir; d++) {
        for (int i = 1; i < 8; i++) {
            int nx = x + i * dir[d][0], ny = y + i * dir[d][1]; 
            if (nx < 0 || nx > 7 || ny < 0 || ny > 7) break; 
            if (plateau[nx][ny].getType() == AUCUN) { 
                X[nb] = nx; Y[nb] = ny; nb++;
            } else {
                if (plateau[nx][ny].getCouleur() != c) { 
                    X[nb] = nx; Y[nb] = ny; nb++;
                }
                break; 
            }
        }
    }
    return nb; 
}

//------------------Prise en passant : API pour le binôme------------------

void setEnPassantTarget(int col, int row) {
  enPassantCol = col;
  enPassantRow = row;
}

void clearEnPassantTarget() {
  enPassantCol = -1;
  enPassantRow = -1;
}

//------------------Vérification échec / case attaquée------------------

// Retourne true si la case (cx, cy) est attaquée par au moins une pièce de la couleur parQui
bool estCaseAttaquee(int cx, int cy, Couleur parQui) {
  const int dirCavalier[8][2] = { { -2, 1 }, { -1, 2 }, { 1, 2 }, { 2, 1 }, { 2, -1 }, { 1, -2 }, { -1, -2 }, { -2, -1 } };
  const int dirRoi[8][2] = { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } };
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      Piece &q = plateau[i][j];
      if (q.getType() == AUCUN || q.getCouleur() != parQui) continue;
      int px = q.getX(), py = q.getY();
      TypePiece tp = q.getType();
      if (tp == PION) {
        int dy = (parQui == BLANC) ? 1 : -1;
        if (cy == py + dy && (cx == px + 1 || cx == px - 1)) return true;
      } else if (tp == CAVALIER) {
        for (int d = 0; d < 8; d++) {
          if (px + dirCavalier[d][0] == cx && py + dirCavalier[d][1] == cy) return true;
        }
      } else if (tp == ROI) {
        for (int d = 0; d < 8; d++) {
          if (px + dirRoi[d][0] == cx && py + dirRoi[d][1] == cy) return true;
        }
      } else if (tp == FOU || tp == TOUR || tp == DAME) {
        int dx = (cx > px) ? 1 : (cx < px) ? -1 : 0;
        int dy = (cy > py) ? 1 : (cy < py) ? -1 : 0;
        if (dx == 0 && dy == 0) continue;
        if (tp == FOU && (dx == 0 || dy == 0)) continue;
        if (tp == TOUR && dx != 0 && dy != 0) continue;
        int steps = (abs(cx - px) > abs(cy - py)) ? abs(cx - px) : abs(cy - py);
        if (px + steps * dx != cx || py + steps * dy != cy) continue;  // (cx,cy) pas sur la ligne
        bool bloque = false;
        for (int s = 1; s < steps; s++) {
          int nx = px + s * dx, ny = py + s * dy;
          if (plateau[nx][ny].getType() != AUCUN) { bloque = true; break; }
        }
        if (!bloque) return true;
      }
    }
  }
  return false;
}

// Enlève de X[], Y[] les coups qui laisseraient notre roi en échec (modifie nb)
// Simulation sans appeler setPosition pour ne pas modifier nbDeplacements.
void filtrerCoupsEnEchec(Piece &p, int X[], int Y[], int &nb) {
  int fromX = p.getX(), fromY = p.getY();
  Couleur nous = p.getCouleur();
  Couleur adversaire = (nous == BLANC) ? NOIR : BLANC;
  for (int i = 0; i < nb; i++) {
    int toX = X[i], toY = Y[i];
    Piece sauveDest = plateau[toX][toY];
    plateau[toX][toY] = plateau[fromX][fromY];
    plateau[fromX][fromY].vider();
    int roiX, roiY;
    if (plateau[toX][toY].getType() == ROI) { roiX = toX; roiY = toY; }
    else {
      roiX = -1; roiY = -1;
      for (int a = 0; a < 8 && roiX < 0; a++)
        for (int b = 0; b < 8; b++)
          if (plateau[a][b].getType() == ROI && plateau[a][b].getCouleur() == nous) { roiX = a; roiY = b; break; }
    }
    bool enEchec = estCaseAttaquee(roiX, roiY, adversaire);
    plateau[fromX][fromY] = plateau[toX][toY];
    plateau[toX][toY] = sauveDest;
    if (enEchec) {
      X[i] = X[nb - 1]; Y[i] = Y[nb - 1];
      nb--;
      i--;
    }
  }
}

//------------------Affichage des coups (à personnaliser)------------------

// Affiche les coups possibles : Serial + LEDs. Modifier ici pour changer l'affichage.
void afficherCoupsPossibles(int X[], int Y[], int nbCoups) {
  Serial.println("Coup possible:");
  for (int i = 0; i < nbCoups; i++) {
    Serial.print("X: ");
    Serial.print(X[i]);
    Serial.print(" | Y: ");
    Serial.println(Y[i]);
    coupPossible[i + 1] = coordVersIndex(X[i], Y[i]);
    //setuLED(X[i] + Y[i] * 8, strip.Color(0, 0, 255));  // LED bleue = case atteignable
  }
}

uint8_t coordVersIndex(int x, int y) {
  // Formule : Index = x + (y * 8)
  // Exemple pour [3][1] : 3 + (1 * 8) = 11
  return x + (y * 8);
}
