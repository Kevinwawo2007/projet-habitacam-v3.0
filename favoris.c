/* ============================================================
 * @file    favoris.c
 * @brief   Module de gestion des logements favoris V2.0.
 *
 * Un favori est un logement qu'un locataire sauvegarde pour
 * le surveiller. Si le logement est reserve, le locataire
 * sera alerte des qu'il redevient disponible.
 *
 * @version 2.0
 * @author  TOGNANG
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structures.h"
#include "auth.h"
#include "logement.h"
#include "favoris.h"

#define FICHIER_FAVORIS "data/favoris.txt"
#define MAX_FAVORIS     300

/* ============================================================
 * UTILITAIRES INTERNES
 * ============================================================ */

/**
 * @brief Cree le dossier data/ si absent — cross-platform.
 */
#ifndef _WIN32
#include <sys/stat.h>
#endif

static void creerDossier() {
#ifdef _WIN32
    system("if not exist data mkdir data");
#else
    mkdir("data", 0755);
#endif
}

/**
 * @brief Lit une ligne depuis le clavier.
 */
static void lireLigne(const char *invite, char *buffer, int taille) {
    printf("%s", invite);
    if (fgets(buffer, taille, stdin))
        buffer[strcspn(buffer, "\n\r")] = '\0';
}

/**
 * @brief Lit un entier depuis le clavier.
 * @return L'entier saisi, ou -99 si saisie invalide.
 */
static int lireEntier() {
    char buf[32];
    int  val;
    lireLigne("", buf, sizeof(buf));
    if (sscanf(buf, "%d", &val) != 1) return -99;
    return val;
}

/**
 * @brief Charge tous les favoris depuis data/favoris.txt.
 *
 * @param favoris Tableau ou stocker les favoris.
 * @param nb      Pointeur pour stocker le nombre de favoris lus.
 */
static void chargerFavoris(Favori *favoris, int *nb) {
    *nb = 0;
    FILE *f = fopen(FICHIER_FAVORIS, "r");
    if (f == NULL) return;

    Favori fav;
    while (fscanf(f, "%d|%d|%19[^\n]\n",
                  &fav.idLocataire,
                  &fav.idLogement,
                  fav.dateAjout) == 3) {
        favoris[(*nb)++] = fav;
        if (*nb >= MAX_FAVORIS) break;
    }
    fclose(f);
}

/**
 * @brief Sauvegarde tous les favoris dans data/favoris.txt.
 *
 * @param favoris Tableau de favoris a sauvegarder.
 * @param nb      Nombre de favoris dans le tableau.
 */
static void sauvegarderFavoris(Favori *favoris, int nb) {
    creerDossier();
    FILE *f = fopen(FICHIER_FAVORIS, "w");
    if (f == NULL) {
        printf("[ERREUR] Impossible de sauvegarder les favoris.\n");
        return;
    }
    int i;
    for (i = 0; i < nb; i++) {
        fprintf(f, "%d|%d|%s\n",
                favoris[i].idLocataire,
                favoris[i].idLogement,
                favoris[i].dateAjout);
    }
    fclose(f);
}

/**
 * @brief Verifie si un logement est deja en favori.
 *
 * @param favoris     Tableau de favoris charge.
 * @param nb          Nombre de favoris.
 * @param idLocataire ID du locataire.
 * @param idLogement  ID du logement a verifier.
 * @return 1 si deja en favori, 0 sinon.
 */
static int dejaEnFavori(Favori *favoris, int nb,
                         int idLocataire, int idLogement) {
    int i;
    for (i = 0; i < nb; i++) {
        if (favoris[i].idLocataire == idLocataire &&
            favoris[i].idLogement  == idLogement)
            return 1;
    }
    return 0;
}

/**
 * @brief Trouve le titre d'un logement par son ID.
 *
 * @param idLogement ID du logement.
 * @param titre      Tableau ou ecrire le titre.
 * @param statut     Pointeur ou ecrire le statut du logement.
 * @return 1 si logement trouve, 0 sinon.
 */
static int trouverLogement(int idLogement, char *titre,
                            StatutLogement *statut) {
    int i;
    for (i = 0; i < nbLogements; i++) {
        if (listeLogements[i].id == idLogement) {
            strncpy(titre, listeLogements[i].titre, TAILLE_TITRE - 1);
            titre[TAILLE_TITRE - 1] = '\0';
            *statut = listeLogements[i].statut;
            return 1;
        }
    }
    strcpy(titre, "Logement supprime");
    *statut = STATUT_INDISPONIBLE;
    return 0;
}


/* ============================================================
 * FONCTIONS PRINCIPALES
 * ============================================================ */

/**
 * @brief Ajoute un logement aux favoris du locataire connecte.
 *
 * Affiche les logements disponibles, demande l'ID a ajouter,
 * verifie qu'il n'est pas deja en favori et l'ajoute.
 * On peut aussi ajouter un logement reserve (pour surveiller).
 */
void ajouterFavori() {
    Favori favoris[MAX_FAVORIS];
    int    nb = 0;
    int    idLogement;

    printf("\n[AJOUTER UN FAVORI]\n");
    printf("------------------------------\n");

    /* Afficher tous les logements (pas seulement disponibles)
     * car on peut mettre en favori un logement reserve aussi */
    if (nbLogements == 0) {
        printf("Aucun logement enregistre sur la plateforme.\n");
        return;
    }

    printf("Logements disponibles :\n");
    int i, trouve = 0;
    for (i = 0; i < nbLogements; i++) {
        Logement *l = &listeLogements[i];
        const char *statut = (l->statut == STATUT_DISPONIBLE)
                             ? "Disponible"
                             : (l->statut == STATUT_RESERVE)
                               ? "Reserve" : "Indisponible";
        printf("  ID: %-3d | %-20s | %-10s | %.0f FCFA | %s\n",
               l->id, l->titre, statut, l->prixMensuel, l->ville);
        trouve = 1;
    }

    if (!trouve) {
        printf("Aucun logement disponible.\n");
        return;
    }

    printf("\nID du logement a ajouter aux favoris : ");
    idLogement = lireEntier();

    if (idLogement == -99) {
        printf("[ERREUR] Saisie invalide.\n");
        return;
    }

    /* Verifier que le logement existe */
    int logementExiste = 0;
    for (i = 0; i < nbLogements; i++) {
        if (listeLogements[i].id == idLogement) {
            logementExiste = 1;
            break;
        }
    }
    if (!logementExiste) {
        printf("[ERREUR] Aucun logement avec l'ID %d.\n", idLogement);
        return;
    }

    /* Charger les favoris et verifier les doublons */
    chargerFavoris(favoris, &nb);
    if (dejaEnFavori(favoris, nb,
                     sessionCourante.utilisateur.id, idLogement)) {
        printf("[INFO] Ce logement est deja dans vos favoris.\n");
        return;
    }

    /* Ajouter le favori */
    Favori nouveau;
    nouveau.idLocataire = sessionCourante.utilisateur.id;
    nouveau.idLogement  = idLogement;
    strcpy(nouveau.dateAjout, "29/03/2026");
    favoris[nb++] = nouveau;

    sauvegarderFavoris(favoris, nb);

    /* Afficher le titre du logement ajoute */
    char titre[TAILLE_TITRE];
    StatutLogement st;
    trouverLogement(idLogement, titre, &st);
    printf("[OK] \"%s\" ajoute a vos favoris.\n", titre);
    if (st != STATUT_DISPONIBLE) {
        printf("[INFO] Ce logement n'est pas disponible actuellement.\n");
        printf("       Vous serez alerte a votre prochaine connexion\n");
        printf("       s'il redevient disponible.\n");
    }
}

/**
 * @brief Affiche tous les favoris du locataire connecte.
 *
 * Pour chaque favori, affiche le titre, la ville, le prix
 * et le statut actuel du logement (Disponible / Reserve...).
 */
void voirMesFavoris() {
    Favori favoris[MAX_FAVORIS];
    int    nb = 0;
    int    i, trouve = 0;

    printf("\n[MES LOGEMENTS FAVORIS]\n");
    printf("------------------------------\n");

    chargerFavoris(favoris, &nb);

    for (i = 0; i < nb; i++) {
        if (favoris[i].idLocataire != sessionCourante.utilisateur.id)
            continue;

        char           titre[TAILLE_TITRE];
        StatutLogement statut;
        trouverLogement(favoris[i].idLogement, titre, &statut);

        const char *statutTxt;
        switch (statut) {
            case STATUT_DISPONIBLE:   statutTxt = "Disponible  "; break;
            case STATUT_RESERVE:      statutTxt = "Reserve     "; break;
            case STATUT_INDISPONIBLE: statutTxt = "Indisponible"; break;
            default:                  statutTxt = "Inconnu     ";
        }

        printf("ID: %-3d | %-20s | %s | Ajoute le : %s\n",
               favoris[i].idLogement,
               titre,
               statutTxt,
               favoris[i].dateAjout);
        trouve = 1;
    }

    if (!trouve)
        printf("Vous n'avez aucun logement en favori.\n");

    printf("------------------------------\n");
}

/**
 * @brief Supprime un favori de la liste du locataire.
 *
 * Affiche les favoris, demande l'ID a supprimer et retire
 * la ligne correspondante de data/favoris.txt.
 */
void supprimerFavori() {
    Favori favoris[MAX_FAVORIS];
    int    nb = 0;
    int    i, idLogement;

    voirMesFavoris();

    printf("ID du logement a retirer des favoris : ");
    idLogement = lireEntier();

    if (idLogement == -99) {
        printf("[ERREUR] Saisie invalide.\n");
        return;
    }

    chargerFavoris(favoris, &nb);

    /* Chercher et supprimer le favori */
    int trouve = 0;
    for (i = 0; i < nb; i++) {
        if (favoris[i].idLocataire == sessionCourante.utilisateur.id &&
            favoris[i].idLogement  == idLogement) {
            /* Decaler le tableau pour supprimer cette entree */
            int j;
            for (j = i; j < nb - 1; j++)
                favoris[j] = favoris[j + 1];
            nb--;
            trouve = 1;
            break;
        }
    }

    if (!trouve) {
        printf("[ERREUR] Ce logement n'est pas dans vos favoris.\n");
        return;
    }

    sauvegarderFavoris(favoris, nb);
    printf("[OK] Logement retire de vos favoris.\n");
}

/**
 * @brief Verifie les alertes favoris a la connexion.
 *
 * Appelee automatiquement apres la connexion du locataire.
 * Pour chaque favori, verifie si le logement est redevenu
 * disponible et affiche une alerte le cas echeant.
 *
 * Exemple d'alerte :
 *   [ALERTE FAVORI] "Studio Bastos" est maintenant DISPONIBLE !
 *   Reservez-le avant qu'il ne parte ! (ID: 5)
 *
 * @param idLocataire ID du locataire connecte.
 */
void verifierAlertesFavoris(int idLocataire) {
    Favori favoris[MAX_FAVORIS];
    int    nb = 0;
    int    i, alertes = 0;

    chargerFavoris(favoris, &nb);

    for (i = 0; i < nb; i++) {
        if (favoris[i].idLocataire != idLocataire) continue;

        char           titre[TAILLE_TITRE];
        StatutLogement statut;

        if (!trouverLogement(favoris[i].idLogement, titre, &statut))
            continue;

        /* Alerte uniquement si le logement est disponible */
        if (statut == STATUT_DISPONIBLE) {
            if (alertes == 0) {
                printf("\n========================================\n");
                printf("         ALERTES FAVORIS\n");
                printf("========================================\n");
            }
            printf("[ALERTE] \"%s\" est maintenant DISPONIBLE !\n", titre);
            printf("         Reservez-le avant qu'il ne parte !\n");
            printf("         (ID logement : %d)\n\n", favoris[i].idLogement);
            alertes++;
        }
    }

    if (alertes > 0) {
        printf("========================================\n");
        printf("%d favori(s) disponible(s) !\n", alertes);
        printf("Allez dans vos favoris pour les reserver.\n");
        printf("========================================\n");
    }
}

/**
 * @brief Affiche le menu de gestion des favoris.
 *
 * Sous-menu accessible depuis le menu locataire.
 * Propose : voir, ajouter et supprimer des favoris.
 */
void menuFavoris() {
    int choix;
    do {
        printf("\n=== MES FAVORIS ===\n");
        printf("1. Voir mes favoris\n");
        printf("2. Ajouter un favori\n");
        printf("3. Supprimer un favori\n");
        printf("0. Retour\n");
        printf("Choix : ");
        choix = lireEntier();

        if (choix == -99) {
            printf("[ERREUR] Saisie invalide.\n");
            continue;
        }

        switch (choix) {
            case 1: voirMesFavoris();  break;
            case 2: ajouterFavori();   break;
            case 3: supprimerFavori(); break;
            case 0: break;
            default:
                printf("[ERREUR] Choix invalide.\n");
        }
    } while (choix != 0);
}
