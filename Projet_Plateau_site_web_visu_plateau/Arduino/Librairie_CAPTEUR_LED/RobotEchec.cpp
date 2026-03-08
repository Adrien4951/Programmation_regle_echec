#include "RobotEchec.h"

// on associe un nombre de point sur chaque case l'objectif est de forcé le robot a aller sur le millieux
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

CoupRobot calculerMeilleurCoup(Couleur camp) {
    CoupRobot meilleur;
    meilleur.x1 = -1;
    meilleur.y1 = -1;
    meilleur.x2 = -1;
    meilleur.y2 = -1;
    meilleur.score = -30000;
    
    int tx[40], ty[40];

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (plateau[x][y].getCouleur() == camp) {
                int nb = genererCoupsPossibles(plateau[x][y], tx, ty); 
                
                for (int i = 0; i < nb; i++) {
                    // Simulation
                    Piece destinationSauve = plateau[tx[i]][ty[i]];
                    Piece origineSauve = plateau[x][y];
                    
                    plateau[tx[i]][ty[i]] = plateau[x][y];
                    plateau[tx[i]][ty[i]].setPosition(tx[i], ty[i]);
                    plateau[x][y].vider();
                    
                    int scoreActuel = evaluerPosition();
                    
                    // Annulation
                    plateau[x][y] = origineSauve;
                    plateau[tx[i]][ty[i]] = destinationSauve;
                    
                    if (scoreActuel > meilleur.score) {
                        meilleur.x1 = x;
                        meilleur.y1 = y;
                        meilleur.x2 = tx[i];
                        meilleur.y2 = ty[i];
                        meilleur.score = scoreActuel;
                    }
                }
            }
        }
    }
    return meilleur;
}