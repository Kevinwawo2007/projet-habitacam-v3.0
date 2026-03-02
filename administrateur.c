/* ============================================================
 * @file    administrateur.c
 * @brief   Module Administrateur de HabitatCam.
 * @version 1.0
 * @author  SOUOPGUI
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structures.h"
#include "auth.h"
#include "administrateur.h"

extern Logement    listeLogements[MAX_LOGEMENTS];
extern int         nbLogements;
extern Reservation listeReservations[MAX_RESERVATIONS];
extern int         nbReservations;
extern void        sauvegarderLogements();

/* ============================================================
 * UTILITAIRES
 * ============================================================ */

static void viderBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void separateur()
{
    printf("---------------------------------------------\n");
}

/**
 * @brief Convertit un role en texte lisible.
 */
static const char* roleEnTexte(Role r)
{
    switch (r)
    {
    case ROLE_LOCATAIRE:
        return "Locataire";
    case ROLE_BAILLEUR:
        return "Bailleur";
    case ROLE_ADMINISTRATEUR:
        return "Administrateur";
    default:
        return "Inconnu";
    }
}

/**
 * @brief Convertit un statut logement en texte lisible.
 */
static const char* statutEnTexte(StatutLogement s)
{
    switch (s)
    {
    case STATUT_DISPONIBLE:
        return "Disponible";
    case STATUT_RESERVE:
        return "Reserve";
    case STATUT_INDISPONIBLE:
        return "Indisponible";
    default:
        return "Inconnu";
    }
}

/**
 * @brief Verifie que l'utilisateur connecte est bien un administrateur.
 * @return 1 si admin, 0 sinon.
 */
static int verifierAdmin()
{
    if (!sessionCourante.connecte ||
            sessionCourante.utilisateur.role != ROLE_ADMINISTRATEUR)
    {
        printf("[ACCES REFUSE] Section reservee a l'administrateur.\n");
        return 0;
    }
    return 1;
}

/* ============================================================
 * GESTION DES UTILISATEURS
 * ============================================================ */

/**
 * @brief Affiche la liste de tous les utilisateurs inscrits.
 */
void adminVoirUtilisateurs()
{
    if (!verifierAdmin()) return;

    printf("\n[LISTE DES UTILISATEURS]\n");
    if (nbUtilisateurs == 0)
    {
        printf("Aucun utilisateur.\n");
        return;
    }

    separateur();
    printf("%-4s %-15s %-15s %-25s %-14s %s\n",
           "ID", "Nom", "Prenom", "Email", "Role", "Statut");
    separateur();
    for (int i = 0; i < nbUtilisateurs; i++)
    {
        Utilisateur *u = &listeUtilisateurs[i];
        printf("%-4d %-15s %-15s %-25s %-14s %s\n",
               u->id, u->nom, u->prenom, u->email,
               roleEnTexte(u->role),
               u->actif ? "Actif" : "DESACTIVE");
    }
    separateur();
    printf("Total : %d utilisateur(s)\n", nbUtilisateurs);
}

/**
 * @brief Active ou desactive le compte d'un utilisateur.
 */
void adminToggleCompte()
{
    if (!verifierAdmin()) return;
    adminVoirUtilisateurs();

    int id;
    printf("\nID a activer/desactiver : ");
    scanf("%d", &id);
    viderBuffer();

    if (id == sessionCourante.utilisateur.id)
    {
        printf("[ERREUR] Impossible de modifier votre propre compte.\n");
        return;
    }
    for (int i = 0; i < nbUtilisateurs; i++)
    {
        if (listeUtilisateurs[i].id == id)
        {
            if (listeUtilisateurs[i].role == ROLE_ADMINISTRATEUR)
            {
                printf("[ERREUR] Impossible de modifier un administrateur.\n");
                return;
            }
            listeUtilisateurs[i].actif = !listeUtilisateurs[i].actif;
            sauvegarderUtilisateurs();
            printf("[OK] Compte %s : %s\n", listeUtilisateurs[i].prenom,
                   listeUtilisateurs[i].actif ? "ACTIF" : "DESACTIVE");
            return;
        }
    }
    printf("[ERREUR] ID %d introuvable.\n", id);
}

/**
 * @brief Supprime definitivement un utilisateur.
 */
void adminSupprimerUtilisateur()
{
    if (!verifierAdmin()) return;
    adminVoirUtilisateurs();

    int id;
    printf("\nID a supprimer : ");
    scanf("%d", &id);
    viderBuffer();

    if (id == sessionCourante.utilisateur.id)
    {
        printf("[ERREUR] Impossible de supprimer votre propre compte.\n");
        return;
    }
    for (int i = 0; i < nbUtilisateurs; i++)
    {
        if (listeUtilisateurs[i].id == id)
        {
            if (listeUtilisateurs[i].role == ROLE_ADMINISTRATEUR)
            {
                printf("[ERREUR] Impossible de supprimer un administrateur.\n");
                return;
            }
            char prenom[TAILLE_NOM];
            strcpy(prenom, listeUtilisateurs[i].prenom);
            for (int j = i; j < nbUtilisateurs - 1; j++)
                listeUtilisateurs[j] = listeUtilisateurs[j + 1];
            nbUtilisateurs--;
            sauvegarderUtilisateurs();
            printf("[OK] Compte de %s supprime.\n", prenom);
            return;
        }
    }
    printf("[ERREUR] ID %d introuvable.\n", id);
}

/* ============================================================
 * GESTION DES LOGEMENTS
 * ============================================================ */

/**
 * @brief Affiche tous les logements de la plateforme.
 */
void adminVoirLogements()
{
    if (!verifierAdmin()) return;

    printf("\n[LISTE DES LOGEMENTS]\n");
    if (nbLogements == 0)
    {
        printf("Aucun logement.\n");
        return;
    }

    separateur();
    printf("%-4s %-20s %-12s %-15s %-10s %-12s %s\n",
           "ID", "Titre", "Type", "Ville", "Surf(m2)", "Prix(FCFA)", "Statut");
    separateur();
    for (int i = 0; i < nbLogements; i++)
    {
        Logement *l = &listeLogements[i];
        printf("%-4d %-20s %-12s %-15s %-10.1f %-12.0f %s\n",
               l->id, l->titre, l->type, l->ville,
               l->superficie, l->prixMensuel,
               statutEnTexte(l->statut));
    }
    separateur();
    printf("Total : %d logement(s)\n", nbLogements);
}

/**
 * @brief Supprime un logement de la plateforme.
 */
void adminSupprimerLogement()
{
    if (!verifierAdmin()) return;
    adminVoirLogements();

    int id;
    printf("\nID du logement a supprimer : ");
    scanf("%d", &id);
    viderBuffer();

    for (int i = 0; i < nbLogements; i++)
    {
        if (listeLogements[i].id == id)
        {
            char titre[TAILLE_TITRE];
            strcpy(titre, listeLogements[i].titre);
            for (int j = i; j < nbLogements - 1; j++)
                listeLogements[j] = listeLogements[j + 1];
            nbLogements--;
            sauvegarderLogements();
            printf("[OK] Logement \"%s\" supprime.\n", titre);
            return;
        }
    }
    printf("[ERREUR] ID %d introuvable.\n", id);
}

/* ============================================================
 * STATISTIQUES
 * ============================================================ */

/**
 * @brief Affiche le tableau de bord general de la plateforme.
 */
void adminStatistiques()
{
    if (!verifierAdmin()) return;

    int nbLoc=0, nbBaill=0, nbAdm=0, nbActifs=0, nbInactifs=0;
    int nbDispo=0, nbRes=0, nbIndispo=0;

    for (int i = 0; i < nbUtilisateurs; i++)
    {
        switch (listeUtilisateurs[i].role)
        {
        case ROLE_LOCATAIRE:
            nbLoc++;
            break;
        case ROLE_BAILLEUR:
            nbBaill++;
            break;
        case ROLE_ADMINISTRATEUR:
            nbAdm++;
            break;
        }
        listeUtilisateurs[i].actif ? nbActifs++ : nbInactifs++;
    }
    for (int i = 0; i < nbLogements; i++)
    {
        switch (listeLogements[i].statut)
        {
        case STATUT_DISPONIBLE:
            nbDispo++;
            break;
        case STATUT_RESERVE:
            nbRes++;
            break;
        case STATUT_INDISPONIBLE:
            nbIndispo++;
            break;
        }
    }

    printf("\n[TABLEAU DE BORD]\n");
    printf("\nUTILISATEURS (%d)\n", nbUtilisateurs);
    separateur();
    printf("  Locataires      : %d\n", nbLoc);
    printf("  Bailleurs       : %d\n", nbBaill);
    printf("  Administrateurs : %d\n", nbAdm);
    printf("  Actifs          : %d  |  Desactives : %d\n", nbActifs, nbInactifs);
    printf("\nLOGEMENTS (%d)\n", nbLogements);
    separateur();
    printf("  Disponibles     : %d\n", nbDispo);
    printf("  Reserves        : %d\n", nbRes);
    printf("  Indisponibles   : %d\n", nbIndispo);
    printf("\nRESERVATIONS\n");
    separateur();
    printf("  Total           : %d\n", nbReservations);
}

/* ============================================================
 * MENU ADMINISTRATEUR
 * ============================================================ */

/**
 * @brief Point d'entree du module administrateur.
 */
void menuAdministrateur()
{
    if (!verifierAdmin()) return;

    int choix;
    do
    {
        printf("\n=== PANNEAU ADMINISTRATEUR ===\n");
        printf("Connecte : %s\n", sessionCourante.utilisateur.prenom);
        printf("------------------------------\n");
        printf("1. Voir tous les utilisateurs\n");
        printf("2. Activer / Desactiver un compte\n");
        printf("3. Supprimer un utilisateur\n");
        printf("4. Voir tous les logements\n");
        printf("5. Supprimer un logement\n");
        printf("6. Statistiques generales\n");
        printf("0. Se deconnecter\n");
        printf("------------------------------\n");
        printf("Choix : ");
        scanf("%d", &choix);
        viderBuffer();

        switch (choix)
        {
        case 1:
            adminVoirUtilisateurs();
            break;
        case 2:
            adminToggleCompte();
            break;
        case 3:
            adminSupprimerUtilisateur();
            break;
        case 4:
            adminVoirLogements();
            break;
        case 5:
            adminSupprimerLogement();
            break;
        case 6:
            adminStatistiques();
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
