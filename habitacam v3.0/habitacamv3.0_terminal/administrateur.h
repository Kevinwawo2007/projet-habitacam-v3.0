/* ============================================================
 * @file    administrateur.h
 * @brief   Declarations du module Administrateur V2.0.
 * @version 2.0
 * @author  SOUOPGUI
 * ============================================================ */

#ifndef ADMINISTRATEUR_H_INCLUDED
#define ADMINISTRATEUR_H_INCLUDED

#include "structures.h"
#include "auth.h"

void menuAdministrateur();
void adminVoirUtilisateurs();
void adminToggleCompte();
void adminReactiverCompte();
void adminSupprimerUtilisateur();
void adminVoirLogements();
void adminSupprimerLogement();
void adminStatistiques();
void adminGererDemandes();

#endif /* ADMINISTRATEUR_H_INCLUDED */
