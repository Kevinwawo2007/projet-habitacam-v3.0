/* ============================================================
 * @file    auth.c
 * @brief   Module d'authentification de HabitatCam.
 *
 * Gere l'inscription, la connexion, la deconnexion et
 * la persistance des utilisateurs dans le fichier texte.
 *
 * @version 1.0
 * @date    2025-2026
 * @author  SOUOPGUI
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Structure.h"
#include "auth.h"

/* ============================================================
 * VARIABLES GLOBALES
 * ============================================================ */

/** @brief Tableau contenant tous les utilisateurs charges en memoire. */
Utilisateur listeUtilisateurs[MAX_UTILISATEURS];

/** @brief Nombre d'utilisateurs actuellement enregistres. */
int nbUtilisateurs = 0;

/** @brief Session de l'utilisateur actuellement connecte. */
Session sessionCourante = {0};


/* ============================================================
 * FONCTIONS UTILITAIRES INTERNES
 * ============================================================ */

/**
 * @brief Vide le tampon du clavier.
 *
 * Consomme tous les caracteres restants dans stdin
 * jusqu'a rencontrer un saut de ligne ou EOF.
 * A appeler apres chaque scanf() pour eviter les bugs de saisie.
 */
static void viderBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * @brief Saisit un mot de passe en masquant les caracteres avec '*'.
 *
 * @param mdp    Tableau de caracteres ou stocker le mot de passe saisi.
 * @param taille Taille maximale du tableau (inclut le '\0' final).
 */
static void saisirMotDePasse(char *mdp, int taille) {
    int i = 0;
    char c;
    printf("Mot de passe : ");
    fflush(stdout);
    while (i < taille - 1) {
        c = getchar();
        if (c == '\n' || c == EOF) break;
        mdp[i++] = c;
        putchar('*');
        fflush(stdout);
    }
    mdp[i] = '\0';
    putchar('\n');
}

/**
 * @brief Verifie si un email est deja utilise par un compte existant.
 *
 * @param email Adresse email a verifier.
 * @return      1 si l'email existe deja, 0 sinon.
 */
int emailExiste(const char *email) {
    for (int i = 0; i < nbUtilisateurs; i++) {
        if (strcmp(listeUtilisateurs[i].email, email) == 0)
            return 1;
    }
    return 0;
}

/**
 * @brief Recherche un utilisateur par email et mot de passe.
 *
 * Verifie egalement que le compte est actif (non desactive par l'admin).
 *
 * @param email Adresse email saisie par l'utilisateur.
 * @param mdp   Mot de passe saisi par l'utilisateur.
 * @return      Index de l'utilisateur dans listeUtilisateurs si trouve,
 *              -1 sinon (identifiants incorrects ou compte desactive).
 */
int rechercherUtilisateur(const char *email, const char *mdp) {
    for (int i = 0; i < nbUtilisateurs; i++) {
        if (strcmp(listeUtilisateurs[i].email, email) == 0 &&
            strcmp(listeUtilisateurs[i].motDePasse, mdp) == 0 &&
            listeUtilisateurs[i].actif == 1) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Genere un nouvel identifiant unique pour un utilisateur.
 *
 * Parcourt listeUtilisateurs pour trouver le plus grand ID existant
 * et retourne ce maximum incremente de 1.
 *
 * @return Nouvel ID unique (entier positif).
 */
int genererIdUtilisateur() {
    int maxId = 0;
    for (int i = 0; i < nbUtilisateurs; i++) {
        if (listeUtilisateurs[i].id > maxId)
            maxId = listeUtilisateurs[i].id;
    }
    return maxId + 1;
}


/* ============================================================
 * PERSISTANCE
 * ============================================================ */

/**
 * @brief Charge les utilisateurs depuis le fichier de donnees.
 *
 * Lit le fichier FICHIER_UTILISATEURS ligne par ligne et remplit
 * listeUtilisateurs. Si le fichier est absent (premiere execution),
 * cree automatiquement un compte administrateur par defaut :
 * email = admin@habitatcam.cm, mot de passe = admin123.
 *
 * @note Le fichier est au format : id|nom|prenom|email|tel|mdp|role|actif
 * @warning Ne pas modifier manuellement le fichier .txt sans respecter ce format.
 */
void chargerUtilisateurs() {
    FILE *f = fopen(FICHIER_UTILISATEURS, "r");
    if (f == NULL) {
        printf("[INFO] Aucun fichier utilisateurs trouve. Creation de l'admin par defaut.\n");

        Utilisateur admin;
        admin.id   = 1;
        strcpy(admin.nom,        "Admin");
        strcpy(admin.prenom,     "Super");
        strcpy(admin.email,      "admin@habitatcam.cm");
        strcpy(admin.telephone,  "000000000");
        strcpy(admin.motDePasse, "admin123");
        admin.role  = ROLE_ADMINISTRATEUR;
        admin.actif = 1;

        listeUtilisateurs[0] = admin;
        nbUtilisateurs = 1;
        sauvegarderUtilisateurs();
        return;
    }

    nbUtilisateurs = 0;
    Utilisateur u;
    while (fscanf(f, "%d|%49[^|]|%49[^|]|%79[^|]|%19[^|]|%49[^|]|%d|%d\n",
                  &u.id, u.nom, u.prenom, u.email,
                  u.telephone, u.motDePasse,
                  (int*)&u.role, &u.actif) == 8) {
        listeUtilisateurs[nbUtilisateurs++] = u;
        if (nbUtilisateurs >= MAX_UTILISATEURS) break;
    }
    fclose(f);
}

/**
 * @brief Sauvegarde tous les utilisateurs dans le fichier de donnees.
 *
 * Cree le dossier data/ si absent, puis ecrit chaque utilisateur
 * sur une ligne au format : id|nom|prenom|email|tel|mdp|role|actif.
 * Ecrase le fichier existant a chaque appel (sauvegarde complete).
 *
 * @note A appeler apres toute modification de listeUtilisateurs.
 */
void sauvegarderUtilisateurs() {
    system("mkdir -p data");

    FILE *f = fopen(FICHIER_UTILISATEURS, "w");
    if (f == NULL) {
        printf("[ERREUR] Impossible d'ouvrir le fichier utilisateurs.\n");
        return;
    }
    for (int i = 0; i < nbUtilisateurs; i++) {
        Utilisateur *u = &listeUtilisateurs[i];
        fprintf(f, "%d|%s|%s|%s|%s|%s|%d|%d\n",
                u->id, u->nom, u->prenom, u->email,
                u->telephone, u->motDePasse,
                (int)u->role, u->actif);
    }
    fclose(f);
}


/* ============================================================
 * INSCRIPTION
 * ============================================================ */

/**
 * @brief Inscrit un nouvel utilisateur sur la plateforme.
 *
 * Saisit les informations du nouvel utilisateur (nom, prenom,
 * email, telephone, mot de passe avec confirmation, role).
 * Verifie que l'email n'est pas deja utilise avant d'enregistrer.
 * Le role Administrateur n'est pas accessible via ce formulaire.
 *
 * @note Le compte est sauvegarde immediatement apres validation.
 * @warning Echoue silencieusement si MAX_UTILISATEURS est atteint.
 */
void inscrireUtilisateur() {
    if (nbUtilisateurs >= MAX_UTILISATEURS) {
        printf("[ERREUR] Nombre maximum d'utilisateurs atteint.\n");
        return;
    }

    Utilisateur nouveau;
    char confirmation[TAILLE_MDP];
    int roleChoix;

    printf("\n[INSCRIPTION]\n");

    printf("Nom        : "); fgets(nouveau.nom,    TAILLE_NOM,   stdin); nouveau.nom[strcspn(nouveau.nom, "\n")]       = '\0';
    printf("Prenom     : "); fgets(nouveau.prenom, TAILLE_NOM,   stdin); nouveau.prenom[strcspn(nouveau.prenom, "\n")] = '\0';
    printf("Email      : "); fgets(nouveau.email,  TAILLE_EMAIL, stdin); nouveau.email[strcspn(nouveau.email, "\n")]   = '\0';

    if (emailExiste(nouveau.email)) {
        printf("[ERREUR] Cet email est deja utilise.\n");
        return;
    }

    printf("Telephone  : "); fgets(nouveau.telephone, TAILLE_TEL, stdin); nouveau.telephone[strcspn(nouveau.telephone, "\n")] = '\0';

    do {
        saisirMotDePasse(nouveau.motDePasse, TAILLE_MDP);
        printf("Confirmation : ");
        saisirMotDePasse(confirmation, TAILLE_MDP);
        if (strcmp(nouveau.motDePasse, confirmation) != 0)
            printf("[ERREUR] Les mots de passe ne correspondent pas.\n");
    } while (strcmp(nouveau.motDePasse, confirmation) != 0);

    printf("\n1. Locataire\n2. Bailleur\nChoix : ");
    scanf("%d", &roleChoix);
    viderBuffer();

    if      (roleChoix == 1) nouveau.role = ROLE_LOCATAIRE;
    else if (roleChoix == 2) nouveau.role = ROLE_BAILLEUR;
    else { printf("[ERREUR] Choix invalide.\n"); return; }

    nouveau.id    = genererIdUtilisateur();
    nouveau.actif = 1;

    listeUtilisateurs[nbUtilisateurs++] = nouveau;
    sauvegarderUtilisateurs();
    printf("[OK] Inscription reussie ! Bienvenue, %s.\n", nouveau.prenom);
}


/* ============================================================
 * CONNEXION
 * ============================================================ */

/**
 * @brief Connecte un utilisateur existant et ouvre une session.
 *
 * Demande l'email et le mot de passe. Autorise un maximum de
 * 3 tentatives consecutives avant de bloquer et de retourner
 * au menu principal. En cas de succes, remplit sessionCourante.
 *
 * @note La session est stockee dans la variable globale sessionCourante.
 * @warning Un compte desactive ne peut pas se connecter meme avec
 *          les bons identifiants.
 */
void connecterUtilisateur() {
    char email[TAILLE_EMAIL];
    char mdp[TAILLE_MDP];
    int  tentatives = 0, index;

    printf("\n[CONNEXION]\n");

    while (tentatives < 3) {
        printf("Email : "); fgets(email, TAILLE_EMAIL, stdin); email[strcspn(email, "\n")] = '\0';
        saisirMotDePasse(mdp, TAILLE_MDP);
        index = rechercherUtilisateur(email, mdp);

        if (index != -1) {
            sessionCourante.connecte    = 1;
            sessionCourante.utilisateur = listeUtilisateurs[index];
            printf("[OK] Bienvenue, %s %s !\n", sessionCourante.utilisateur.prenom, sessionCourante.utilisateur.nom);
            return;
        }
        printf("[ERREUR] Identifiants incorrects. (%d/3)\n", ++tentatives);
    }
    printf("[SECURITE] Trop de tentatives. Retour au menu.\n");
}


/* ============================================================
 * DECONNEXION
 * ============================================================ */

/**
 * @brief Deconnecte l'utilisateur courant et reinitialise la session.
 *
 * Affiche un message d'au revoir, passe sessionCourante.connecte a 0
 * et efface les donnees de l'utilisateur en session.
 */
void deconnecterUtilisateur() {
    printf("\n[OK] Au revoir, %s !\n", sessionCourante.utilisateur.prenom);
    sessionCourante.connecte = 0;
    memset(&sessionCourante.utilisateur, 0, sizeof(Utilisateur));
}


/* ============================================================
 * MENU PRINCIPAL D'AUTHENTIFICATION
 * ============================================================ */

/**
 * @brief Affiche le menu de bienvenue et orchestre l'authentification.
 *
 * Propose trois choix : connexion, inscription, quitter.
 * La boucle continue tant qu'aucun utilisateur n'est connecte
 * et que l'utilisateur ne choisit pas de quitter (choix 0).
 */
void menuAuthentification() {
    int choix;
    do {
        printf("\n=== HABITATCAM v1.0 ===\n");
        printf("1. Se connecter\n");
        printf("2. Creer un compte\n");
        printf("0. Quitter\n");
        printf("Choix : ");
        scanf("%d", &choix);
        viderBuffer();

        switch (choix) {
            case 1: connecterUtilisateur(); break;
            case 2: inscrireUtilisateur();  break;
            case 0: printf("Au revoir !\n"); break;
            default: printf("[ERREUR] Choix invalide.\n");
        }
    } while (choix != 0 && sessionCourante.connecte == 0);
}
