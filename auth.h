/* ============================================================
 *  HabitatCam — Plateforme de logement au Cameroun
 *  Fichier     : auth.h
 *  Version     : 1.0
 *  Description : Module d'authentification — inscriptions,
 *                connexions, gestion de session.
 * ============================================================ */

#ifndef AUTH_H_INCLUDED
#define AUTH_H_INCLUDED

#include "Structure.h"

/* ── Variables globales partagées ──────────────────────────── */
extern Utilisateur  listeUtilisateurs[MAX_UTILISATEURS];
extern int          nbUtilisateurs;
extern Session      sessionCourante;

/* ── Fonctions principales ─────────────────────────────────── */

/* Affiche le menu de bienvenue (connexion / inscription / quitter) */
void menuAuthentification();

/* Inscription d'un nouvel utilisateur */
void inscrireUtilisateur();

/* Connexion d'un utilisateur existant */
void connecterUtilisateur();

/* Déconnexion — réinitialise la session courante */
void deconnecterUtilisateur();

/* ── Fonctions utilitaires internes ────────────────────────── */

/* Vérifie si un email est déjà utilisé (retourne 1 si oui) */
int emailExiste(const char *email);

/* Recherche un utilisateur par email et mot de passe
   Retourne l'index dans listeUtilisateurs, ou -1 si non trouvé */
int rechercherUtilisateur(const char *email, const char *mdp);

/* Génère un nouvel ID unique pour un utilisateur */
int genererIdUtilisateur();

/* ── Fonctions de persistance ──────────────────────────────── */

/* Charge les utilisateurs depuis le fichier texte */
void chargerUtilisateurs();

/* Sauvegarde tous les utilisateurs dans le fichier texte */
void sauvegarderUtilisateurs();

#endif /* AUTH_H_INCLUDED */
