/* ============================================================
 * @file    bailleur.c
 * @brief   Module Bailleur de HabitatCam.
 *
 * Fournit le menu et les actions disponibles pour un bailleur
 * connecte : gerer ses annonces et voir ses reservations.
 *
 * @version 1.0
 * @author  TOGNANG
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include "structures.h"
#include "auth.h"
#include "logement.h"
#include "bailleur.h"

/* -- Variables extern definies dans reservations (main.c) -- */
extern Reservation listeReservations[MAX_RESERVATIONS];
extern int         nbReservations;

/**
 * @brief Vide le tampon du clavier apres un scanf().
 */
static void viderBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * @brief Affiche les logements appartenant au bailleur connecte.
 *
 * Parcourt listeLogements[] et affiche uniquement les logements
 * dont idBailleur correspond a l'ID de l'utilisateur en session.
 */
static void voirMesAnnonces()
{
    int idBailleur = sessionCourante.utilisateur.id;
    int trouve = 0;

    printf("\n--- MES ANNONCES ---\n");
    for (int i = 0; i < nbLogements; i++)
    {
        Logement *l = &listeLogements[i];
        if (l->idBailleur == idBailleur)
        {
            printf("ID: %d | %s | %s | %.0f FCFA | ",
                   l->id, l->titre, l->ville, l->prixMensuel);
            switch (l->statut)
            {
            case STATUT_DISPONIBLE:
                printf("Disponible\n");
                break;
            case STATUT_RESERVE:
                printf("Reserve\n");
                break;
            case STATUT_INDISPONIBLE:
                printf("Indisponible\n");
                break;
            }
            trouve = 1;
        }
    }
    if (!trouve)
        printf("Vous n'avez aucune annonce pour le moment.\n");
}

/**
 * @brief Affiche les reservations en attente sur les logements du bailleur.
 *
 * Parcourt listeReservations[] et affiche celles dont le logement
 * appartient au bailleur connecte et dont le statut est RES_EN_ATTENTE.
 */
static void voirMesReservations()
{
    int idBailleur = sessionCourante.utilisateur.id;
    int trouve = 0;

    printf("\n--- MES RESERVATIONS EN ATTENTE ---\n");
    for (int i = 0; i < nbReservations; i++)
    {
        Reservation *r = &listeReservations[i];

        /* Trouver le logement correspondant */
        for (int j = 0; j < nbLogements; j++)
        {
            if (listeLogements[j].id == r->idLogement &&
                    listeLogements[j].idBailleur == idBailleur &&
                    r->statut == RES_EN_ATTENTE)
            {

                printf("Reservation ID: %d | Logement: %s | Date: %s\n",
                       r->id, listeLogements[j].titre, r->dateReservation);
                trouve = 1;
            }
        }
    }
    if (!trouve)
        printf("Aucune reservation en attente.\n");
}

/**
 * @brief Point d'entree du module bailleur.
 *
 * Affiche le menu bailleur et traite les choix jusqu'a
 * la deconnexion (choix 0).
 *
 * @warning Accessible uniquement au role ROLE_BAILLEUR.
 */
void menuBailleur()
{
    if (sessionCourante.utilisateur.role != ROLE_BAILLEUR)
    {
        printf("[ACCES REFUSE] Section reservee aux bailleurs.\n");
        return;
    }

    int choix;
    do
    {
        printf("\n=== MENU BAILLEUR ===\n");
        printf("Connecte : %s\n", sessionCourante.utilisateur.prenom);
        printf("1. Ajouter une annonce\n");
        printf("2. Voir mes annonces\n");
        printf("3. Supprimer une annonce\n");
        printf("4. Voir mes reservations\n");
        printf("0. Se deconnecter\n");
        printf("Choix : ");
        scanf("%d", &choix);
        viderBuffer();

        switch (choix)
        {
        case 1:
            ajouterLogement();
            break;
        case 2:
            voirMesAnnonces();
            break;
        case 3:
            supprimerLogement();
            break;
        case 4:
            voirMesReservations();
            break;
        case 0:
            deconnecterUtilisateur();
            break;
        default:
            printf("[ERREUR] Choix invalide.\n");
        }
    }
    while (choix != 0 && sessionCourante.connecte);
}
