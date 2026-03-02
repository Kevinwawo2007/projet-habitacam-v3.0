/* ============================================================
 *  HabitatCam — Plateforme de logement au Cameroun
 *  Fichier     : structures.h
 *  Version     : 1.0
 *  Description : Définition de toutes les structures de données
 *                utilisées dans l'application.
 * ============================================================ */

#ifndef STRUCTURES_H_INCLUDED
#define STRUCTURES_H_INCLUDED

/* ── Constantes globales ───────────────────────────────────── */
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

/* ── Fichiers de stockage ──────────────────────────────────── */
#define FICHIER_UTILISATEURS  "data/utilisateurs.txt"
#define FICHIER_LOGEMENTS     "data/logements.txt"
#define FICHIER_RESERVATIONS  "data/reservations.txt"

/* ── Rôles utilisateur ─────────────────────────────────────── */
typedef enum
{
    ROLE_LOCATAIRE      = 1,
    ROLE_BAILLEUR       = 2,
    ROLE_ADMINISTRATEUR = 3
} Role;

/* ── Statut d'un logement ──────────────────────────────────── */
typedef enum
{
    STATUT_DISPONIBLE   = 1,
    STATUT_RESERVE      = 2,
    STATUT_INDISPONIBLE = 3
} StatutLogement;

/* ── Statut d'une réservation ──────────────────────────────── */
typedef enum
{
    RES_EN_ATTENTE = 1,
    RES_CONFIRMEE  = 2,
    RES_ANNULEE    = 3
} StatutReservation;

/* ══════════════════════════════════════════════════════════════
 *  Structure : Utilisateur
 * ══════════════════════════════════════════════════════════════ */
typedef struct
{
    int  id;
    char nom[TAILLE_NOM];
    char prenom[TAILLE_NOM];
    char email[TAILLE_EMAIL];
    char telephone[TAILLE_TEL];
    char motDePasse[TAILLE_MDP];
    Role role;
    int  actif;   /* 1 = actif, 0 = désactivé par l'admin */
} Utilisateur;

/* ══════════════════════════════════════════════════════════════
 *  Structure : Logement
 * ══════════════════════════════════════════════════════════════ */
typedef struct
{
    int   id;
    char  titre[TAILLE_TITRE];
    char  type[TAILLE_TYPE];        /* Studio, Appartement, Villa... */
    char  description[TAILLE_DESC];
    char  ville[TAILLE_VILLE];
    char  quartier[TAILLE_QUARTIER];
    float superficie;               /* en m²   */
    int   nbPieces;
    float prixMensuel;              /* en FCFA */
    StatutLogement statut;
    int   idBailleur;
} Logement;

/* ══════════════════════════════════════════════════════════════
 *  Structure : Reservation
 * ══════════════════════════════════════════════════════════════ */
typedef struct
{
    int   id;
    int   idLocataire;
    int   idLogement;
    char  dateReservation[20];   /* format : JJ/MM/AAAA */
    StatutReservation statut;
} Reservation;

/* ══════════════════════════════════════════════════════════════
 *  Structure : Session (utilisateur actuellement connecté)
 * ══════════════════════════════════════════════════════════════ */
typedef struct
{
    int         connecte;       /* 1 = session active */
    Utilisateur utilisateur;
} Session;

#endif /* STRUCTURES_H_INCLUDED */
