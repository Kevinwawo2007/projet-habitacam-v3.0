/* ============================================================
 * @file    matching.h
 * @brief   Module de recommandations intelligentes V2.0.
 *
 * Ce module analyse les preferences de recherche du locataire
 * et lui propose automatiquement les logements les plus
 * compatibles avec un score en pourcentage.
 *
 * Fonctionnement :
 *   1. Quand un locataire fait une recherche, ses criteres
 *      (ville, budget, superficie) sont sauvegardes dans
 *      data/profils.txt
 *   2. A chaque connexion, le systeme compare ces criteres
 *      avec tous les logements disponibles
 *   3. Les logements sont tries par score decroissant
 *   4. Les 5 meilleurs sont affiches automatiquement
 *
 * Fichier de donnees :
 *   data/profils.txt — format : idLocataire|ville|budgetMax|surfaceMin
 *
 * @version 2.0
 * @author  SOUOPGUI
 * ============================================================ */

#ifndef MATCHING_H_INCLUDED
#define MATCHING_H_INCLUDED

#include "structures.h"

/* ============================================================
 * CONSTANTES
 * ============================================================ */

/** @brief Chemin du fichier de profils de recherche. */
#define FICHIER_PROFILS "data/profils.txt"

/** @brief Nombre maximum de recommandations affichees. */
#define MAX_RECOMMANDATIONS 5

/* ============================================================
 * STRUCTURE — Profil de recherche
 * ============================================================ */

/**
 * @brief Profil de recherche d'un locataire.
 *
 * Contient les derniers criteres de recherche utilises.
 * Sauvegarde dans data/profils.txt apres chaque recherche.
 */
typedef struct {
    int   idLocataire;          /**< ID du locataire.          */
    char  ville[TAILLE_VILLE];  /**< Ville recherchee.         */
    float budgetMax;            /**< Budget maximum en FCFA.   */
    float surfaceMin;           /**< Superficie minimum en m2. */
} ProfilRecherche;

/* ============================================================
 * FONCTIONS PRINCIPALES
 * ============================================================ */

/**
 * @brief Sauvegarde le profil de recherche du locataire.
 *
 * Appelee automatiquement apres chaque recherche avancee
 * pour mettre a jour les preferences du locataire.
 *
 * @param profil Le profil a sauvegarder.
 */
void sauvegarderProfil(ProfilRecherche profil);

/**
 * @brief Charge le profil de recherche d'un locataire.
 *
 * @param idLocataire ID du locataire dont on veut le profil.
 * @param profil      Structure ou stocker le profil charge.
 * @return 1 si profil trouve, 0 sinon.
 */
int chargerProfil(int idLocataire, ProfilRecherche *profil);

/**
 * @brief Calcule le score de compatibilite d'un logement.
 *
 * Compare le logement avec le profil du locataire sur
 * 3 criteres : ville, budget, superficie.
 * Chaque critere vaut 33 points. Score total sur 100.
 *
 * @param l      Le logement a evaluer.
 * @param profil Le profil de recherche du locataire.
 * @return Score entre 0 et 100.
 */
int calculerScore(const Logement *l, const ProfilRecherche *profil);

/**
 * @brief Affiche les logements recommandes a la connexion.
 *
 * Charge le profil du locataire, calcule le score de chaque
 * logement disponible, trie par score decroissant et affiche
 * les MAX_RECOMMANDATIONS meilleurs resultats.
 * N'affiche rien si le locataire n'a jamais fait de recherche.
 *
 * @param idLocataire ID du locataire connecte.
 */
void afficherRecommandations(int idLocataire);

/**
 * @brief Recherche avancee avec sauvegarde automatique du profil.
 *
 * Permet au locataire de definir ses criteres (ville, budget,
 * superficie), effectue la recherche et sauvegarde les criteres
 * dans data/profils.txt pour les prochaines recommandations.
 */
void rechercheAvancee(void);

#endif /* MATCHING_H_INCLUDED */
