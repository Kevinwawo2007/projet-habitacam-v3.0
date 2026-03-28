/* ============================================================
 * @file    logement.c
 * @brief   Module Logement de HabitatCam.
 *
 * Gere l'ajout, l'affichage, la recherche et la suppression
 * des logements. Utilise le tableau global listeLogements[]
 * et le fichier data/logements.txt pour la persistance.
 *
 * @version 1.0
 * @author  FOMEKONG (corrige par SOUOPGUI)
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structures.h"
#include "auth.h"
#include "logement.h"

/* Variables de auth.c necessaires pour afficher le nom du bailleur */
extern Utilisateur listeUtilisateurs[MAX_UTILISATEURS];
extern int         nbUtilisateurs;

/* ============================================================
 * VARIABLES GLOBALES
 * ============================================================ */

/** @brief Tableau de tous les logements charges en memoire. */
Logement listeLogements[MAX_LOGEMENTS];

/** @brief Nombre de logements actuellement enregistres. */
int nbLogements = 0;


/* ============================================================
 * UTILITAIRES
 * ============================================================ */

/**
 * @brief Vide le tampon du clavier apres un scanf().
 */
static void viderBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * @brief Genere un ID unique pour un nouveau logement.
 * @return Le plus grand ID existant + 1.
 */
int genererIdLogement()
{
    int maxId = 0;
    for (int i = 0; i < nbLogements; i++)
    {
        if (listeLogements[i].id > maxId)
            maxId = listeLogements[i].id;
    }
    return maxId + 1;
}

/**
 * @brief Recherche le nom et le telephone d'un bailleur par son ID.
 *
 * Parcourt listeUtilisateurs[] pour trouver l'utilisateur dont l'ID
 * correspond a idBailleur, puis copie son nom et telephone dans les
 * tableaux fournis. Utilise dans afficherLogements() et rechercherLogement().
 *
 * @param idBailleur  ID du bailleur a rechercher.
 * @param nom         Tableau ou ecrire le nom complet (prenom + nom).
 * @param telephone   Tableau ou ecrire le numero de telephone.
 * @note  Si le bailleur n'est pas trouve, les valeurs par defaut
 *        "Inconnu" et "Non disponible" sont utilisees.
 */
static void trouverBailleur(int idBailleur, char *nom, char *telephone)
{
    strcpy(nom,       "Inconnu");
    strcpy(telephone, "Non disponible");

    for (int i = 0; i < nbUtilisateurs; i++)
    {
        if (listeUtilisateurs[i].id == idBailleur)
        {
            snprintf(nom, 100, "%s %s",
                     listeUtilisateurs[i].prenom,
                     listeUtilisateurs[i].nom);
            strncpy(telephone, listeUtilisateurs[i].telephone, 19);
            telephone[19] = '\0';
            return;
        }
    }
}


/* ============================================================
 * PERSISTANCE
 * ============================================================ */

/**
 * @brief Charge les logements depuis le fichier texte en memoire.
 *
 * Lit data/logements.txt et remplit listeLogements[].
 * Si le fichier est absent, le tableau reste vide (normal
 * au premier lancement, aucun logement n'existe encore).
 *
 * @note Format d'une ligne : id|titre|type|ville|quartier|superficie|nbPieces|prix|statut|idBailleur
 */
void chargerLogements()
{
    FILE *f = fopen(FICHIER_LOGEMENTS, "r");
    if (f == NULL) return; /* Pas de fichier = pas de logements, c'est normal */

    nbLogements = 0;
    Logement l;
    while (fscanf(f, "%d|%79[^|]|%29[^|]|%199[^|]|%49[^|]|%49[^|]|%f|%d|%f|%d|%d\n",
                  &l.id, l.titre, l.type, l.description,
                  l.ville, l.quartier,
                  &l.superficie, &l.nbPieces,
                  &l.prixMensuel, (int*)&l.statut,
                  &l.idBailleur) == 11)
    {
        listeLogements[nbLogements++] = l;
        if (nbLogements >= MAX_LOGEMENTS) break;
    }
    fclose(f);
}

/**
 * @brief Sauvegarde tous les logements du tableau dans le fichier texte.
 *
 * Ecrase le fichier existant avec le contenu actuel de listeLogements[].
 *
 * @note A appeler apres chaque ajout ou suppression de logement.
 */
void sauvegarderLogements()
{
    system("if not exist data mkdir data");
    FILE *f = fopen(FICHIER_LOGEMENTS, "w");
    if (f == NULL)
    {
        printf("[ERREUR] Impossible d'ouvrir le fichier logements.\n");
        return;
    }
    for (int i = 0; i < nbLogements; i++)
    {
        Logement *l = &listeLogements[i];
        fprintf(f, "%d|%s|%s|%s|%s|%s|%.1f|%d|%.0f|%d|%d\n",
                l->id, l->titre, l->type, l->description,
                l->ville, l->quartier,
                l->superficie, l->nbPieces,
                l->prixMensuel, (int)l->statut,
                l->idBailleur);
    }
    fclose(f);
}


/* ============================================================
 * FONCTIONS PRINCIPALES
 * ============================================================ */

/**
 * @brief Permet a un bailleur connecte d'ajouter un logement.
 *
 * Saisit les informations du logement, genere un ID automatique,
 * lie le logement au bailleur connecte et sauvegarde.
 *
 * @warning Accessible uniquement aux utilisateurs de role ROLE_BAILLEUR.
 */
void ajouterLogement()
{

    /* Verification du role */
    if (sessionCourante.utilisateur.role != ROLE_BAILLEUR)
    {
        printf("[ERREUR] Seul un bailleur peut ajouter un logement.\n");
        return;
    }

    if (nbLogements >= MAX_LOGEMENTS)
    {
        printf("[ERREUR] Nombre maximum de logements atteint.\n");
        return;
    }

    Logement l;

    printf("\n--- AJOUTER UN LOGEMENT ---\n");

    printf("Titre        : ");
    fgets(l.titre,       TAILLE_TITRE,   stdin);
    l.titre[strcspn(l.titre, "\n")]             = '\0';
    printf("Type         : ");
    fgets(l.type,        TAILLE_TYPE,    stdin);
    l.type[strcspn(l.type, "\n")]               = '\0';
    printf("Description  : ");
    fgets(l.description, TAILLE_DESC,    stdin);
    l.description[strcspn(l.description, "\n")] = '\0';
    printf("Ville        : ");
    fgets(l.ville,       TAILLE_VILLE,   stdin);
    l.ville[strcspn(l.ville, "\n")]             = '\0';
    printf("Quartier     : ");
    fgets(l.quartier,    TAILLE_QUARTIER,stdin);
    l.quartier[strcspn(l.quartier, "\n")]       = '\0';

    printf("Superficie   : ");
    scanf("%f", &l.superficie);
    viderBuffer();
    printf("Nb pieces    : ");
    scanf("%d", &l.nbPieces);
    viderBuffer();
    printf("Prix/mois    : ");
    scanf("%f", &l.prixMensuel);
    viderBuffer();

    /* Champs generes automatiquement */
    l.id         = genererIdLogement();
    l.statut     = STATUT_DISPONIBLE;
    l.idBailleur = sessionCourante.utilisateur.id;

    listeLogements[nbLogements++] = l;
    sauvegarderLogements();

    printf("[OK] Logement \"%s\" ajoute avec succes ! (ID: %d)\n", l.titre, l.id);
}

/**
 * @brief Affiche tous les logements disponibles sur la plateforme.
 *
 * Parcourt listeLogements[] et affiche uniquement ceux dont
 * le statut est STATUT_DISPONIBLE.
 */
void afficherLogements()
{
    printf("\n--- LOGEMENTS DISPONIBLES ---\n");

    int trouve = 0;
    for (int i = 0; i < nbLogements; i++)
    {
        Logement *l = &listeLogements[i];
        if (l->statut == STATUT_DISPONIBLE)
        {
            /* Recuperer nom et telephone du bailleur */
            char nomBailleur[100], telBailleur[20];
            trouverBailleur(l->idBailleur, nomBailleur, telBailleur);

            printf("\nID           : %d",      l->id);
            printf("\nTitre        : %s",      l->titre);
            printf("\nType         : %s",      l->type);
            printf("\nVille        : %s - %s", l->ville, l->quartier);
            printf("\nSurface      : %.1f m2", l->superficie);
            printf("\nPieces       : %d",      l->nbPieces);
            printf("\nPrix         : %.0f FCFA/mois", l->prixMensuel);
            printf("\nProprietaire : %s",      nomBailleur);
            printf("\nTelephone    : %s",      telBailleur);
            printf("\n-----------------------------\n");
            trouve = 1;
        }
    }

    if (!trouve)
        printf("Aucun logement disponible pour le moment.\n");
}

/**
 * @brief Permet de rechercher un logement selon des criteres.
 *
 * Propose trois filtres : par prix maximum, par superficie
 * minimum, ou par ville. Affiche les logements correspondants
 * parmi ceux qui sont disponibles.
 */
void rechercherLogement()
{
    int choix;
    printf("\n--- RECHERCHER UN LOGEMENT ---\n");
    printf("1. Par prix maximum\n");
    printf("2. Par superficie minimum\n");
    printf("3. Par ville\n");
    printf("Choix : ");
    scanf("%d", &choix);
    viderBuffer();

    int trouve = 0;

    if (choix == 1)
    {

        float prixMax;
        printf("Prix maximum (FCFA) : ");
        scanf("%f", &prixMax);
        viderBuffer();

        for (int i = 0; i < nbLogements; i++)
        {
            Logement *l = &listeLogements[i];
            if (l->statut == STATUT_DISPONIBLE && l->prixMensuel <= prixMax)
            {
                char nom1[100], tel1[20];
                trouverBailleur(l->idBailleur, nom1, tel1);
                printf("\nID: %d | %s | %s | %.0f FCFA | %s | %s\n",
                       l->id, l->titre, l->ville, l->prixMensuel, nom1, tel1);
                trouve = 1;
            }
        }

    }
    else if (choix == 2)
    {

        float surfMin;
        printf("Superficie minimum (m2) : ");
        scanf("%f", &surfMin);
        viderBuffer();

        for (int i = 0; i < nbLogements; i++)
        {
            Logement *l = &listeLogements[i];
            if (l->statut == STATUT_DISPONIBLE && l->superficie >= surfMin)
            {
                char nom2[100], tel2[20];
                trouverBailleur(l->idBailleur, nom2, tel2);
                printf("\nID: %d | %s | %.1f m2 | %.0f FCFA | %s | %s\n",
                       l->id, l->titre, l->superficie, l->prixMensuel, nom2, tel2);
                trouve = 1;
            }
        }

    }
    else if (choix == 3)
    {

        char ville[TAILLE_VILLE];
        printf("Ville : ");
        fgets(ville, TAILLE_VILLE, stdin);
        ville[strcspn(ville, "\n")] = '\0';

        for (int i = 0; i < nbLogements; i++)
        {
            Logement *l = &listeLogements[i];
            if (l->statut == STATUT_DISPONIBLE &&
                    strstr(l->ville, ville) != NULL)
            {
                char nom1[100], tel1[20];
                trouverBailleur(l->idBailleur, nom1, tel1);
                printf("\nID: %d | %s | %s | %.0f FCFA | %s | %s\n",
                       l->id, l->titre, l->ville, l->prixMensuel, nom1, tel1);
                trouve = 1;
            }
        }

    }
    else
    {
        printf("[ERREUR] Choix invalide.\n");
        return;
    }

    if (!trouve)
        printf("Aucun logement trouve pour ces criteres.\n");
}

/**
 * @brief Supprime un logement appartenant au bailleur connecte.
 *
 * Verifie que le logement appartient bien au bailleur connecte
 * avant de le supprimer. L'administrateur peut supprimer
 * n'importe quel logement via adminSupprimerLogement().
 *
 * @warning Accessible uniquement au role ROLE_BAILLEUR.
 * @warning Action irreversible.
 */
void supprimerLogement()
{

    if (sessionCourante.utilisateur.role != ROLE_BAILLEUR)
    {
        printf("[ERREUR] Seul un bailleur peut supprimer ses logements.\n");
        return;
    }

    afficherLogements();

    int id;
    printf("\nID du logement a supprimer : ");
    scanf("%d", &id);
    viderBuffer();

    for (int i = 0; i < nbLogements; i++)
    {
        if (listeLogements[i].id == id)
        {

            /* Verification que le logement appartient au bailleur connecte */
            if (listeLogements[i].idBailleur != sessionCourante.utilisateur.id)
            {
                printf("[ERREUR] Ce logement ne vous appartient pas.\n");
                return;
            }

            /* Decalage du tableau pour supprimer l'entree */
            for (int j = i; j < nbLogements - 1; j++)
                listeLogements[j] = listeLogements[j + 1];

            nbLogements--;
            sauvegarderLogements();
            printf("[OK] Logement supprime.\n");
            return;
        }
    }
    printf("[ERREUR] Aucun logement avec l'ID %d.\n", id);
}
