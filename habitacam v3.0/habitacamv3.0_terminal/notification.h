/* ============================================================
 * @file    notification.h
 * @brief   Module de notifications V2.0.
 *
 * Envoie des alertes automatiques a la connexion :
 *   - Locataire : nouveaux logements dans sa ville recherchee
 *   - Bailleur  : nouvelles reservations recues
 *
 * Fichier de donnees :
 *   data/notifications.txt
 *   Format : id|idDestinataire|message|lu|date
 *
 * @version 2.0
 * @author  WANDJOU
 * ============================================================ */

#ifndef NOTIFICATION_H_INCLUDED
#define NOTIFICATION_H_INCLUDED

#include "structures.h"

#define FICHIER_NOTIFICATIONS "data/notifications.txt"
#define TAILLE_MESSAGE        200

/**
 * @brief Represente une notification pour un utilisateur.
 */
typedef struct {
    int  id;                        /**< ID unique.                */
    int  idDestinataire;            /**< ID du destinataire.       */
    char message[TAILLE_MESSAGE];   /**< Contenu de la notification*/
    int  lu;                        /**< 0 = non lu, 1 = lu.       */
    char date[20];                  /**< Date d'envoi.             */
} Notification;

/**
 * @brief Cree et sauvegarde une notification pour un utilisateur.
 * @param idDestinataire ID de l'utilisateur a notifier.
 * @param message        Contenu du message.
 */
void envoyerNotification(int idDestinataire, const char *message);

/**
 * @brief Affiche les notifications non lues a la connexion.
 * Marque automatiquement les notifications comme lues.
 * @param idUtilisateur ID de l'utilisateur connecte.
 */
void afficherNotifications(int idUtilisateur);

/**
 * @brief Cree une notification pour le bailleur quand son
 * logement est reserve par un locataire.
 * @param idBailleur  ID du bailleur a notifier.
 * @param titreLogement Titre du logement reserve.
 * @param prenomLocataire Prenom du locataire.
 */
void notifierReservation(int idBailleur,
                          const char *titreLogement,
                          const char *prenomLocataire);

/**
 * @brief Verifie les nouveaux logements dans la ville du locataire
 * et envoie une notification si necessaire.
 * @param idLocataire ID du locataire connecte.
 */
void notifierNouveauxLogements(int idLocataire);

#endif /* NOTIFICATION_H_INCLUDED */
