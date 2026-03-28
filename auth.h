/* ============================================================
 * @file    auth.h
 * @brief   Module d'authentification de HabitatCam V2.0.
 *
 * Ce fichier declare toutes les fonctions et variables
 * du module d'authentification. Il doit etre inclus dans
 * tout fichier qui utilise les fonctions de connexion,
 * d'inscription ou de gestion des mots de passe.
 *
 * Fonctionnalites V2.0 :
 *   - Retour AuthStatus sur toutes les fonctions principales
 *   - Verrouillage automatique apres 3 echecs de connexion
 *   - Messages d'erreur precis selon le code retour
 *   - Lecture des saisies avec lire_ligne() sans bug Windows
 *   - Toggle visibilite mot de passe (touche V pendant saisie)
 *   - Authentification admin renforcee a 3 niveaux :
 *       1. Code secret lu dans data/admin_config.txt
 *       2. Email du compte administrateur
 *       3. Mot de passe avec toggle visibilite
 *   - Mot de passe oublie : demande envoyee a l'admin
 *   - Changement de mot de passe pour tous les roles
 *   - Compatible Windows (conio.h) et Linux (termios.h)
 *
 * Fichiers de donnees utilises :
 *   - data/utilisateurs.txt    : comptes utilisateurs
 *   - data/admin_config.txt    : code secret administrateur
 *   - data/demandes_reinit.txt : demandes de reinitialisation
 *
 * @version 2.0
 * @author  SOUOPGUI
 * ============================================================ */

#ifndef AUTH_H_INCLUDED
#define AUTH_H_INCLUDED

#include "structures.h"

/* ============================================================
 * VARIABLES GLOBALES PARTAGEES
 *
 * Ces variables sont definies dans auth.c et partagees
 * avec tous les autres modules via le mot-cle extern.
 * ============================================================ */

/** @brief Tableau de tous les utilisateurs charges en memoire. */
extern Utilisateur listeUtilisateurs[MAX_UTILISATEURS];

/** @brief Nombre d'utilisateurs actuellement enregistres. */
extern int nbUtilisateurs;

/** @brief Session de l'utilisateur actuellement connecte. */
extern Session sessionCourante;

/* ============================================================
 * MENU PRINCIPAL D'AUTHENTIFICATION
 * ============================================================ */

/**
 * @brief Affiche le menu de bienvenue et orchestre l'auth.
 *
 * Propose 4 options :
 *   1. Se connecter       (Locataire / Bailleur)
 *   2. Creer un compte    (Locataire / Bailleur)
 *   3. Acces restreint    (Admin — 3 niveaux de verification)
 *   0. Quitter
 *
 * Boucle jusqu'a connexion reussie ou choix de quitter.
 * Gere les saisies invalides sans planter.
 */
void menuAuthentification();

/* ============================================================
 * INSCRIPTION ET CONNEXION
 * ============================================================ */

/**
 * @brief Inscrit un nouveau Locataire ou Bailleur.
 *
 * Saisit nom, prenom, email, telephone, mot de passe
 * avec confirmation et role. Verifie l'unicite de l'email
 * avant d'enregistrer le compte.
 *
 * @return AUTH_OK si inscription reussie,
 *         AUTH_ERR_EXISTS si email deja utilise,
 *         AUTH_ERR_IO si probleme fichier,
 *         AUTH_ERR_INVALID si role invalide.
 */
AuthStatus inscrireUtilisateur();

/**
 * @brief Connecte un Locataire ou Bailleur existant.
 *
 * Demande l'email et le mot de passe. Autorise 3 tentatives.
 * Apres 3 echecs le compte est verrouille (actif = 0) et
 * l'option mot de passe oublie est proposee automatiquement.
 *
 * @return AUTH_OK si connexion reussie,
 *         AUTH_ERR_NOTFOUND si email inconnu,
 *         AUTH_ERR_INVALID si mot de passe incorrect ou inactif.
 */
AuthStatus connecterUtilisateur();

/**
 * @brief Deconnecte l'utilisateur et reinitialise la session.
 *
 * Remet sessionCourante.connecte a 0 et efface les donnees
 * de l'utilisateur en session.
 */
void deconnecterUtilisateur();

/* ============================================================
 * GESTION DES MOTS DE PASSE
 * ============================================================ */

/**
 * @brief Enregistre une demande de reinitialisation de mdp.
 *
 * Appelee automatiquement apres 3 echecs de connexion.
 * Au lieu de laisser l'utilisateur reinitialiser lui-meme
 * son mot de passe (faille de securite), cette fonction :
 *   1. Propose a l'utilisateur d'envoyer une demande
 *   2. Enregistre la demande dans data/demandes_reinit.txt
 *   3. L'admin verra la demande dans son panneau (option 9)
 *   4. L'admin attribue un mot de passe temporaire
 *   5. L'utilisateur se connecte et change son mot de passe
 *      via l'option [Changer mon mot de passe]
 *
 * @note La notification en temps reel sera implementee
 *       dans le module notification.c (V2.0).
 *
 * @param email Email du compte concerne.
 * @return AUTH_OK si demande enregistree,
 *         AUTH_ERR_IO si probleme fichier,
 *         AUTH_ERR_INVALID si l'utilisateur refuse la demande.
 */
AuthStatus motDePasseOublie(const char *email);

/**
 * @brief Permet a l'utilisateur connecte de changer son mdp.
 *
 * Verifie l'ancien mot de passe avant d'autoriser le
 * changement. Met a jour la session courante immediatement.
 * Disponible dans le menu de chaque role :
 *   - Locataire      : option 5
 *   - Bailleur       : option 5
 *   - Administrateur : option 10
 *
 * @return AUTH_OK si changement reussi,
 *         AUTH_ERR_INVALID si ancien mot de passe incorrect,
 *         AUTH_ERR_NOTFOUND si session corrompue.
 */
AuthStatus changerMotDePasse();

/**
 * @brief Met a jour le code secret admin dans admin_config.txt.
 *
 * Verifie l'ancien code avant d'autoriser le changement.
 * Permet a l'admin de changer son code secret sans modifier
 * le code source ni recompiler l'application.
 * Accessible depuis le panneau admin (option 8).
 *
 * @return AUTH_OK si mise a jour reussie,
 *         AUTH_ERR_INVALID si ancien code incorrect,
 *         AUTH_ERR_IO si probleme d'ecriture fichier.
 */
AuthStatus modifierCodeSecret();

/* ============================================================
 * FONCTIONS UTILITAIRES
 * ============================================================ */

/**
 * @brief Verifie si un email est deja utilise par un compte.
 *
 * Parcourt listeUtilisateurs[] et compare chaque email.
 *
 * @param email Adresse email a verifier.
 * @return 1 si l'email existe deja, 0 sinon.
 */
int emailExiste(const char *email);

/**
 * @brief Recherche un utilisateur par son email uniquement.
 *
 * Contrairement a la V1.0 qui verificait email ET mot de
 * passe en meme temps, cette fonction ne cherche que par
 * email — ce qui permet des messages d'erreur plus precis
 * (email inconnu VS mauvais mot de passe).
 *
 * @param email Adresse email a rechercher.
 * @return Index dans listeUtilisateurs si trouve, -1 sinon.
 */
int rechercherUtilisateurParEmail(const char *email);

/**
 * @brief Genere un ID unique pour un nouvel utilisateur.
 *
 * Parcourt listeUtilisateurs[] pour trouver le plus grand
 * ID existant et retourne ce maximum incremente de 1.
 *
 * @return Nouvel ID unique (entier positif >= 1).
 */
int genererIdUtilisateur();

/**
 * @brief Affiche un message d'erreur precis selon AuthStatus.
 *
 * Traduit le code de retour en message lisible par l'utilisateur.
 * Exemple : AUTH_ERR_NOTFOUND -> "Aucun compte avec cet email."
 *
 * @param st Le code AuthStatus a interpreter et afficher.
 */
void afficherErreurAuth(AuthStatus st);

/* ============================================================
 * PERSISTANCE
 * ============================================================ */

/**
 * @brief Charge les utilisateurs depuis le fichier texte.
 *
 * Lit FICHIER_UTILISATEURS et remplit listeUtilisateurs[].
 * Si le fichier est absent (premier lancement), cree
 * automatiquement un compte administrateur par defaut :
 *   Email        : admin@habitatcam.cm
 *   Mot de passe : admin123
 *
 * @note Format : id|nom|prenom|email|tel|mdp|role|nbEchecs|actif
 * @warning Appeler cette fonction au demarrage dans main.c
 *          avant tout autre appel aux fonctions d'auth.
 */
void chargerUtilisateurs();

/**
 * @brief Sauvegarde tous les utilisateurs dans le fichier texte.
 *
 * Ecrase le fichier existant avec le contenu actuel de
 * listeUtilisateurs[]. Cree le dossier data/ si absent.
 * A appeler apres toute modification de listeUtilisateurs[].
 *
 * @note Format : id|nom|prenom|email|tel|mdp|role|nbEchecs|actif
 * @note Compatible Windows : utilise "if not exist data mkdir data"
 */
void sauvegarderUtilisateurs();

#endif /* AUTH_H_INCLUDED */
