/* ============================================================
 * @file    logement.h
 * @brief   Declarations du module Logement de HabitatCam.
 * @version 1.0
 * @author  FOMEKONG (corrige par SOUOPGUI)
 * ============================================================ */

#ifndef LOGEMENT_H_INCLUDED
#define LOGEMENT_H_INCLUDED

#include "structures.h"

/* Variables globales partagees avec les autres modules */
extern Logement listeLogements[MAX_LOGEMENTS];
extern int      nbLogements;

/* Persistance */
void chargerLogements();
void sauvegarderLogements();

/* Fonctions principales */
void ajouterLogement();
void afficherLogements();
void rechercherLogement();
void supprimerLogement();

/* Utilitaire */
int genererIdLogement();

#endif /* LOGEMENT_H_INCLUDED */
