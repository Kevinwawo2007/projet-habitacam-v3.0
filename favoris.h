/* ============================================================
 * @file    favoris.h
 * @brief   Module de gestion des logements favoris V2.0.
 *
 * Permet a un locataire de sauvegarder des logements qui
 * l'interessent et d'etre alerte quand ils deviennent
 * disponibles.
 *
 * Fichier de donnees :

 *   data/favoris.txt â€” format : idLocataire|idLogement|dateAjout
=======
 *   data/favoris.txt — format : idLocataire|idLogement|dateAjout

 *
 * @version 2.0
 * @author  TOGNANG
 * ============================================================ */

#ifndef FAVORIS_H_INCLUDED
#define FAVORIS_H_INCLUDED

#include "structures.h"

/* ==================================================
 * STRUCTURE â€” Favori
=======
 * STRUCTURE — Favori
 * ============================================================ */

/**
 * @brief Represente un logement mis en favori par un locataire.
 */
typedef struct {
    int  idLocataire;       /**< ID du locataire.          */
    int  idLogement;        /**< ID du logement favori.    */
    char dateAjout[20];     /**< Date d'ajout du favori.   */
} Favori;

/* ============================================================
 * FONCTIONS PRINCIPALES
 * ============================================================ */

/**
 * @brief Ajoute un logement aux favoris du locataire connecte.
 *
 * Verifie que le logement existe et n'est pas deja en favori
 * avant de l'ajouter dans data/favoris.txt.
 */
void ajouterFavori();

/**
 * @brief Affiche tous les favoris du locataire connecte.
 *
 * Affiche le titre, la ville, le prix et le statut actuel
 * de chaque logement favori.
 */
void voirMesFavoris();

/**
 * @brief Supprime un favori de la liste du locataire.
 */
void supprimerFavori();

/**
 * @brief Verifie les alertes favoris a la connexion.
 *
 * Parcourt les favoris du locataire et affiche une alerte
 * pour chaque logement favori qui est maintenant disponible.
 * Appelee automatiquement apres la connexion du locataire.
 *
 * @param idLocataire ID du locataire connecte.
 */
void verifierAlertesFavoris(int idLocataire);

/**
 * @brief Affiche le menu complet des favoris.
 */
void menuFavoris();

#endif /* FAVORIS_H_INCLUDED */
