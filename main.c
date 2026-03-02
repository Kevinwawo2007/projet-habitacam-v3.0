/* ============================================================
 * @file    main.c
 * @brief   Point d'entree principal de HabitatCam V1.0
 * @version 1.0
 * @author  CHEDJOU
 * ============================================================ */

#include <stdio.h>
#include "structures.h"
#include "auth.h"
#include "logement.h"
#include "locataire.h"
#include "bailleur.h"
#include "administrateur.h"

/**
 * @brief Redirige l'utilisateur connecte vers son menu selon son role.
 */
static void lancerMenuRole()
{
    switch (sessionCourante.utilisateur.role)
    {
    case ROLE_LOCATAIRE:
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
 * @brief Fonction principale - point d'entree du programme.
 * @return 0 si le programme se termine normalement.
 */
int main()
{

    printf("============================================\n");
    printf("   HABITATCAM - Plateforme de logement     \n");
    printf("        Cameroun  |  Version 1.0           \n");
    printf("============================================\n");

    /* Chargement des donnees au demarrage */
    chargerUtilisateurs();
    chargerLogements();
    chargerReservations();

    /* Boucle principale */
    int continuer = 1;
    while (continuer)
    {

        menuAuthentification();

        if (sessionCourante.connecte)
        {
            lancerMenuRole();
        }
        else
        {
            continuer = 0;
        }
    }

    printf("\n============================================\n");
    printf("   Merci d'avoir utilise HabitatCam !      \n");
    printf("============================================\n");

    return 0;
}
