/* ============================================================
 * @file    administrateur.c
 * @brief   Module Administrateur de HabitatCam.
 *
 * Fournit les outils de supervision et de gestion complete
 * de la plateforme : utilisateurs, logements et statistiques.
 * Accessible uniquement aux comptes de role ROLE_ADMINISTRATEUR.
 *
 * @version  1.0
 * @date    2024-2025
 * @author  SOUOPGUI
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Structure.h"
#include "auth.h"
#include "administrateur.h"

/* ── Declarations extern (definies dans logement.c) ─────── */
extern Logement    listeLogements[MAX_LOGEMENTS];
extern int         nbLogements;
extern Reservation listeReservations[MAX_RESERVATIONS];
extern int         nbReservations;
extern void        sauvegarderLogements();
extern void        sauvegarderReservations();


/* ============================================================
 * FONCTIONS UTILITAIRES INTERNES
 * ============================================================ */

/**
 * @brief Vide le tampon du clavier.
 *
 * A appeler systematiquement apres chaque scanf() pour eviter
 * les residus dans stdin qui provoqueraient des bugs de saisie.
 */
static void viderBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * @brief Affiche une ligne de separation visuelle dans le terminal.
 */
static void separateur() {
    printf("---------------------------------------------\n");
}

/**
 * @brief Convertit un role (enum) en chaine de caracteres lisible.
 *
 * @param r  Le role a convertir (valeur de l'enum Role).
 * @return   Chaine correspondante : "Locataire", "Bailleur"
 *           ou "Administrateur". Retourne "Inconnu" si invalide.
 */
static const char* roleEnTexte(Role r) {
    switch (r) {
        case ROLE_LOCATAIRE:      return "Locataire";
        case ROLE_BAILLEUR:       return "Bailleur";
        case ROLE_ADMINISTRATEUR: return "Administrateur";
        default:                  return "Inconnu";
    }
}

/**
 * @brief Convertit un statut de logement (enum) en chaine lisible.
 *
 * @param s  Le statut a convertir (valeur de l'enum StatutLogement).
 * @return   "Disponible", "Reserve" ou "Indisponible".
 *           Retourne "Inconnu" si la valeur est invalide.
 */
static const char* statutLogementEnTexte(StatutLogement s) {
    switch (s) {
        case STATUT_DISPONIBLE:   return "Disponible";
        case STATUT_RESERVE:      return "Reserve";
        case STATUT_INDISPONIBLE: return "Indisponible";
        default:                  return "Inconnu";
    }
}

/**
 * @brief Verifie que la session courante est bien celle d'un administrateur.
 *
 * Controle que sessionCourante.connecte == 1 et que le role est
 * ROLE_ADMINISTRATEUR. Affiche un message de refus si ce n'est pas le cas.
 *
 * @return 1 si l'utilisateur courant est administrateur, 0 sinon.
 * @warning A appeler en debut de chaque fonction sensible de ce module.
 */
static int verifierAdmin() {
    if (!sessionCourante.connecte ||
        sessionCourante.utilisateur.role != ROLE_ADMINISTRATEUR) {
        printf("[ACCES REFUSE] Section reservee a l'administrateur.\n");
        return 0;
    }
    return 1;
}


/* ============================================================
 * GESTION DES UTILISATEURS
 * ============================================================ */

/**
 * @brief Affiche la liste complete de tous les utilisateurs inscrits.
 *
 * Presente un tableau avec l'ID, le nom, le prenom, l'email,
 * le role et le statut (Actif / DESACTIVE) de chaque compte.
 *
 * @note Accessible uniquement au role Administrateur.
 */
void adminVoirUtilisateurs() {
    if (!verifierAdmin()) return;

    printf("\n[LISTE DES UTILISATEURS]\n");

    if (nbUtilisateurs == 0) { printf("Aucun utilisateur enregistre.\n"); return; }

    separateur();
    printf("%-4s %-15s %-15s %-25s %-14s %-6s\n", "ID", "Nom", "Prenom", "Email", "Role", "Statut");
    separateur();
    for (int i = 0; i < nbUtilisateurs; i++) {
        Utilisateur *u = &listeUtilisateurs[i];
        printf("%-4d %-15s %-15s %-25s %-14s %s\n",
               u->id, u->nom, u->prenom, u->email,
               roleEnTexte(u->role), u->actif ? "Actif" : "DESACTIVE");
    }
    separateur();
    printf("Total : %d utilisateur(s)\n", nbUtilisateurs);
}

/**
 * @brief Active ou desactive le compte d'un utilisateur.
 *
 * Inverse la valeur du champ 'actif' de l'utilisateur cible.
 * Un compte desactive ne peut plus se connecter.
 * La modification est sauvegardee immediatement.
 *
 * @note Accessible uniquement au role Administrateur.
 * @warning Ne peut pas desactiver son propre compte ni celui
 *          d'un autre administrateur.
 */
void adminToggleCompte() {
    if (!verifierAdmin()) return;

    adminVoirUtilisateurs();
    int id;
    printf("\nID de l'utilisateur a activer/desactiver : ");
    scanf("%d", &id); viderBuffer();

    if (id == sessionCourante.utilisateur.id) {
        printf("[ERREUR] Impossible de desactiver votre propre compte.\n"); return;
    }
    for (int i = 0; i < nbUtilisateurs; i++) {
        if (listeUtilisateurs[i].id == id) {
            if (listeUtilisateurs[i].role == ROLE_ADMINISTRATEUR) {
                printf("[ERREUR] Impossible de modifier un compte administrateur.\n"); return;
            }
            listeUtilisateurs[i].actif = !listeUtilisateurs[i].actif;
            sauvegarderUtilisateurs();
            printf("[OK] Compte de %s : %s\n", listeUtilisateurs[i].prenom,
                   listeUtilisateurs[i].actif ? "ACTIF" : "DESACTIVE");
            return;
        }
    }
    printf("[ERREUR] Aucun utilisateur avec l'ID %d.\n", id);
}

/**
 * @brief Supprime definitivement un utilisateur de la plateforme.
 *
 * Decale le tableau listeUtilisateurs pour effacer l'entree,
 * decremente nbUtilisateurs et sauvegarde immediatement.
 *
 * @note Accessible uniquement au role Administrateur.
 * @warning Action irreversible. Ne peut pas supprimer son propre
 *          compte ni un autre compte administrateur.
 */
void adminSupprimerUtilisateur() {
    if (!verifierAdmin()) return;

    adminVoirUtilisateurs();
    int id;
    printf("\nID de l'utilisateur a supprimer : ");
    scanf("%d", &id); viderBuffer();

    if (id == sessionCourante.utilisateur.id) {
        printf("[ERREUR] Impossible de supprimer votre propre compte.\n"); return;
    }
    for (int i = 0; i < nbUtilisateurs; i++) {
        if (listeUtilisateurs[i].id == id) {
            if (listeUtilisateurs[i].role == ROLE_ADMINISTRATEUR) {
                printf("[ERREUR] Impossible de supprimer un administrateur.\n"); return;
            }
            char nom[TAILLE_NOM];
            strcpy(nom, listeUtilisateurs[i].prenom);
            for (int j = i; j < nbUtilisateurs - 1; j++)
                listeUtilisateurs[j] = listeUtilisateurs[j + 1];
            nbUtilisateurs--;
            sauvegarderUtilisateurs();
            printf("[OK] Compte de %s (ID: %d) supprime.\n", nom, id);
            return;
        }
    }
    printf("[ERREUR] Aucun utilisateur avec l'ID %d.\n", id);
}


/* ============================================================
 * GESTION DES LOGEMENTS
 * ============================================================ */

/**
 * @brief Affiche la liste complete de tous les logements de la plateforme.
 *
 * Presente l'ID, le titre, le type, la ville, la superficie,
 * le prix mensuel et le statut de chaque logement enregistre.
 *
 * @note Accessible uniquement au role Administrateur.
 */
void adminVoirLogements() {
    if (!verifierAdmin()) return;

    printf("\n[LISTE DES LOGEMENTS]\n");
    if (nbLogements == 0) { printf("Aucun logement enregistre.\n"); return; }

    separateur();
    printf("%-4s %-20s %-12s %-15s %-10s %-12s %-12s\n",
           "ID", "Titre", "Type", "Ville", "Surf(m2)", "Prix(FCFA)", "Statut");
    separateur();
    for (int i = 0; i < nbLogements; i++) {
        Logement *l = &listeLogements[i];
        printf("%-4d %-20s %-12s %-15s %-10.1f %-12.0f %s\n",
               l->id, l->titre, l->type, l->ville,
               l->superficie, l->prixMensuel,
               statutLogementEnTexte(l->statut));
    }
    separateur();
    printf("Total : %d logement(s)\n", nbLogements);
}

/**
 * @brief Supprime definitivement un logement de la plateforme.
 *
 * Decale le tableau listeLogements, decremente nbLogements
 * et sauvegarde immediatement dans le fichier.
 *
 * @note Accessible uniquement au role Administrateur.
 * @warning Action irreversible. Les reservations liees au logement
 *          supprime ne sont pas automatiquement annulees en V1.0.
 */
void adminSupprimerLogement() {
    if (!verifierAdmin()) return;

    adminVoirLogements();
    int id;
    printf("\nID du logement a supprimer : ");
    scanf("%d", &id); viderBuffer();

    for (int i = 0; i < nbLogements; i++) {
        if (listeLogements[i].id == id) {
            char titre[TAILLE_TITRE];
            strcpy(titre, listeLogements[i].titre);
            for (int j = i; j < nbLogements - 1; j++)
                listeLogements[j] = listeLogements[j + 1];
            nbLogements--;
            sauvegarderLogements();
            printf("[OK] Logement \"%s\" (ID: %d) supprime.\n", titre, id);
            return;
        }
    }
    printf("[ERREUR] Aucun logement avec l'ID %d.\n", id);
}


/* ============================================================
 * STATISTIQUES
 * ============================================================ */

/**
 * @brief Affiche le tableau de bord general de la plateforme.
 *
 * Calcule et affiche en temps reel :
 *  - Repartition des utilisateurs par role (locataires, bailleurs, admins)
 *  - Nombre de comptes actifs et desactives
 *  - Repartition des logements par statut (disponible, reserve, indisponible)
 *  - Nombre total de reservations enregistrees
 *
 * @note Accessible uniquement au role Administrateur.
 * @note Les donnees sont calculees depuis les tableaux en memoire,
 *       donc toujours a jour sans relecture du fichier.
 */
void adminStatistiques() {
    if (!verifierAdmin()) return;

    int nbLoc=0, nbBaill=0, nbAdm=0, nbActifs=0, nbInactifs=0;
    int nbDispo=0, nbRes=0, nbIndispo=0;

    for (int i = 0; i < nbUtilisateurs; i++) {
        switch (listeUtilisateurs[i].role) {
            case ROLE_LOCATAIRE:      nbLoc++;   break;
            case ROLE_BAILLEUR:       nbBaill++; break;
            case ROLE_ADMINISTRATEUR: nbAdm++;   break;
        }
        listeUtilisateurs[i].actif ? nbActifs++ : nbInactifs++;
    }
    for (int i = 0; i < nbLogements; i++) {
        switch (listeLogements[i].statut) {
            case STATUT_DISPONIBLE:   nbDispo++;   break;
            case STATUT_RESERVE:      nbRes++;     break;
            case STATUT_INDISPONIBLE: nbIndispo++; break;
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
    separateur();
}


/* ============================================================
 * MENU PRINCIPAL ADMINISTRATEUR
 * ============================================================ */

/**
 * @brief Point d'entree du module administrateur.
 *
 * Affiche le panneau d'administration et traite les choix
 * de l'administrateur connecte. La boucle tourne jusqu'a
 * ce que l'administrateur choisisse de se deconnecter (choix 0)
 * ou que la session soit fermee.
 *
 * @note Verifie en premier lieu que l'utilisateur connecte
 *       est bien un administrateur via verifierAdmin().
 * @see  verifierAdmin(), deconnecterUtilisateur()
 */
void menuAdministrateur() {
    if (!verifierAdmin()) return;
    int choix;

    do {
        printf("\n=== PANNEAU ADMINISTRATEUR ===\n");
        printf("Connecte : %s\n", sessionCourante.utilisateur.prenom);
        separateur();
        printf("1. Voir tous les utilisateurs\n");
        printf("2. Activer / Desactiver un compte\n");
        printf("3. Supprimer un utilisateur\n");
        printf("4. Voir tous les logements\n");
        printf("5. Supprimer un logement\n");
        printf("6. Statistiques generales\n");
        printf("0. Se deconnecter\n");
        printf("Choix : ");
        scanf("%d", &choix); viderBuffer();

        switch (choix) {
            case 1: adminVoirUtilisateurs();    break;
            case 2: adminToggleCompte();         break;
            case 3: adminSupprimerUtilisateur(); break;
            case 4: adminVoirLogements();        break;
            case 5: adminSupprimerLogement();    break;
            case 6: adminStatistiques();         break;
            case 0: deconnecterUtilisateur();    break;
            default: printf("[ERREUR] Choix invalide.\n");
        }
    } while (choix != 0 && sessionCourante.connecte);
}
