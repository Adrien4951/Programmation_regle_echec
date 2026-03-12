#include "RobotEchec.h"

// Tables de position pour favoriser le centre
const int p_pion[8][8] = {
    {0,  0,  0,  0,  0,  0,  0,  0},
    {5, 10, 10,-20,-20, 10, 10,  5},
    {5, -5,-10,  0,  0,-10, -5,  5},
    {0,  0,  0, 20, 20,  0,  0,  0},
    {5,  5, 10, 25, 25, 10,  5,  5},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {0,  0,  0,  0,  0,  0,  0,  0}
};

// Variable globale de profondeur définie dans le .ino
extern int profondeur; 

int evaluerPosition() {
    int score = 0;
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            Piece &p = plateau[x][y];
            if (p.getType() == AUCUN) continue;

            int valeur = 0;
            switch (p.getType()) {
                case PION:     valeur = 100 + p_pion[x][y]; break;
                case CAVALIER: valeur = 320; break;
                case FOU:      valeur = 330; break;
                case TOUR:     valeur = 500; break;
                case DAME:     valeur = 900; break;
                case ROI:      valeur = 20000; break;
                default: break;
            }

            if (p.getCouleur() == NOIR) score += valeur;
            else score -= valeur;
        }
    }
    return score;
}

// Fonction récursive Minimax avec élagage Alpha-Bêta
int minimax(int prof, bool estMax, int alpha, int beta) {
    if (prof == 0) return evaluerPosition();

    int tx[40], ty[40];

    if (estMax) {
        int meilleurScore = -30000;
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                if (plateau[x][y].getCouleur() == NOIR) {
                    int nb = genererCoupsPossibles(plateau[x][y], tx, ty);
                    for (int i = 0; i < nb; i++) {
                        Piece sauveDest = plateau[tx[i]][ty[i]];
                        Piece sauveOrig = plateau[x][y];

                        plateau[tx[i]][ty[i]] = plateau[x][y];
                        plateau[tx[i]][ty[i]].setPosition(tx[i], ty[i]);
                        plateau[x][y].vider();

                        int score = minimax(prof - 1, false, alpha, beta);

                        plateau[x][y] = sauveOrig;
                        plateau[tx[i]][ty[i]] = sauveDest;

                        if (score > meilleurScore) meilleurScore = score;
                        if (meilleurScore > alpha) alpha = meilleurScore;
                        if (beta <= alpha) return meilleurScore; 
                    }
                }
            }
        }
        return meilleurScore;
    } else {
        int pireScore = 30000;
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                if (plateau[x][y].getCouleur() == BLANC) {
                    int nb = genererCoupsPossibles(plateau[x][y], tx, ty);
                    for (int i = 0; i < nb; i++) {
                        Piece sauveDest = plateau[tx[i]][ty[i]];
                        Piece sauveOrig = plateau[x][y];

                        plateau[tx[i]][ty[i]] = plateau[x][y];
                        plateau[tx[i]][ty[i]].setPosition(tx[i], ty[i]);
                        plateau[x][y].vider();

                        int score = minimax(prof - 1, true, alpha, beta);

                        plateau[x][y] = sauveOrig;
                        plateau[tx[i]][ty[i]] = sauveDest;

                        if (score < pireScore) pireScore = score;
                        if (pireScore < beta) beta = pireScore;
                        if (beta <= alpha) return pireScore;
                    }
                }
            }
        }
        return pireScore;
    }
}

CoupRobot calculerMeilleurCoup(Couleur camp) {
    CoupRobot meilleur;
    meilleur.x1 = -1; meilleur.y1 = -1;
    meilleur.x2 = -1; meilleur.y2 = -1;
    meilleur.score = -30000;
    
    int alpha = -30000;
    int beta = 30000;
    int tx[40], ty[40];

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (plateau[x][y].getCouleur() == camp) {
                int nb = genererCoupsPossibles(plateau[x][y], tx, ty); 
                
                for (int i = 0; i < nb; i++) {
                    Piece sauveDest = plateau[tx[i]][ty[i]];
                    Piece sauveOrig = plateau[x][y];
                    
                    plateau[tx[i]][ty[i]] = plateau[x][y];
                    plateau[tx[i]][ty[i]].setPosition(tx[i], ty[i]);
                    plateau[x][y].vider();
                    
                    int scoreActuel = minimax(profondeur - 1, false, alpha, beta);
                    
                    plateau[x][y] = sauveOrig;
                    plateau[tx[i]][ty[i]] = sauveDest;
                    
                    if (scoreActuel > meilleur.score) {
                        meilleur.x1 = x;
                        meilleur.y1 = y;
                        meilleur.x2 = tx[i];
                        meilleur.y2 = ty[i];
                        meilleur.score = scoreActuel;
                        alpha = scoreActuel; 
                    }
                }
            }
        }
    }
    return meilleur;
}