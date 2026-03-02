/* ============================================================
 *  HabitatCam — Plateforme de logement au Cameroun
 *  Fichier     : administrateur.h
 *  Version     : 1.0
 *  Description : Module Administrateur — gestion des utilisateurs
 *                et des logements depuis le panneau admin.
 * ============================================================ */

#ifndef ADMINISTRATEUR_H_INCLUDED
#define ADMINISTRATEUR_H_INCLUDED

#include "Structures.h"

/* ── Point d'entree du module ──────────────────────────────── */
void menuAdministrateur();

/* ── Gestion des utilisateurs ──────────────────────────────── */
void adminVoirUtilisateurs();
void adminToggleCompte();
void adminSupprimerUtilisateur();

/* ── Gestion des logements ─────────────────────────────────── */
void adminVoirLogements();
void adminSupprimerLogement();

/* ── Statistiques ──────────────────────────────────────────── */
void adminStatistiques();

#endif /* ADMINISTRATEUR_H_INCLUDED */
