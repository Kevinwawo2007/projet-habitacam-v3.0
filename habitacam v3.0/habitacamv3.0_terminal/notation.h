/* ============================================================
 * @file    notation.h
 * @brief   Module de notation des logements V2.0.
 *
 * Permet aux locataires de noter les logements apres
 * reservation et d'afficher les avis sur les annonces.
 *
 * Fichier de donnees :
 *   data/notations.txt
 *   Format : idNotation|idLocataire|idLogement|note|commentaire
 *
 * @version 2.0
 * @author  GUIASSU
 * ============================================================ */

#ifndef NOTATION_H_INCLUDED
#define NOTATION_H_INCLUDED

#include "structures.h"

#define FICHIER_NOTATIONS "data/notations.txt"
#define TAILLE_COMMENTAIRE 200

/**
 * @brief Represente une notation d'un logement par un locataire.
 */
typedef struct {
    int  idNotation;                     /**< ID unique.             */
    int  idLocataire;                    /**< ID du locataire.       */
    int  idLogement;                     /**< ID du logement note.   */
    int  note;                           /**< Note de 1 a 5.         */
    char commentaire[TAILLE_COMMENTAIRE];/**< Commentaire libre.     */
} Notation;

/**
 * @brief Permet au locataire de noter un logement.
 * Verifie qu'il n'a pas deja note ce logement.
 */
void noterLogement();

/**
 * @brief Affiche les avis d'un logement avec sa note moyenne.
 * @param idLogement ID du logement dont on veut les avis.
 */
void afficherAvis(int idLogement);

/**
 * @brief Calcule la note moyenne d'un logement.
 * @param idLogement ID du logement.
 * @return Note moyenne (0.0 si aucun avis).
 */
float calculerMoyenne(int idLogement);

/**
 * @brief Menu de gestion des notations pour le locataire.
 */
void menuNotation();

#endif /* NOTATION_H_INCLUDED */
