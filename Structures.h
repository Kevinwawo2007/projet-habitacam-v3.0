/* ============================================================
 * @file    structures.h
 * @brief   Structures, constantes et enumerations de HabitatCam.
 * @version 2.0
 * ============================================================ */

#ifndef STRUCTURES_H_INCLUDED
#define STRUCTURES_H_INCLUDED

/* ============================================================
 * CONSTANTES GLOBALES
 * ============================================================ */
#define MAX_UTILISATEURS   100
#define MAX_LOGEMENTS      200
#define MAX_RESERVATIONS   300

#define TAILLE_NOM         50
#define TAILLE_EMAIL       80
#define TAILLE_MDP         50
#define TAILLE_TEL         20
#define TAILLE_VILLE       50
#define TAILLE_QUARTIER    50
#define TAILLE_TITRE       80
#define TAILLE_DESC        200
#define TAILLE_TYPE        30

/* ============================================================
 * FICHIERS DE STOCKAGE
 * ============================================================ */
#define FICHIER_UTILISATEURS  "data/utilisateurs.txt"
#define FICHIER_LOGEMENTS     "data/logements.txt"
#define FICHIER_RESERVATIONS  "data/reservations.txt"

/* ============================================================
 * @brief Codes de retour des fonctions d'authentification.
 *
 * Inspire du module du professeur. Permet de savoir exactement
 * pourquoi une operation a reussi ou echoue au lieu d'un
 * simple 0 ou -1.
 *
 * Exemple d'utilisation :
 *   AuthStatus st = inscrireUtilisateur();
 *   if (st == AUTH_ERR_EXISTS)
 *       printf("Email deja utilise.\n");
 * ============================================================ */
typedef enum
{
    AUTH_OK           =  0,  /* Operation reussie                 */
    AUTH_ERR_IO       = -1,  /* Probleme d'ouverture de fichier   */
    AUTH_ERR_NOTFOUND = -2,  /* Aucun compte avec cet email       */
    AUTH_ERR_EXISTS   = -3,  /* Email deja utilise                */
    AUTH_ERR_INVALID  = -4   /* Mauvais mot de passe ou inactif   */
} AuthStatus;

/* ============================================================
 * ROLES UTILISATEUR
 * ============================================================ */
typedef enum
{
    ROLE_LOCATAIRE      = 1,
    ROLE_BAILLEUR       = 2,
    ROLE_ADMINISTRATEUR = 3
} Role;

/* ============================================================
 * STATUT D'UN LOGEMENT
 * ============================================================ */
typedef enum
{
    STATUT_DISPONIBLE   = 1,
    STATUT_RESERVE      = 2,
    STATUT_INDISPONIBLE = 3
} StatutLogement;

/* ============================================================
 * STATUT D'UNE RESERVATION
 * ============================================================ */
typedef enum
{
    RES_EN_ATTENTE = 1,
    RES_CONFIRMEE  = 2,
    RES_ANNULEE    = 3
} StatutReservation;

/* ============================================================
 * @brief Structure Utilisateur.
 *
 * Represente un compte inscrit sur la plateforme.
 * Le champ nbEchecs compte les tentatives de connexion
 * echouees consecutives. Quand il atteint 3, le compte
 * est automatiquement desactive (actif = 0).
 * ============================================================ */
typedef struct
{
    int  id;
    char nom[TAILLE_NOM];
    char prenom[TAILLE_NOM];
    char email[TAILLE_EMAIL];
    char telephone[TAILLE_TEL];
    char motDePasse[TAILLE_MDP];
    Role role;
    int  nbEchecs;  /* Nb de tentatives echouees consecutives */
    int  actif;     /* 1 = actif, 0 = desactive par l'admin  */
} Utilisateur;

/* ============================================================
 * @brief Structure Logement.
 *
 * Represente une annonce publiee par un bailleur.
 * ============================================================ */
typedef struct
{
    int   id;
    char  titre[TAILLE_TITRE];
    char  type[TAILLE_TYPE];
    char  description[TAILLE_DESC];
    char  ville[TAILLE_VILLE];
    char  quartier[TAILLE_QUARTIER];
    float superficie;
    int   nbPieces;
    float prixMensuel;
    StatutLogement statut;
    int   idBailleur;
} Logement;

/* ============================================================
 * @brief Structure Reservation.
 *
 * Represente une demande de reservation faite par un locataire.
 * ============================================================ */
typedef struct
{
    int   id;
    int   idLocataire;
    int   idLogement;
    char  dateReservation[20];
    StatutReservation statut;
} Reservation;

/* ============================================================
 * @brief Structure Session.
 *
 * Contient les informations de l'utilisateur actuellement
 * connecte. connecte = 0 signifie aucune session active.
 * ============================================================ */
typedef struct
{
    int         connecte;
    Utilisateur utilisateur;
} Session;

#endif /* STRUCTURES_H_INCLUDED */
