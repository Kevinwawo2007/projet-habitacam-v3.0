/* ============================================================
 * @file    main.c
 * @brief   Point d'entree principal de HabitatCam V2.0
 * @version 2.0
 * @author  CHEDJOU
 * ============================================================ */

#include <stdio.h>
#include "structures.h"
#include "auth.h"
#include "logement.h"
#include "locataire.h"
#include "bailleur.h"
#include "administrateur.h"
#include "matching.h"
#include "favoris.h"

/**
 * @brief Redirige vers le bon menu selon le role connecte.
 */
static void lancerMenuRole() {
    switch (sessionCourante.utilisateur.role) {
        case ROLE_LOCATAIRE:
            /* Afficher les recommandations a la connexion */
            afficherRecommandations(sessionCourante.utilisateur.id);
            verifierAlertesFavoris(sessionCourante.utilisateur.id);
            menuLocataire();
            break;
        case ROLE_BAILLEUR:
            menuBailleur();
            break;
        case ROLE_ADMINISTRATEUR:
            menuAdministrateur();
            break;
        default:
            printf("[ERREUR] Role inconnu.\n");
    }
}

/**
 * @brief Point d'entree du programme.
 */
int main() {

    printf("============================================\n");
    printf("   HABITATCAM - Plateforme de logement     \n");
    printf("        Cameroun  |  Version 2.0           \n");
    printf("============================================\n");

    /* Chargement des donnees au demarrage */
    chargerUtilisateurs();
    chargerLogements();
    chargerReservations();

    /* Boucle principale */
    int continuer = 1;
    while (continuer) {
        menuAuthentification();
        if (sessionCourante.connecte) {
            lancerMenuRole();
        } else {
            continuer = 0;
        }
    }

    printf("\n============================================\n");
    printf("   Merci d'avoir utilise HabitatCam !      \n");
    printf("============================================\n");

    return 0;
}
