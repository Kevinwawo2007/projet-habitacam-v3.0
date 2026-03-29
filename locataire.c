/* ============================================================
 * @file    locataire.c
 * @brief   Module Locataire de HabitatCam.
 *
 * Fournit le menu et les actions pour un locataire connecte :
 * rechercher un logement et effectuer une reservation.
 *
 * @version 1.0
 * @author  NANDA
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structures.h"
#include "auth.h"
#include "logement.h"
#include "locataire.h"
#include "matching.h"
#include "favoris.h"

/* -- Variables globales des reservations --------------------- */

/** @brief Tableau de toutes les reservations en memoire. */
Reservation listeReservations[MAX_RESERVATIONS];

/** @brief Nombre de reservations actuellement enregistrees. */
int nbReservations = 0;

/**
 * @brief Vide le tampon du clavier apres un scanf().
 */
static void viderBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
/**
 * @brief  Lit un entier depuis le clavier de facon securisee.
 *
 * Si l'utilisateur entre une lettre ou un caractere invalide,
 * vide le buffer et retourne -99 pour signaler l'erreur.
 *
 * @return L'entier saisi, ou -99 si la saisie est invalide.
 */
static int saisirEntier() {
    int val;
    if (scanf("%d", &val) != 1) {
        int c; while ((c = getchar()) != '\n' && c != EOF);
        return -99; /* Code d'erreur : saisie invalide */
    }
    int c; while ((c = getchar()) != '\n' && c != EOF);
    return val;
}


/**
 * @brief Charge les reservations depuis le fichier texte.
 *
 * Lit data/reservations.txt et remplit listeReservations[].
 * Si le fichier est absent, le tableau reste vide.
 */
void chargerReservations() {
    FILE *f = fopen(FICHIER_RESERVATIONS, "r");
    if (f == NULL) return;

    nbReservations = 0;
    Reservation r;
    while (fscanf(f, "%d|%d|%d|%19[^|]|%d\n",
                  &r.id, &r.idLocataire, &r.idLogement,
                  r.dateReservation, (int*)&r.statut) == 5) {
        listeReservations[nbReservations++] = r;
        if (nbReservations >= MAX_RESERVATIONS) break;
    }
    fclose(f);
}

/**
 * @brief Sauvegarde toutes les reservations dans le fichier texte.
 *
 * @note A appeler apres chaque nouvelle reservation.
 */
void sauvegarderReservations() {
    system("if not exist data mkdir data");
    FILE *f = fopen(FICHIER_RESERVATIONS, "w");
    if (f == NULL) {
        printf("[ERREUR] Impossible d'ouvrir le fichier reservations.\n");
        return;
    }
    for (int i = 0; i < nbReservations; i++) {
        Reservation *r = &listeReservations[i];
        fprintf(f, "%d|%d|%d|%s|%d\n",
                r->id, r->idLocataire, r->idLogement,
                r->dateReservation, (int)r->statut);
    }
    fclose(f);
}

/**
 * @brief Genere un ID unique pour une nouvelle reservation.
 * @return Le plus grand ID existant + 1.
 */
static int genererIdReservation() {
    int maxId = 0;
    for (int i = 0; i < nbReservations; i++) {
        if (listeReservations[i].id > maxId)
            maxId = listeReservations[i].id;
    }
    return maxId + 1;
}

/**
 * @brief Permet au locataire de reserver un logement disponible.
 *
 * Affiche les logements disponibles, demande l'ID du logement
 * choisi, cree une reservation et change le statut du logement
 * en STATUT_RESERVE.
 *
 * @warning Accessible uniquement au role ROLE_LOCATAIRE.
 * @warning Le locataire ne peut pas reserver un logement deja reserve.
 */
static void reserverLogement() {
    afficherLogements();

    int id;
    printf("\nEntrez l'ID du logement a reserver : ");
    scanf("%d", &id);
    viderBuffer();

    /* Chercher le logement */
    for (int i = 0; i < nbLogements; i++) {
        if (listeLogements[i].id == id) {

            if (listeLogements[i].statut != STATUT_DISPONIBLE) {
                printf("[ERREUR] Ce logement n'est pas disponible.\n");
                return;
            }

            /* Creer la reservation */
            Reservation r;
            r.id          = genererIdReservation();
            r.idLocataire = sessionCourante.utilisateur.id;
            r.idLogement  = id;
            r.statut      = RES_EN_ATTENTE;
            strcpy(r.dateReservation, "01/01/2025"); /* Date fixe en V1.0 */

            listeReservations[nbReservations++] = r;
            sauvegarderReservations();

            /* Changer le statut du logement */
            listeLogements[i].statut = STATUT_RESERVE;
            sauvegarderLogements();

            printf("[OK] Logement \"%s\" reserve avec succes !\n",
                   listeLogements[i].titre);
            printf("     Votre reservation ID : %d\n", r.id);
            return;
        }
    }
    printf("[ERREUR] Aucun logement avec l'ID %d.\n", id);
}

/**
 * @brief Affiche les reservations du locataire connecte.
 *
 * Parcourt listeReservations[] et affiche uniquement celles
 * appartenant a l'utilisateur en session.
 */
static void voirMesReservations() {
    int idLocataire = sessionCourante.utilisateur.id;
    int trouve = 0;

    printf("\n--- MES RESERVATIONS ---\n");
    for (int i = 0; i < nbReservations; i++) {
        Reservation *r = &listeReservations[i];
        if (r->idLocataire == idLocataire) {

            /* Trouver le titre du logement */
            char titre[TAILLE_TITRE] = "Inconnu";
            for (int j = 0; j < nbLogements; j++) {
                if (listeLogements[j].id == r->idLogement) {
                    strcpy(titre, listeLogements[j].titre);
                    break;
                }
            }

            printf("Reservation %d | Logement : %s | Date : %s | ",
                   r->id, titre, r->dateReservation);

            switch (r->statut) {
                case RES_EN_ATTENTE: printf("En attente\n");  break;
                case RES_CONFIRMEE:  printf("Confirmee\n");   break;
                case RES_ANNULEE:    printf("Annulee\n");     break;
            }
            trouve = 1;
        }
    }
    if (!trouve)
        printf("Vous n'avez aucune reservation.\n");
}

/**
 * @brief Point d'entree du module locataire.
 *
 * Affiche le menu locataire et traite les choix jusqu'a
 * la deconnexion (choix 0).
 *
 * @warning Accessible uniquement au role ROLE_LOCATAIRE.
 */
void menuLocataire() {
    if (sessionCourante.utilisateur.role != ROLE_LOCATAIRE) {
        printf("[ACCES REFUSE] Section reservee aux locataires.\n");
        return;
    }

    int choix;
    do {
        printf("\n=== MENU LOCATAIRE ===\n");
        printf("Connecte : %s\n", sessionCourante.utilisateur.prenom);
        printf("1. Voir les logements disponibles\n");
        printf("2. Recherche avancee (avec recommandations)\n");
        printf("3. Reserver un logement\n");
        printf("4. Voir mes reservations\n");
        printf("5. Mes favoris\n");
        printf("6. Personnalisation du profil\n");
        printf("0. Se deconnecter\n");
        printf("Choix : ");
        choix = saisirEntier();

        if (choix == -99) {
            printf("[ERREUR] Entrez un chiffre valide (0 a 6).\n");
            continue;
        }

        switch (choix) {
            case 1: afficherLogements();      break;
            case 2: rechercheAvancee();        break;
            case 3: reserverLogement();       break;
            case 4: voirMesReservations();    break;
            case 5: menuFavoris();            break;
            case 6: menuPersonnalisation();   break;
            case 0: deconnecterUtilisateur(); break;
            default:
                printf("[ERREUR] Le choix %d n'existe pas.\n", choix);
                printf("         Veuillez choisir entre 0 et 5.\n");
        }
    } while (choix != 0 && sessionCourante.connecte);
}
