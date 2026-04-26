/* ============================================================
 * @file    notation.c
 * @brief   Module de notation des logements V2.0.
 *
 * Un locataire peut noter un logement de 1 a 5 etoiles
 * et laisser un commentaire. La note moyenne s'affiche
 * sur chaque annonce consultee.
 *
 * @version 2.0
 * @author  GUIASSU
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structures.h"
#include "auth.h"
#include "logement.h"
#include "notation.h"

#ifndef _WIN32
#include <sys/stat.h>
#endif

#define MAX_NOTATIONS 500

/* ============================================================
 * UTILITAIRES
 * ============================================================ */

static void creerDossier() {
#ifdef _WIN32
    system("if not exist data mkdir data");
#else
    mkdir("data", 0755);
#endif
}

static void lireLigne(const char *invite, char *buffer, int taille) {
    printf("%s", invite);
    if (fgets(buffer, taille, stdin))
        buffer[strcspn(buffer, "\n\r")] = '\0';
}

static int lireEntier() {
    char buf[32]; int val;
    lireLigne("", buf, sizeof(buf));
    if (sscanf(buf, "%d", &val) != 1) return -99;
    return val;
}

/* ============================================================
 * PERSISTANCE
 * ============================================================ */

/**
 * @brief Charge toutes les notations depuis le fichier.
 * @param notations Tableau de sortie.
 * @param nb        Nombre de notations lues.
 */
static void chargerNotations(Notation *notations, int *nb) {
    *nb = 0;
    FILE *f = fopen(FICHIER_NOTATIONS, "r");
    if (!f) return;

    Notation n;
    while (fscanf(f, "%d|%d|%d|%d|%199[^\n]\n",
                  &n.idNotation, &n.idLocataire,
                  &n.idLogement, &n.note,
                  n.commentaire) == 5) {
        notations[(*nb)++] = n;
        if (*nb >= MAX_NOTATIONS) break;
    }
    fclose(f);
}

/**
 * @brief Sauvegarde toutes les notations dans le fichier.
 */
static void sauvegarderNotations(Notation *notations, int nb) {
    creerDossier();
    FILE *f = fopen(FICHIER_NOTATIONS, "w");
    if (!f) { printf("[ERREUR] Impossible de sauvegarder les notations.\n"); return; }
    int i;
    for (i = 0; i < nb; i++)
        fprintf(f, "%d|%d|%d|%d|%s\n",
                notations[i].idNotation, notations[i].idLocataire,
                notations[i].idLogement, notations[i].note,
                notations[i].commentaire);
    fclose(f);
}

/**
 * @brief Genere un ID unique pour une notation.
 */
static int genererIdNotation(Notation *notations, int nb) {
    int maxId = 0, i;
    for (i = 0; i < nb; i++)
        if (notations[i].idNotation > maxId) maxId = notations[i].idNotation;
    return maxId + 1;
}

/* ============================================================
 * FONCTIONS PRINCIPALES
 * ============================================================ */

/**
 * @brief Calcule la note moyenne d'un logement.
 *
 * Parcourt toutes les notations et calcule la moyenne
 * des notes pour le logement demande.
 *
 * @param idLogement ID du logement.
 * @return Moyenne des notes, 0.0 si aucun avis.
 */
float calculerMoyenne(int idLogement) {
    Notation notations[MAX_NOTATIONS];
    int nb = 0, i, total = 0, count = 0;

    chargerNotations(notations, &nb);
    for (i = 0; i < nb; i++) {
        if (notations[i].idLogement == idLogement) {
            total += notations[i].note;
            count++;
        }
    }
    return (count > 0) ? (float)total / count : 0.0f;
}

/**
 * @brief Affiche les avis et la note moyenne d'un logement.
 *
 * Affiche le nombre d'etoiles sous forme visuelle :
 *   Note : *** (3.5/5) — 4 avis
 *
 * @param idLogement ID du logement.
 */
void afficherAvis(int idLogement) {
    Notation notations[MAX_NOTATIONS];
    int nb = 0, i, count = 0;

    chargerNotations(notations, &nb);

    /* Compter les avis pour ce logement */
    for (i = 0; i < nb; i++)
        if (notations[i].idLogement == idLogement) count++;

    if (count == 0) {
        printf("Note    : Aucun avis pour le moment.\n");
        return;
    }

    float moyenne = calculerMoyenne(idLogement);

    /* Afficher les etoiles visuellement */
    printf("Note    : ");
    int etoiles = (int)(moyenne + 0.5f); /* arrondi */
    int k;
    for (k = 0; k < etoiles;     k++) printf("*");
    for (k = etoiles; k < 5;     k++) printf("-");
    printf(" (%.1f/5) - %d avis\n", moyenne, count);

    /* Afficher les 3 derniers commentaires */
    printf("Avis    :\n");
    int affiches = 0;
    for (i = nb - 1; i >= 0 && affiches < 3; i--) {
        if (notations[i].idLogement == idLogement) {
            /* Trouver le prenom du locataire */
            char prenom[TAILLE_NOM] = "Anonyme";
            int j;
            for (j = 0; j < nbUtilisateurs; j++) {
                if (listeUtilisateurs[j].id == notations[i].idLocataire) {
                    strncpy(prenom, listeUtilisateurs[j].prenom, TAILLE_NOM-1);
                    break;
                }
            }
            printf("  - %s (%d/5) : %s\n",
                   prenom, notations[i].note, notations[i].commentaire);
            affiches++;
        }
    }
}

/**
 * @brief Permet au locataire de noter un logement.
 *
 * Regles :
 *   - Un locataire ne peut noter un logement qu'une seule fois.
 *   - La note doit etre entre 1 et 5.
 *   - Le commentaire est obligatoire.
 *
 * Affiche la liste des logements disponibles pour choisir.
 */
void noterLogement() {
    Notation notations[MAX_NOTATIONS];
    int nb = 0, i;
    int idLogement, note;
    char commentaire[TAILLE_COMMENTAIRE];

    printf("\n[NOTER UN LOGEMENT]\n");
    printf("------------------------------\n");

    if (nbLogements == 0) {
        printf("Aucun logement enregistre.\n");
        return;
    }

    /* Afficher tous les logements */
    printf("Logements disponibles :\n");
    for (i = 0; i < nbLogements; i++) {
        printf("  ID: %-3d | %-20s | %s\n",
               listeLogements[i].id,
               listeLogements[i].titre,
               listeLogements[i].ville);
    }

    printf("\nID du logement a noter : ");
    idLogement = lireEntier();
    if (idLogement == -99) {
        printf("[ERREUR] Saisie invalide.\n");
        return;
    }

    /* Verifier que le logement existe */
    int existe = 0;
    for (i = 0; i < nbLogements; i++)
        if (listeLogements[i].id == idLogement) { existe = 1; break; }
    if (!existe) {
        printf("[ERREUR] Logement ID %d introuvable.\n", idLogement);
        return;
    }

    /* Verifier qu'il n'a pas deja note ce logement */
    chargerNotations(notations, &nb);
    for (i = 0; i < nb; i++) {
        if (notations[i].idLocataire == sessionCourante.utilisateur.id &&
            notations[i].idLogement  == idLogement) {
            printf("[INFO] Vous avez deja note ce logement.\n");
            return;
        }
    }

    /* Saisir la note */
    do {
        printf("Note (1 a 5 etoiles) : ");
        note = lireEntier();
        if (note < 1 || note > 5)
            printf("[ERREUR] La note doit etre entre 1 et 5.\n");
    } while (note < 1 || note > 5);

    /* Saisir le commentaire */
    lireLigne("Votre commentaire : ", commentaire, TAILLE_COMMENTAIRE);
    if (strlen(commentaire) == 0)
        strcpy(commentaire, "Aucun commentaire.");

    /* Creer et sauvegarder la notation */
    Notation nouvelle;
    nouvelle.idNotation  = genererIdNotation(notations, nb);
    nouvelle.idLocataire = sessionCourante.utilisateur.id;
    nouvelle.idLogement  = idLogement;
    nouvelle.note        = note;
    strncpy(nouvelle.commentaire, commentaire, TAILLE_COMMENTAIRE - 1);
    nouvelle.commentaire[TAILLE_COMMENTAIRE - 1] = '\0';

    notations[nb++] = nouvelle;
    sauvegarderNotations(notations, nb);

    /* Afficher les etoiles */
    printf("[OK] Merci pour votre avis ! Vous avez donne ");
    for (i = 0; i < note; i++) printf("*");
    printf(" (%d/5)\n", note);
}

/**
 * @brief Menu de gestion des notations pour le locataire.
 */
void menuNotation() {
    int choix;
    do {
        printf("\n=== NOTATIONS ET AVIS ===\n");
        printf("1. Noter un logement\n");
        printf("2. Voir les avis d'un logement\n");
        printf("0. Retour\n");
        printf("Choix : ");
        choix = lireEntier();

        if (choix == -99) { printf("[ERREUR] Saisie invalide.\n"); continue; }

        switch (choix) {
            case 1: noterLogement(); break;
            case 2: {
                printf("ID du logement : ");
                int id = lireEntier();
                if (id != -99) afficherAvis(id);
                break;
            }
            case 0: break;
            default: printf("[ERREUR] Choix invalide.\n");
        }
    } while (choix != 0);
}
