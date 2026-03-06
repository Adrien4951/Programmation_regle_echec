// Auteur Adrien et léni 
//programme test regle jeu echec 
//config : speed serial 115200 / PIN_LED : a modifer en fonction arduino uno et esp32
enum Couleur { VIDE, BLANC, NOIR };
enum TypePiece {AUCUN, PION, CAVALIER, FOU, TOUR, DAME, ROI };

class Piece {
  private:
    TypePiece type;
    Couleur couleur;
    bool active;
    int x, y;          // Coordonnées (0 à 7)
    int nbDeplacements; // Pour gérer le premier pas du pion et le roque

  public:
    // Constructeur par défaut
    Piece() : type(AUCUN), couleur(VIDE), active(false), x(-1), y(-1), nbDeplacements(0) {}

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
    TypePiece getType() { return type; }
    Couleur getCouleur() { return couleur; }
    bool estActive() { return active; }
    int getX() { return x; }
    int getY() { return y; }
    int getNbDeplacements() { return nbDeplacements; }

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


Piece plateau[8][8];


void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial){
    Serial.println("pas de serial");
  }
  Serial.println("Setup");
  plateau[4][2].reset(CAVALIER, BLANC, 4, 2);
  plateau[2][4].reset(PION, NOIR, 2, 4);
}

void loop() {
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
      int x1 = cmd.substring(0, 1).toInt(); int y1 = cmd.substring(2, 3).toInt();
      int x2 = cmd.substring(4, 5).toInt(); int y2 = cmd.substring(6, 7).toInt();
      
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
  int dirCavalier[8][2] = {{-2,1},{-1,2},{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1}};
  int dirRook[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
  int dirBishop[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};


  int cassePossible = 0;
  int X[40];
  int Y[40];
  // --- PION ---
 // --- PION ---
  if (t == PION) {
    int dirY = (maCouleur == BLANC) ? 1 : -1;
    
    // Avance simple
    if (y + dirY >= 0 && y + dirY <= 7 && plateau[x][y + dirY].getType() == AUCUN) 
    {
        X[cassePossible] = x;
        Y[cassePossible] = y + dirY;
        cassePossible++;

        if(nbrDeplacment == 0 && plateau[x][y + dirY].getType() == AUCUN)
        {
          X[cassePossible] = x;
          Y[cassePossible] = y + 2*dirY;
          cassePossible++;
        }
    }
      if (y + dirY >= 0 && y + dirY <= 7 && x + dirY >= 0 && x + dirY <= 7 && plateau[x + dirY][y + dirY].getType() != AUCUN && plateau[x + dirY][y + dirY].getCouleur() != maCouleur) 
      {
        X[cassePossible] = x + dirY;
        Y[cassePossible] = y + dirY;
        cassePossible++;
      }

      if (y + dirY >= 0 && y + dirY <= 7 && x - dirY >= 0 && x - dirY <= 7 && plateau[x - dirY][y + dirY].getType() != AUCUN && plateau[x - dirY][y + dirY].getCouleur() != maCouleur)
      {
        X[cassePossible] = x - dirY;
        Y[cassePossible] = y + dirY;
        cassePossible++;
      }  
    
  }

  if (t == CAVALIER) {
    for (int i = 1; i<7; i++ )//a modifier si en case 00 nous avons pas 7 valeur
    {
      if(y + i >= 0 && y + i <= 7 && x + i >= 0 && x + i <= 7 && (plateau[x + i][y + i].getType() == AUCUN || (plateau[x + i][y + i].getType() != AUCUN && plateau[x + i][y + i].getCouleur() != maCouleur)))//++
      {
        X[cassePossible] = x + i;
        Y[cassePossible] = y + i;
        cassePossible++;
      }
      else i = 8;
      if(plateau[x + i][y + i].getType() != AUCUN)
      {
        i = 8;
      }
    }
    for (int i = 1; i<7; i++ )//a modifier si en case 00 nous avons pas 7 valeur
    {
      if(y - i >= 0 && y - i <= 7 && x - i >= 0 && x - i <= 7 && (plateau[x - i][y - i].getType() == AUCUN || (plateau[x - i][y - i].getType() != AUCUN && plateau[x - i][y - i].getCouleur() != maCouleur)))//--
      {
        X[cassePossible] = x - i;
        Y[cassePossible] = y - i;
        cassePossible++;
      }
      else i = 8;
      if(plateau[x - i][y - i].getType() != AUCUN)
      {
        i = 8;
      }
    }

    for (int i = 1; i<7; i++ )//a modifier si en case 00 nous avons pas 7 valeur
    {
      if(y - i >= 0 && y - i <= 7 && x + i >= 0 && x + i <= 7 && (plateau[x + i][y - i].getType() == AUCUN || (plateau[x + i][y - i].getType() != AUCUN && plateau[x + i][y - i].getCouleur() != maCouleur)))//+-
      {
        X[cassePossible] = x + i;
        Y[cassePossible] = y - i;
        cassePossible++;
      }
      else i = 8;
      if(plateau[x + i][y - i].getType() != AUCUN)
      {
        i = 8;
      }
    }

    for (int i = 1; i<7; i++ )//a modifier si en case 00 nous avons pas 7 valeur
    {
      if(y + i >= 0 && y + i <= 7 && x - i >= 0 && x - i <= 7 && (plateau[x - i][y + i].getType() == AUCUN || (plateau[x - i][y + i].getType() != AUCUN && plateau[x - i][y + i].getCouleur() != maCouleur)))//-+
      {
        X[cassePossible] = x - i;
        Y[cassePossible] = y + i;
        cassePossible++;
      }
      else i = 8;
      if(plateau[x - i][y + i].getType() != AUCUN)
      {
        i = 8;
      }
    }
  }

  Serial.println("Coup possible:");

  for(int i = 0; i<cassePossible; i++){
    Serial.print("X: ");
    Serial.print(X[i]);
    Serial.print(" | Y: ");
    Serial.println(Y[i]);
  }

 
}




void afficherPlateauSerial() {
  Serial.println("\n    0    1    2    3    4    5    6    7   (X)");
  Serial.println("  +----+----+----+----+----+----+----+----+");
  
  for (int y = 0; y < 8; y++) {
    Serial.print(y); Serial.print(" | "); // Indice de ligne Y
    for (int x = 0; x < 8; x++) {
      Piece &p = plateau[x][y];
      if (p.getType() == AUCUN) {
        Serial.print("  ");
      } else {
        // Initiale de la pièce (P, C, F, T, D, R)
        char c;
        switch(p.getType()) {
          case PION:     c = 'P'; break;
          case CAVALIER: c = 'C'; break;
          case FOU:      c = 'F'; break;
          case TOUR:     c = 'T'; break;
          case DAME:     c = 'D'; break;
          case ROI:      c = 'R'; break;
          default:       c = ' '; break;
        }
        // Minuscule pour Noir, Majuscule pour Blanc
        if (p.getCouleur() == NOIR) c = c + 32; 
        Serial.print(c);
        Serial.print((p.getCouleur() == BLANC) ? "b" : "n"); // b pour blanc, n pour noir
      }
      Serial.print(" | ");
    }
    Serial.println();
    Serial.println("  +----+----+----+----+----+----+----+----+");
  }
}






