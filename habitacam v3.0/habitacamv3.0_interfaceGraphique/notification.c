/* ============================================================
 * @file    notification.c
 * @brief   Module de notifications V2.0.
 *
 * Les notifications sont des messages automatiques envoyes
 * aux utilisateurs. Elles s'affichent a la connexion et
 * sont marquees comme lues apres affichage.
 *
 * @version 2.0
 * @author  WANDJOU
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structures.h"
#include "auth.h"
#include "logement.h"
#include "matching.h"
#include "notification.h"

#ifndef _WIN32
#include <sys/stat.h>
#endif

#define MAX_NOTIFICATIONS 500

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

/* ============================================================
 * PERSISTANCE
 * ============================================================ */

/**
 * @brief Charge toutes les notifications depuis le fichier.
 */
static void chargerNotifications(Notification *notifs, int *nb) {
    *nb = 0;
    FILE *f = fopen(FICHIER_NOTIFICATIONS, "r");
    if (!f) return;

    Notification n;
    while (fscanf(f, "%d|%d|%199[^|]|%d|%19[^\n]\n",
                  &n.id, &n.idDestinataire,
                  n.message, &n.lu, n.date) == 5) {
        notifs[(*nb)++] = n;
        if (*nb >= MAX_NOTIFICATIONS) break;
    }
    fclose(f);
}

/**
 * @brief Sauvegarde toutes les notifications dans le fichier.
 */
static void sauvegarderNotifications(Notification *notifs, int nb) {
    creerDossier();
    FILE *f = fopen(FICHIER_NOTIFICATIONS, "w");
    if (!f) return;
    int i;
    for (i = 0; i < nb; i++)
        fprintf(f, "%d|%d|%s|%d|%s\n",
                notifs[i].id, notifs[i].idDestinataire,
                notifs[i].message, notifs[i].lu, notifs[i].date);
    fclose(f);
}

/**
 * @brief Genere un ID unique pour une notification.
 */
static int genererIdNotification(Notification *notifs, int nb) {
    int maxId = 0, i;
    for (i = 0; i < nb; i++)
        if (notifs[i].id > maxId) maxId = notifs[i].id;
    return maxId + 1;
}

/* ============================================================
 * FONCTIONS PRINCIPALES
 * ============================================================ */

/**
 * @brief Cree et sauvegarde une notification.
 *
 * Ajoute une nouvelle notification dans data/notifications.txt
 * avec le statut "non lu" (lu = 0).
 *
 * @param idDestinataire ID de l'utilisateur a notifier.
 * @param message        Contenu du message.
 */
void envoyerNotification(int idDestinataire, const char *message) {
    Notification notifs[MAX_NOTIFICATIONS];
    int nb = 0;

    chargerNotifications(notifs, &nb);

    Notification nouvelle;
    nouvelle.id              = genererIdNotification(notifs, nb);
    nouvelle.idDestinataire  = idDestinataire;
    nouvelle.lu              = 0;
    strcpy(nouvelle.date,    "29/03/2026");
    strncpy(nouvelle.message, message, TAILLE_MESSAGE - 1);
    nouvelle.message[TAILLE_MESSAGE - 1] = '\0';

    notifs[nb++] = nouvelle;
    sauvegarderNotifications(notifs, nb);
}

/**
 * @brief Affiche les notifications non lues a la connexion.
 *
 * Parcourt data/notifications.txt, affiche les messages
 * non lus pour cet utilisateur et les marque comme lus.
 *
 * Exemple d'affichage :
 *   ========================================
 *            NOTIFICATIONS (2)
 *   ========================================
 *   [29/03/2026] Votre logement "Villa Bastos" a ete reserve !
 *   [29/03/2026] Nouveau logement disponible a Yaounde.
 *   ========================================
 *
 * @param idUtilisateur ID de l'utilisateur connecte.
 */
void afficherNotifications(int idUtilisateur) {
    Notification notifs[MAX_NOTIFICATIONS];
    int nb = 0, i, count = 0;

    chargerNotifications(notifs, &nb);

    /* Compter les non lues */
    for (i = 0; i < nb; i++)
        if (notifs[i].idDestinataire == idUtilisateur && !notifs[i].lu)
            count++;

    if (count == 0) return; /* Pas de nouvelles notifications */

    printf("\n========================================\n");
    printf("        NOTIFICATIONS (%d)\n", count);
    printf("========================================\n");

    /* Afficher et marquer comme lues */
    for (i = 0; i < nb; i++) {
        if (notifs[i].idDestinataire == idUtilisateur && !notifs[i].lu) {
            printf("[%s] %s\n", notifs[i].date, notifs[i].message);
            notifs[i].lu = 1; /* Marquer comme lu */
        }
    }
    printf("========================================\n");

    /* Sauvegarder avec les nouvelles valeurs "lu" */
    sauvegarderNotifications(notifs, nb);
}

/**
 * @brief Notifie le bailleur qu'un de ses logements a ete reserve.
 *
 * Appelee depuis locataire.c quand une reservation est creee.
 * Cree une notification dans data/notifications.txt pour
 * que le bailleur la voie a sa prochaine connexion.
 *
 * @param idBailleur      ID du bailleur a notifier.
 * @param titreLogement   Titre du logement reserve.
 * @param prenomLocataire Prenom du locataire qui a reserve.
 */
void notifierReservation(int idBailleur,
                          const char *titreLogement,
                          const char *prenomLocataire) {
    char message[TAILLE_MESSAGE];
    snprintf(message, TAILLE_MESSAGE,
             "Votre logement \"%s\" a ete reserve par %s.",
             titreLogement, prenomLocataire);
    envoyerNotification(idBailleur, message);
}

/**
 * @brief Notifie le locataire s'il y a de nouveaux logements
 * dans sa ville de recherche depuis sa derniere connexion.
 *
 * Utilise le profil de recherche sauvegarde par le module
 * matching pour connaitre la ville preferee du locataire.
 * Compte les logements disponibles dans cette ville et envoie
 * une notification si au moins un est trouve.
 *
 * @param idLocataire ID du locataire connecte.
 */
void notifierNouveauxLogements(int idLocataire) {
    ProfilRecherche profil;
    int i, count = 0;

    /* Charger le profil de recherche */
    if (!chargerProfil(idLocataire, &profil)) return;
    if (strlen(profil.ville) == 0) return;

    /* Compter les logements disponibles dans cette ville */
    for (i = 0; i < nbLogements; i++) {
        if (listeLogements[i].statut == STATUT_DISPONIBLE &&
            strstr(listeLogements[i].ville, profil.ville) != NULL) {
            count++;
        }
    }

    if (count > 0) {
        char message[TAILLE_MESSAGE];
        snprintf(message, TAILLE_MESSAGE,
                 "%d logement(s) disponible(s) a %s. "
                 "Faites une recherche pour les voir !",
                 count, profil.ville);
        envoyerNotification(idLocataire, message);
    }
}
