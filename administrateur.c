/* ============================================================
 * @file    administrateur.c
 * @brief   Module Administrateur de HabitatCam V2.0.
 *
 * Ameliorations V2.0 :
 *   - Nouvelle fonction adminReactiverCompte() pour debloquer
 *     les comptes verrouilles apres 3 echecs
 *   - Messages d'erreur precis avec AuthStatus
 *   - Menus strictement reserves a l'administrateur
 *   - Validation des saisies avec saisirEntier()
 *
 * @version 2.0
 * @author  SOUOPGUI
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structures.h"
#include "auth.h"
#include "administrateur.h"

/* Lit une ligne depuis le clavier (copie de auth.c pour ce module) */
static void lire_ligne(const char *invite, char *buffer, int taille)
{
    printf("%s", invite);
    if (fgets(buffer, taille, stdin))
        buffer[strcspn(buffer, "\n\r")] = '\0';
}

/* Variables definies dans logement.c */
extern Logement    listeLogements[MAX_LOGEMENTS];
extern int         nbLogements;
extern Reservation listeReservations[MAX_RESERVATIONS];
extern int         nbReservations;
extern void        sauvegarderLogements();

/* ============================================================
 * UTILITAIRES INTERNES
 * ============================================================ */

/**
 * @brief Lit un entier depuis le clavier de facon securisee.
 * @return L'entier saisi, ou -99 si saisie invalide.
 */
static int saisirEntier()
{
    char buffer[32];
    int val;
    if (fgets(buffer, sizeof(buffer), stdin))
    {
        buffer[strcspn(buffer, "\n\r")] = '\0';
        if (sscanf(buffer, "%d", &val) == 1)
            return val;
    }
    return -99;
}

/**
 * @brief Affiche une ligne de separation.
 */
static void separateur()
{
    printf("---------------------------------------------\n");
}

/**
 * @brief Convertit un role en texte lisible.
 * @param r Role a convertir.
 * @return Chaine correspondante.
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
 * @param s Statut a convertir.
 * @return Chaine correspondante.
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
 * @brief Verifie que l'utilisateur connecte est bien un admin.
 *
 * Bloque l'acces si le role n'est pas ROLE_ADMINISTRATEUR.
 * A appeler en debut de chaque fonction sensible.
 *
 * @return 1 si admin, 0 sinon.
 */
static int verifierAdmin()
{
    if (!sessionCourante.connecte ||
            sessionCourante.utilisateur.role != ROLE_ADMINISTRATEUR)
    {
        printf("[ACCES REFUSE] Cette section est reservee a l'administrateur.\n");
        return 0;
    }
    return 1;
}


/* ============================================================
 * GESTION DES UTILISATEURS
 * ============================================================ */

/**
 * @brief Affiche la liste complete de tous les utilisateurs.
 *
 * Affiche ID, nom, prenom, email, role, echecs et statut
 * de chaque compte enregistre sur la plateforme.
 *
 * @note Accessible uniquement au role Administrateur.
 */
void adminVoirUtilisateurs()
{
    if (!verifierAdmin()) return;

    printf("\n[LISTE DES UTILISATEURS]\n");
    if (nbUtilisateurs == 0)
    {
        printf("Aucun utilisateur enregistre.\n");
        return;
    }

    separateur();
    printf("%-4s %-15s %-15s %-25s %-14s %-7s %s\n",
           "ID", "Nom", "Prenom", "Email", "Role", "Echecs", "Statut");
    separateur();

    for (int i = 0; i < nbUtilisateurs; i++)
    {
        Utilisateur *u = &listeUtilisateurs[i];
        printf("%-4d %-15s %-15s %-25s %-14s %-7d %s\n",
               u->id, u->nom, u->prenom, u->email,
               roleEnTexte(u->role), u->nbEchecs,
               u->actif ? "Actif" : "DESACTIVE");
    }
    separateur();
    printf("Total : %d utilisateur(s)\n", nbUtilisateurs);
}

/**
 * @brief Active ou desactive le compte d'un utilisateur.
 *
 * Inverse le champ actif du compte cible.
 * Remet aussi nbEchecs a 0 quand on reactive un compte.
 *
 * @note Ne peut pas modifier son propre compte ni un autre admin.
 */
void adminToggleCompte()
{
    if (!verifierAdmin()) return;
    adminVoirUtilisateurs();

    printf("\nID du compte a activer/desactiver : ");
    int id = saisirEntier();

    if (id == -99)
    {
        printf("[ERREUR] Saisie invalide.\n");
        return;
    }
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
                printf("[ERREUR] Impossible de modifier un autre administrateur.\n");
                return;
            }
            listeUtilisateurs[i].actif = !listeUtilisateurs[i].actif;
            /* Si on reactive, remettre les echecs a 0 */
            if (listeUtilisateurs[i].actif == 1)
                listeUtilisateurs[i].nbEchecs = 0;
            sauvegarderUtilisateurs();
            printf("[OK] Compte de %s : %s\n",
                   listeUtilisateurs[i].prenom,
                   listeUtilisateurs[i].actif ? "ACTIVE" : "DESACTIVE");
            return;
        }
    }
    printf("[ERREUR] Aucun utilisateur avec l'ID %d.\n", id);
}

/**
 * @brief Reactive specifiquement un compte verrouille.
 *
 * Fonction dediee aux comptes bloques apres 3 echecs de
 * connexion. Remet actif = 1 et nbEchecs = 0.
 *
 * @note Accessible uniquement au role Administrateur.
 */
void adminReactiverCompte()
{
    if (!verifierAdmin()) return;

    printf("\n[COMPTES VERROUILLES]\n");
    separateur();

    int trouve = 0;
    for (int i = 0; i < nbUtilisateurs; i++)
    {
        if (listeUtilisateurs[i].actif == 0)
        {
            printf("ID: %-4d | %-15s %-15s | Echecs: %d\n",
                   listeUtilisateurs[i].id,
                   listeUtilisateurs[i].prenom,
                   listeUtilisateurs[i].nom,
                   listeUtilisateurs[i].nbEchecs);
            trouve = 1;
        }
    }

    if (!trouve)
    {
        printf("Aucun compte verrouille.\n");
        return;
    }

    separateur();
    printf("ID du compte a reactiver : ");
    int id = saisirEntier();

    if (id == -99)
    {
        printf("[ERREUR] Saisie invalide.\n");
        return;
    }

    for (int i = 0; i < nbUtilisateurs; i++)
    {
        if (listeUtilisateurs[i].id == id)
        {
            listeUtilisateurs[i].actif    = 1;
            listeUtilisateurs[i].nbEchecs = 0;
            sauvegarderUtilisateurs();
            printf("[OK] Compte de %s reactive avec succes.\n",
                   listeUtilisateurs[i].prenom);
            return;
        }
    }
    printf("[ERREUR] Aucun utilisateur avec l'ID %d.\n", id);
}

/**
 * @brief Supprime definitivement un utilisateur.
 *
 * Decale le tableau et decremente nbUtilisateurs.
 *
 * @note Action irreversible.
 * @note Ne peut pas supprimer son propre compte ni un admin.
 */
void adminSupprimerUtilisateur()
{
    if (!verifierAdmin()) return;
    adminVoirUtilisateurs();

    printf("\nID du compte a supprimer : ");
    int id = saisirEntier();

    if (id == -99)
    {
        printf("[ERREUR] Saisie invalide.\n");
        return;
    }
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
            printf("[OK] Compte de %s (ID: %d) supprime.\n", prenom, id);
            return;
        }
    }
    printf("[ERREUR] Aucun utilisateur avec l'ID %d.\n", id);
}


/* ============================================================
 * GESTION DES LOGEMENTS
 * ============================================================ */

/**
 * @brief Affiche tous les logements de la plateforme.
 * @note Accessible uniquement au role Administrateur.
 */
void adminVoirLogements()
{
    if (!verifierAdmin()) return;

    printf("\n[LISTE DES LOGEMENTS]\n");
    if (nbLogements == 0)
    {
        printf("Aucun logement enregistre.\n");
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
 * @brief Supprime definitivement un logement.
 * @note Action irreversible.
 * @note Accessible uniquement au role Administrateur.
 */
void adminSupprimerLogement()
{
    if (!verifierAdmin()) return;
    adminVoirLogements();

    printf("\nID du logement a supprimer : ");
    int id = saisirEntier();

    if (id == -99)
    {
        printf("[ERREUR] Saisie invalide.\n");
        return;
    }

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
 * Calcule en temps reel :
 *   - Repartition des utilisateurs par role
 *   - Comptes actifs / desactives / verrouilles
 *   - Repartition des logements par statut
 *   - Nombre total de reservations
 *
 * @note Accessible uniquement au role Administrateur.
 */
void adminStatistiques()
{
    if (!verifierAdmin()) return;

    int nbLoc=0, nbBaill=0, nbAdm=0;
    int nbActifs=0, nbInactifs=0, nbVerrouilles=0;
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
        if (listeUtilisateurs[i].actif)
            nbActifs++;
        else
        {
            nbInactifs++;
            if (listeUtilisateurs[i].nbEchecs >= 3)
                nbVerrouilles++;
        }
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
    printf("  Actifs          : %d\n", nbActifs);
    printf("  Desactives      : %d\n", nbInactifs);
    printf("  Verrouilles     : %d  (trop d'echecs de connexion)\n", nbVerrouilles);
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
 * GESTION DES DEMANDES DE REINITIALISATION
 * ============================================================ */

/**
 * @brief Affiche et traite les demandes de reinitialisation de mdp.
 *
 * Lit data/demandes_reinit.txt et liste les utilisateurs
 * ayant fait une demande. L'admin peut alors :
 *   1. Attribuer un mot de passe temporaire a un utilisateur
 *   2. Reactiver son compte automatiquement
 *   L'utilisateur devra ensuite changer ce mot de passe
 *   temporaire via l'option "Changer mon mot de passe".
 *
 * @note Accessible uniquement au role Administrateur.
 */
void adminGererDemandes()
{
    if (!verifierAdmin()) return;

    printf("\n[DEMANDES DE REINITIALISATION DE MOT DE PASSE]\n");
    separateur();

    /* Lire le fichier des demandes */
    FILE *f = fopen("data/demandes_reinit.txt", "r");
    if (!f)
    {
        printf("Aucune demande en attente.\n");
        return;
    }

    char ligne[TAILLE_EMAIL + 20];
    char emails[50][TAILLE_EMAIL];
    int  nbDemandes = 0;

    /* Afficher toutes les demandes EN_ATTENTE */
    while (fgets(ligne, sizeof(ligne), f) && nbDemandes < 50)
    {
        ligne[strcspn(ligne, "\n\r")] = '\0';

        /* Format : email|statut */
        char *sep = strchr(ligne, '|');
        if (!sep) continue;
        *sep = '\0';
        char *statut = sep + 1;

        if (strcmp(statut, "EN_ATTENTE") == 0)
        {
            strncpy(emails[nbDemandes], ligne, TAILLE_EMAIL - 1);
            printf("%d. %s\n", nbDemandes + 1, emails[nbDemandes]);
            nbDemandes++;
        }
    }
    fclose(f);

    if (nbDemandes == 0)
    {
        printf("Aucune demande en attente.\n");
        return;
    }

    separateur();
    printf("Numero de la demande a traiter (0=Annuler) : ");
    int choix = saisirEntier();

    if (choix <= 0 || choix > nbDemandes)
    {
        printf("Annule.\n");
        return;
    }

    /* Trouver l'utilisateur correspondant */
    char *emailCible = emails[choix - 1];
    int index = -1;
    for (int i = 0; i < nbUtilisateurs; i++)
    {
        if (strcmp(listeUtilisateurs[i].email, emailCible) == 0)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        printf("[ERREUR] Utilisateur introuvable.\n");
        return;
    }

    /* Saisir le mot de passe temporaire */
    char mdpTemp[TAILLE_MDP];
    printf("\nMot de passe temporaire pour %s %s : ",
           listeUtilisateurs[index].prenom,
           listeUtilisateurs[index].nom);
    lire_ligne("", mdpTemp, TAILLE_MDP);

    /* Mettre a jour le compte */
    strncpy(listeUtilisateurs[index].motDePasse, mdpTemp, TAILLE_MDP - 1);
    listeUtilisateurs[index].motDePasse[TAILLE_MDP - 1] = '\0';
    listeUtilisateurs[index].actif    = 1;
    listeUtilisateurs[index].nbEchecs = 0;
    sauvegarderUtilisateurs();

    /* Marquer la demande comme TRAITEE dans le fichier */
    FILE *fin  = fopen("data/demandes_reinit.txt", "r");
    FILE *fout = fopen("data/demandes_reinit_tmp.txt", "w");
    if (fin && fout)
    {
        char buf[TAILLE_EMAIL + 20];
        while (fgets(buf, sizeof(buf), fin))
        {
            buf[strcspn(buf, "\n\r")] = '\0';
            char *s = strchr(buf, '|');
            if (s)
            {
                *s = '\0';
                if (strcmp(buf, emailCible) == 0)
                    fprintf(fout, "%s|TRAITE\n", buf);
                else
                    fprintf(fout, "%s|%s\n", buf, s + 1);
            }
        }
        fclose(fin);
        fclose(fout);
        remove("data/demandes_reinit.txt");
        rename("data/demandes_reinit_tmp.txt", "data/demandes_reinit.txt");
    }

    printf("\n[OK] Mot de passe temporaire attribue a %s.\n",
           listeUtilisateurs[index].prenom);
    printf("     Compte reactive. L'utilisateur doit changer\n");
    printf("     son mot de passe a la prochaine connexion.\n");
}

/* ============================================================
 * MENU ADMINISTRATEUR
 * ============================================================ */

/**
 * @brief Point d'entree du module administrateur.
 *
 * Affiche toutes les fonctions reservees a l'admin.
 * Boucle jusqu'a deconnexion (choix 0).
 *
 * @note Verifie en premier lieu le role via verifierAdmin().
 */
void menuAdministrateur()
{
    if (!verifierAdmin()) return;

    int choix;
    do
    {
        printf("\n=== PANNEAU ADMINISTRATEUR ===\n");
        printf("Connecte : %s\n", sessionCourante.utilisateur.prenom);
        separateur();
        printf("1. Voir tous les utilisateurs\n");
        printf("2. Activer / Desactiver un compte\n");
        printf("3. Reactiver un compte verrouille\n");
        printf("4. Supprimer un utilisateur\n");
        printf("5. Voir tous les logements\n");
        printf("6. Supprimer un logement\n");
        printf("7. Statistiques generales\n");
        printf("8. Modifier le code secret admin\n");
        printf("9. Demandes de reinitialisation de mdp\n");
        printf("10. Changer mon mot de passe\n");
        printf("0. Se deconnecter\n");
        separateur();
        printf("Choix : ");
        choix = saisirEntier();

        if (choix == -99)
        {
            printf("[ERREUR] Entrez un chiffre valide (0 a 10).\n");
            continue;
        }

        switch (choix)
        {
        case 1:
            adminVoirUtilisateurs();
            break;
        case 2:
            adminToggleCompte();
            break;
        case 3:
            adminReactiverCompte();
            break;
        case 4:
            adminSupprimerUtilisateur();
            break;
        case 5:
            adminVoirLogements();
            break;
        case 6:
            adminSupprimerLogement();
            break;
        case 7:
            adminStatistiques();
            break;
        case 8:
            modifierCodeSecret();
            break;
        case 9:
            adminGererDemandes();
            break;
        case 10:
            changerMotDePasse();
            break;
        case 0:
            deconnecterUtilisateur();
            break;
        default:
            printf("[ERREUR] Le choix %d n'existe pas.\n", choix);
            printf("         Veuillez choisir entre 0 et 10.\n");
        }
    }
    while (choix != 0 && sessionCourante.connecte);
}
