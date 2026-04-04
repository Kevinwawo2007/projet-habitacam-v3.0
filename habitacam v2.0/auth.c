/* ============================================================
 * @file    auth.c
 * @brief   Module d'authentification de HabitatCam V2.0.
 *
 * Ce fichier gere :
 *   - L'inscription des locataires et bailleurs
 *   - La connexion avec verrouillage apres 3 echecs
 *   - La deconnexion
 *   - L'acces administrateur securise (3 niveaux)
 *   - Le changement de mot de passe et d'email admin
 *   - Le mot de passe oublie (demande a l'admin)
 *   - La persistance dans data/utilisateurs.txt
 *
 * Compatible Windows (Code::Blocks) et Linux (gcc).
 *
 * @version 2.0
 * @author  SOUOPGUI
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * COMPATIBILITE WINDOWS / LINUX
 *
 * Sur Windows : conio.h fournit _getch() nativement.
 * Sur Linux   : on la recree avec termios.h.
 * Le compilateur choisit automatiquement selon le systeme.
 * ============================================================ */
#ifdef _WIN32
    #include <conio.h>
    static void creerDossierData() {
        system("if not exist data mkdir data");
    }
#else
    #include <termios.h>
    #include <unistd.h>
    #include <sys/stat.h>

    /**
     * @brief Lit une touche sans l'afficher — version Linux.
     * Passe le terminal en mode raw, lit 1 caractere,
     * remet le terminal en mode normal.
     */
    static int _getch() {
        struct termios ancien, nouveau;
        tcgetattr(STDIN_FILENO, &ancien);
        nouveau          = ancien;
        nouveau.c_lflag &= ~(ICANON | ECHO);
        nouveau.c_cc[VMIN]  = 1;
        nouveau.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &nouveau);
        int c = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &ancien);
        return c;
    }

    static void creerDossierData() {
        mkdir("data", 0755);
    }
#endif

#include "structures.h"
#include "auth.h"


/* ============================================================
 * VARIABLES GLOBALES
 * ============================================================ */

Utilisateur listeUtilisateurs[MAX_UTILISATEURS];
int         nbUtilisateurs  = 0;
Session     sessionCourante = {0};


/* ============================================================
 * UTILITAIRES INTERNES
 * ============================================================ */

/**
 * @brief Vide le buffer stdin.
 *
 * A appeler APRES saisirMotDePasse() et AVANT lire_ligne().
 * Sans ca, lire_ligne() lirait le \n residuel laisse par
 * _getch() et retournerait une chaine vide.
 *
 * C'est la cause principale du bug "Aucun compte trouve"
 * apres la saisie du code secret.
 */
static void viderStdin() {
    int c;
    /* Lire et ignorer tout jusqu'au \n ou la fin */
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * @brief Lit une ligne de texte depuis le clavier.
 *
 * Affiche l'invite, lit toute la ligne et supprime \n et \r.
 *
 * @param invite  Texte affiche avant la saisie.
 * @param buffer  Tableau ou stocker la saisie.
 * @param taille  Taille maximale du tableau.
 */
static void lire_ligne(const char *invite, char *buffer, int taille) {
    printf("%s", invite);
    fflush(stdout);
    if (fgets(buffer, taille, stdin))
        buffer[strcspn(buffer, "\n\r")] = '\0';
}

/**
 * @brief Lit un entier depuis le clavier.
 * Retourne -99 si la saisie est invalide.
 */
static int saisirEntier() {
    char buffer[32];
    int  val;
    lire_ligne("", buffer, sizeof(buffer));
    if (sscanf(buffer, "%d", &val) != 1)
        return -99;
    return val;
}

/**
 * @brief Saisit un mot de passe de facon securisee avec toggle TAB.
 *
 * PRINCIPE DE FONCTIONNEMENT :
 *   Le libelle (ex: "Mot de passe : ") est affiche UNE SEULE FOIS
 *   avant la boucle. Ensuite, on affiche uniquement l indicateur
 *   de mode ([TAB=voir]) et les etoiles/caracteres. Pour effacer
 *   et reafficher proprement, on utilise des caracteres Backspace
 *   (\b) plutot que \r, ce qui evite le bug de superposition
 *   observe quand plusieurs lignes s affichent en meme temps.
 *
 * POURQUOI PAS \r :
 *   \r revient au debut de la ligne mais ne tient pas compte
 *   de la longueur du libelle. Sur certains terminaux Windows,
 *   cela cause la repetition du libelle sur plusieurs colonnes.
 *
 * TOUCHES DISPONIBLES :
 *   TAB       -> bascule visible (*) / cache (texte)
 *   Backspace -> efface le dernier caractere
 *   Entree    -> valide et termine
 *   Autres    -> ajoute au mot de passe
 *
 * @param libelle Texte affiche avant la zone de saisie.
 * @param mdp     Tableau ou stocker le mot de passe.
 * @param taille  Taille maximale du tableau.
 */
static void saisirMotDePasse(const char *libelle, char *mdp, int taille) {
    int len     = 0;
    int visible = 0;
    int c, i;

    mdp[0] = '\0';

    /* Afficher le libelle UNE SEULE FOIS
     * puis l indicateur de mode initial */
    printf("%s[TAB=voir  ] ", libelle);
    fflush(stdout);

    while (1) {
        c = _getch();

        #ifdef _WIN32
        /* Touches speciales Windows (fleches, F1-F12...) :
         * premier octet = 0 ou 0xE0, second = code specifique.
         * On lit et ignore les deux octets. */
        if (c == 0 || c == 0xE0) {
            _getch();
            continue;
        }
        #endif

        /* ── Entree : fin de saisie ── */
        if (c == '\r' || c == '\n') {
            /* Si on etait en visible, effacer et remontrer en cache */
            if (visible) {
                /* Effacer : indicateur (13 chars) + len caracteres */
                for (i = 0; i < 13 + len; i++) printf("\b \b");
                printf("[TAB=voir  ] ");
                for (i = 0; i < len; i++) putchar('*');
                fflush(stdout);
            }
            printf("\n");
            break;
        }

        /* ── TAB : basculer visible / cache ── */
        if (c == 9) {
            if (!visible) {
                /* Passer en visible :
                 * effacer les etoiles + indicateur cache
                 * puis afficher indicateur visible + texte clair */
                for (i = 0; i < 13 + len; i++) printf("\b \b");
                printf("[TAB=cacher] %s", mdp);
            } else {
                /* Repasser en cache :
                 * effacer texte + indicateur visible
                 * puis afficher indicateur cache + etoiles */
                for (i = 0; i < 13 + len; i++) printf("\b \b");
                printf("[TAB=voir  ] ");
                for (i = 0; i < len; i++) putchar('*');
            }
            visible = !visible;
            fflush(stdout);
            continue;
        }

        /* ── Backspace : effacer le dernier caractere ── */
        if ((c == 8 || c == 127) && len > 0) {
            len--;
            mdp[len] = '\0';
            /* Effacer 1 caractere a l ecran (etoile ou lettre) */
            printf("\b \b");
            fflush(stdout);
            continue;
        }

        /* ── Caractere imprimable (codes 32 a 126) ── */
        if (c >= 32 && c <= 126 && len < taille - 1) {
            /* Si on etait en visible et on retape :
             * repasser en cache automatiquement */
            if (visible) {
                /* Effacer texte visible + indicateur */
                for (i = 0; i < 13 + len; i++) printf("\b \b");
                printf("[TAB=voir  ] ");
                for (i = 0; i < len; i++) putchar('*');
                visible = 0;
            }
            mdp[len++] = (char)c;
            mdp[len]   = '\0';
            /* Afficher une etoile (mode cache) */
            putchar('*');
            fflush(stdout);
        }
    }
}


/* ============================================================
 * MESSAGES D'ERREUR
 * ============================================================ */

/**
 * @brief Affiche un message d'erreur precis selon AuthStatus.
 */
void afficherErreurAuth(AuthStatus st) {
    switch (st) {
        case AUTH_ERR_NOTFOUND:
            printf("[ERREUR] Aucun compte trouve avec cet email.\n");
            break;
        case AUTH_ERR_INVALID:
            printf("[ERREUR] Mot de passe incorrect ou compte desactive.\n");
            break;
        case AUTH_ERR_EXISTS:
            printf("[ERREUR] Cet email est deja utilise.\n");
            break;
        case AUTH_ERR_IO:
            printf("[ERREUR] Probleme d'acces au fichier.\n");
            break;
        default:
            printf("[ERREUR] Erreur inconnue.\n");
    }
}


/* ============================================================
 * UTILITAIRES METIER
 * ============================================================ */

/**
 * @brief Verifie si un email est deja utilise.
 * @return 1 si existe, 0 sinon.
 */
int emailExiste(const char *email) {
    int i;
    for (i = 0; i < nbUtilisateurs; i++)
        if (strcmp(listeUtilisateurs[i].email, email) == 0) return 1;
    return 0;
}

/**
 * @brief Recherche un utilisateur par email.
 * @return Index dans listeUtilisateurs, ou -1 si non trouve.
 */
int rechercherUtilisateurParEmail(const char *email) {
    int i;
    for (i = 0; i < nbUtilisateurs; i++)
        if (strcmp(listeUtilisateurs[i].email, email) == 0) return i;
    return -1;
}

/**
 * @brief Genere un ID unique pour un nouvel utilisateur.
 * @return Le plus grand ID existant + 1.
 */
int genererIdUtilisateur() {
    int maxId = 0, i;
    for (i = 0; i < nbUtilisateurs; i++)
        if (listeUtilisateurs[i].id > maxId) maxId = listeUtilisateurs[i].id;
    return maxId + 1;
}


/* ============================================================
 * PERSISTANCE
 * ============================================================ */

/**
 * @brief Charge les utilisateurs depuis data/utilisateurs.txt.
 *
 * Compatible V1.0 (8 champs) et V2.0 (9 champs avec nbEchecs).
 * Si le fichier est absent ou vide, cree l'admin par defaut :
 *   Email : admin@habitatcam.cm / Mot de passe : admin123
 *
 * Format V2.0 : id|nom|prenom|email|tel|mdp|role|nbEchecs|actif
 */
void chargerUtilisateurs() {
    FILE *f = fopen(FICHIER_UTILISATEURS, "r");

    if (f == NULL) {
        /* Fichier absent : creer l'admin par defaut */
        goto creer_admin;
    }

    nbUtilisateurs = 0;
    Utilisateur u;
    int nb;

    while (!feof(f)) {
        memset(&u, 0, sizeof(Utilisateur));
        nb = fscanf(f,
            "%d|%49[^|]|%49[^|]|%79[^|]|%19[^|]|%49[^|]|%d|%d|%d\n",
            &u.id, u.nom, u.prenom, u.email, u.telephone,
            u.motDePasse, (int*)&u.role, &u.nbEchecs, &u.actif);

        if (nb == 9) {
            listeUtilisateurs[nbUtilisateurs++] = u;
        } else if (nb == 8) {
            /* Format V1.0 sans nbEchecs */
            u.nbEchecs = 0;
            listeUtilisateurs[nbUtilisateurs++] = u;
        } else {
            break;
        }
        if (nbUtilisateurs >= MAX_UTILISATEURS) break;
    }
    fclose(f);

    /* Verifier qu'il y a bien un admin dans la liste */
    int adminTrouve = 0;
    int i;
    for (i = 0; i < nbUtilisateurs; i++) {
        if (listeUtilisateurs[i].role == ROLE_ADMINISTRATEUR) {
            adminTrouve = 1;
            break;
        }
    }

    if (!adminTrouve && nbUtilisateurs > 0) {
        printf("[AVERTISSEMENT] Aucun admin dans le fichier.\n");
        printf("                Ajout de l'admin par defaut.\n");
        goto ajouter_admin;
    }

    if (nbUtilisateurs > 0) return; /* Tout va bien */

    /* Fichier vide */
    creer_admin:
    {
        FILE *tmp = fopen(FICHIER_UTILISATEURS, "w");
        if (tmp) fclose(tmp);
    }

    ajouter_admin:
    {
        Utilisateur admin;
        memset(&admin, 0, sizeof(Utilisateur));
        admin.id       = 1;
        admin.nbEchecs = 0;
        admin.actif    = 1;
        admin.role     = ROLE_ADMINISTRATEUR;
        strcpy(admin.nom,        "Admin");
        strcpy(admin.prenom,     "Super");
        strcpy(admin.email,      "admin@habitatcam.cm");
        strcpy(admin.telephone,  "000000000");
        strcpy(admin.motDePasse, "admin123");
        listeUtilisateurs[nbUtilisateurs++] = admin;
        sauvegarderUtilisateurs();
        printf("[INFO] Admin cree : admin@habitatcam.cm / admin123\n");
        printf("[INFO] Pensez a changer email et mot de passe !\n");
    }
}

/**
 * @brief Sauvegarde tous les utilisateurs dans data/utilisateurs.txt.
 * Format : id|nom|prenom|email|tel|mdp|role|nbEchecs|actif
 */
void sauvegarderUtilisateurs() {
    int i;
    creerDossierData();
    FILE *f = fopen(FICHIER_UTILISATEURS, "w");
    if (f == NULL) {
        printf("[ERREUR] Impossible de sauvegarder les utilisateurs.\n");
        return;
    }
    for (i = 0; i < nbUtilisateurs; i++) {
        Utilisateur *u = &listeUtilisateurs[i];
        fprintf(f, "%d|%s|%s|%s|%s|%s|%d|%d|%d\n",
                u->id, u->nom, u->prenom, u->email, u->telephone,
                u->motDePasse, (int)u->role, u->nbEchecs, u->actif);
    }
    fclose(f);
}


/* ============================================================
 * INSCRIPTION
 * ============================================================ */

/**
 * @brief Inscrit un nouveau Locataire ou Bailleur.
 *
 * L'admin ne s'inscrit pas ici — il passe par l'option 3
 * du menu principal (Acces restreint).
 *
 * @return AUTH_OK, AUTH_ERR_EXISTS, AUTH_ERR_IO ou AUTH_ERR_INVALID.
 */
AuthStatus inscrireUtilisateur() {
    Utilisateur nouveau;
    char        confirmation[TAILLE_MDP];
    int         roleChoix;

    if (nbUtilisateurs >= MAX_UTILISATEURS) {
        printf("[ERREUR] Nombre maximum d'utilisateurs atteint.\n");
        return AUTH_ERR_IO;
    }

    printf("\n[INSCRIPTION]\n");
    printf("------------------------------\n");

    lire_ligne("Nom        : ", nouveau.nom,       TAILLE_NOM);
    lire_ligne("Prenom     : ", nouveau.prenom,    TAILLE_NOM);
    lire_ligne("Email      : ", nouveau.email,     TAILLE_EMAIL);

    if (emailExiste(nouveau.email)) {
        afficherErreurAuth(AUTH_ERR_EXISTS);
        return AUTH_ERR_EXISTS;
    }

    lire_ligne("Telephone  : ", nouveau.telephone, TAILLE_TEL);

    do {
        saisirMotDePasse("Mot de passe : ", nouveau.motDePasse, TAILLE_MDP);
        saisirMotDePasse("Confirmation : ", confirmation,       TAILLE_MDP);
        if (strcmp(nouveau.motDePasse, confirmation) != 0)
            printf("[ERREUR] Les mots de passe ne correspondent pas.\n");
    } while (strcmp(nouveau.motDePasse, confirmation) != 0);

    /* Apres saisirMotDePasse, vider stdin pour la prochaine saisie */
    viderStdin();

    printf("\nVotre role :\n");
    printf("1. Locataire\n");
    printf("2. Bailleur\n");
    printf("Choix : ");
    roleChoix = saisirEntier();

    if      (roleChoix == 1) nouveau.role = ROLE_LOCATAIRE;
    else if (roleChoix == 2) nouveau.role = ROLE_BAILLEUR;
    else {
        printf("[ERREUR] Choix invalide. Choisir 1 ou 2.\n");
        return AUTH_ERR_INVALID;
    }

    nouveau.id       = genererIdUtilisateur();
    nouveau.nbEchecs = 0;
    nouveau.actif    = 1;

    listeUtilisateurs[nbUtilisateurs++] = nouveau;
    sauvegarderUtilisateurs();
    printf("[OK] Inscription reussie ! Bienvenue, %s.\n", nouveau.prenom);
    return AUTH_OK;
}


/* ============================================================
 * CONNEXION
 * ============================================================ */

/**
 * @brief Connecte un Locataire ou Bailleur (3 tentatives max).
 *
 * Apres 3 echecs : compte verrouille et option mot de passe
 * oublie proposee automatiquement.
 */
AuthStatus connecterUtilisateur() {
    char email[TAILLE_EMAIL];
    char mdp[TAILLE_MDP];
    int  tentatives = 0;
    int  index;

    printf("\n[CONNEXION]\n");
    printf("------------------------------\n");

    lire_ligne("Email      : ", email, TAILLE_EMAIL);
    index = rechercherUtilisateurParEmail(email);
    if (index == -1) {
        afficherErreurAuth(AUTH_ERR_NOTFOUND);
        return AUTH_ERR_NOTFOUND;
    }

    if (listeUtilisateurs[index].actif == 0) {
        printf("[ERREUR] Ce compte est verrouille.\n");
        printf("         Contactez l'administrateur.\n");
        return AUTH_ERR_INVALID;
    }

    while (tentatives < 3) {
        saisirMotDePasse("Mot de passe : ", mdp, TAILLE_MDP);

        if (strcmp(listeUtilisateurs[index].motDePasse, mdp) == 0) {
            listeUtilisateurs[index].nbEchecs = 0;
            sauvegarderUtilisateurs();
            sessionCourante.connecte    = 1;
            sessionCourante.utilisateur = listeUtilisateurs[index];
            printf("[OK] Bienvenue, %s %s !\n",
                   sessionCourante.utilisateur.prenom,
                   sessionCourante.utilisateur.nom);
            return AUTH_OK;
        }

        tentatives++;
        listeUtilisateurs[index].nbEchecs++;
        sauvegarderUtilisateurs();
        printf("[ERREUR] Mot de passe incorrect. (%d/3)\n", tentatives);
    }

    listeUtilisateurs[index].actif = 0;
    sauvegarderUtilisateurs();
    printf("\n[SECURITE] Compte verrouille apres 3 echecs.\n");

    /* Vider stdin avant motDePasseOublie qui utilise saisirEntier */
    viderStdin();
    return motDePasseOublie(email);
}


/* ============================================================
 * DECONNEXION
 * ============================================================ */

void deconnecterUtilisateur() {
    printf("[OK] Au revoir, %s !\n", sessionCourante.utilisateur.prenom);
    sessionCourante.connecte = 0;
    memset(&sessionCourante.utilisateur, 0, sizeof(Utilisateur));
}


/* ============================================================
 * MENU PRINCIPAL
 * ============================================================ */

/* Prototype anticipe — authentifierAdmin est definie plus bas */
static AuthStatus authentifierAdmin();

/**
 * @brief Menu de bienvenue — boucle jusqu'a connexion ou quitter.
 *
 * Option 3 "Acces restreint" est intentionnellement vague
 * pour ne pas signaler l'existence d'un acces admin.
 */
void menuAuthentification() {
    int choix;
    do {
        printf("\n=== HABITATCAM v2.0 ===\n");
        printf("1. Se connecter\n");
        printf("2. Creer un compte\n");
        printf("3. Acces restreint\n");
        printf("0. Quitter\n");
        printf("Choix : ");
        choix = saisirEntier();

        if (choix == -99) {
            printf("[ERREUR] Entrez 0, 1, 2 ou 3.\n");
            continue;
        }

        switch (choix) {
            case 1: connecterUtilisateur(); break;
            case 2: inscrireUtilisateur();  break;
            case 3: authentifierAdmin();    break;
            case 0: printf("Au revoir !\n"); break;
            default:
                printf("[ERREUR] Choix invalide.\n");
        }
    } while (choix != 0 && sessionCourante.connecte == 0);
}


/* ============================================================
 * ACCES ADMINISTRATEUR — 3 NIVEAUX DE SECURITE
 * ============================================================ */

/**
 * @brief Lit le code secret depuis data/admin_config.txt.
 * Cree le fichier avec "HABITAT2025" s'il est absent.
 */
static void lireCodeSecret(char *codeSecret, int taille) {
    FILE *f = fopen("data/admin_config.txt", "r");

    if (f == NULL) {
        creerDossierData();
        f = fopen("data/admin_config.txt", "w");
        if (f) { fprintf(f, "CODE_SECRET=HABITAT2025\n"); fclose(f); }
        strncpy(codeSecret, "HABITAT2025", taille - 1);
        codeSecret[taille - 1] = '\0';
        return;
    }

    char ligne[128];
    while (fgets(ligne, sizeof(ligne), f)) {
        ligne[strcspn(ligne, "\n\r")] = '\0';
        if (strncmp(ligne, "CODE_SECRET=", 12) == 0) {
            strncpy(codeSecret, ligne + 12, taille - 1);
            codeSecret[taille - 1] = '\0';
            fclose(f);
            return;
        }
    }
    strncpy(codeSecret, "HABITAT2025", taille - 1);
    codeSecret[taille - 1] = '\0';
    fclose(f);
}

/**
 * @brief Authentifie l'admin avec 3 niveaux de verification.
 *
 * Niveau 1 : Code secret (data/admin_config.txt) — 3 tentatives
 * Niveau 2 : Email du compte administrateur
 * Niveau 3 : Mot de passe — 3 tentatives
 *
 * CORRECTION CLÉ : viderStdin() est appele entre chaque
 * saisirMotDePasse() et lire_ligne() pour eviter que le \n
 * residuel de _getch() ne fasse sauter la saisie suivante.
 */
static AuthStatus authentifierAdmin() {
    char codeSecret[64], codeSaisi[64];
    char email[TAILLE_EMAIL];
    char mdp[TAILLE_MDP];
    int  tentatives, codeOk, index;

    printf("\n[ACCES ADMINISTRATEUR]\n");
    printf("------------------------------\n");

    /* Niveau 1 : code secret */
    lireCodeSecret(codeSecret, sizeof(codeSecret));
    tentatives = 0;
    codeOk     = 0;

    while (tentatives < 3) {
        saisirMotDePasse("Code secret  : ", codeSaisi, sizeof(codeSaisi));

        if (strcmp(codeSaisi, codeSecret) == 0) {
            codeOk = 1;
            break;
        }
        tentatives++;
        printf("[ERREUR] Code incorrect. (%d/3)\n", tentatives);
    }

    if (!codeOk) {
        printf("[SECURITE] Acces refuse.\n");
        return AUTH_ERR_INVALID;
    }

    /* IMPORTANT : vider stdin apres saisirMotDePasse
     * avant d'appeler lire_ligne pour l'email.
     * Sans ca, lire_ligne lirait un \n vide et l'email
     * serait une chaine vide -> "Aucun compte trouve". */
    viderStdin();

    /* Niveau 2 : email admin */
    lire_ligne("Email admin  : ", email, TAILLE_EMAIL);

    index = rechercherUtilisateurParEmail(email);
    if (index == -1) {
        afficherErreurAuth(AUTH_ERR_NOTFOUND);
        return AUTH_ERR_NOTFOUND;
    }
    if (listeUtilisateurs[index].role != ROLE_ADMINISTRATEUR) {
        printf("[ERREUR] Ce compte n'est pas un administrateur.\n");
        return AUTH_ERR_INVALID;
    }
    if (listeUtilisateurs[index].actif == 0) {
        printf("[ERREUR] Ce compte est verrouille.\n");
        return AUTH_ERR_INVALID;
    }

    /* Niveau 3 : mot de passe */
    tentatives = 0;
    while (tentatives < 3) {
        saisirMotDePasse("Mot de passe : ", mdp, TAILLE_MDP);

        if (strcmp(listeUtilisateurs[index].motDePasse, mdp) == 0) {
            listeUtilisateurs[index].nbEchecs = 0;
            sauvegarderUtilisateurs();
            sessionCourante.connecte    = 1;
            sessionCourante.utilisateur = listeUtilisateurs[index];
            printf("[OK] Bienvenue, %s ! Acces admin accorde.\n",
                   sessionCourante.utilisateur.prenom);
            return AUTH_OK;
        }
        tentatives++;
        listeUtilisateurs[index].nbEchecs++;
        sauvegarderUtilisateurs();
        printf("[ERREUR] Mot de passe incorrect. (%d/3)\n", tentatives);
    }

    listeUtilisateurs[index].actif = 0;
    sauvegarderUtilisateurs();
    printf("[SECURITE] Compte admin verrouille.\n");
    return AUTH_ERR_INVALID;
}

/**
 * @brief Modifie le code secret dans data/admin_config.txt.
 * Verifie l'ancien code avant d'autoriser le changement.
 */
AuthStatus modifierCodeSecret() {
    char ancien[64], nouveau1[64], nouveau2[64], codeActuel[64];

    printf("\n[MODIFIER LE CODE SECRET]\n");
    printf("------------------------------\n");

    lireCodeSecret(codeActuel, sizeof(codeActuel));
    saisirMotDePasse("Ancien code  : ", ancien, sizeof(ancien));

    /* Vider stdin avant lire_ligne */
    viderStdin();

    if (strcmp(ancien, codeActuel) != 0) {
        printf("[ERREUR] Ancien code incorrect.\n");
        return AUTH_ERR_INVALID;
    }

    do {
        saisirMotDePasse("Nouveau code : ", nouveau1, sizeof(nouveau1));
        saisirMotDePasse("Confirmation : ", nouveau2, sizeof(nouveau2));
        if (strcmp(nouveau1, nouveau2) != 0)
            printf("[ERREUR] Les codes ne correspondent pas.\n");
    } while (strcmp(nouveau1, nouveau2) != 0);

    creerDossierData();
    FILE *f = fopen("data/admin_config.txt", "w");
    if (!f) {
        printf("[ERREUR] Impossible d'ecrire dans admin_config.txt.\n");
        return AUTH_ERR_IO;
    }
    fprintf(f, "CODE_SECRET=%s\n", nouveau1);
    fclose(f);

    printf("[OK] Code secret mis a jour.\n");
    return AUTH_OK;
}

/**
 * @brief Permet a l'admin de changer son email de connexion.
 *
 * Fonctionnalite V2.0 : l'admin peut modifier l'email par
 * defaut (admin@habitatcam.cm) depuis son panneau.
 * Verifie que le nouvel email n'est pas deja utilise.
 *
 * @return AUTH_OK si succes, AUTH_ERR_EXISTS si email pris.
 */
AuthStatus changerEmailAdmin() {
    char nouvelEmail[TAILLE_EMAIL];
    char confirmation[TAILLE_EMAIL];

    printf("\n[CHANGER EMAIL ADMINISTRATEUR]\n");
    printf("------------------------------\n");
    printf("Email actuel : %s\n\n", sessionCourante.utilisateur.email);

    lire_ligne("Nouvel email     : ", nouvelEmail,   TAILLE_EMAIL);
    lire_ligne("Confirmer email  : ", confirmation,  TAILLE_EMAIL);

    if (strcmp(nouvelEmail, confirmation) != 0) {
        printf("[ERREUR] Les emails ne correspondent pas.\n");
        return AUTH_ERR_INVALID;
    }

    /* Verifier que le nouvel email n'est pas deja pris */
    if (emailExiste(nouvelEmail) &&
        strcmp(nouvelEmail, sessionCourante.utilisateur.email) != 0) {
        afficherErreurAuth(AUTH_ERR_EXISTS);
        return AUTH_ERR_EXISTS;
    }

    /* Mettre a jour dans la liste */
    int index = rechercherUtilisateurParEmail(
                    sessionCourante.utilisateur.email);
    if (index == -1) {
        afficherErreurAuth(AUTH_ERR_NOTFOUND);
        return AUTH_ERR_NOTFOUND;
    }

    strncpy(listeUtilisateurs[index].email, nouvelEmail, TAILLE_EMAIL - 1);
    listeUtilisateurs[index].email[TAILLE_EMAIL - 1] = '\0';

    /* Mettre a jour la session courante */
    strncpy(sessionCourante.utilisateur.email, nouvelEmail, TAILLE_EMAIL - 1);
    sessionCourante.utilisateur.email[TAILLE_EMAIL - 1] = '\0';

    sauvegarderUtilisateurs();
    printf("[OK] Email change : %s\n", nouvelEmail);
    printf("     Utilisez ce nouvel email a la prochaine connexion.\n");
    return AUTH_OK;
}


/* ============================================================
 * MOT DE PASSE OUBLIE
 * ============================================================ */

/**
 * @brief Enregistre une demande de reinitialisation de mdp.
 *
 * Appelee apres 3 echecs de connexion. Enregistre la demande
 * dans data/demandes_reinit.txt. L'admin la traite depuis
 * son panneau (option 9) et attribue un mot de passe temporaire.
 */
AuthStatus motDePasseOublie(const char *email) {
    int choix;

    printf("\n[MOT DE PASSE OUBLIE]\n");
    printf("------------------------------\n");
    printf("Compte verrouille apres 3 tentatives.\n\n");
    printf("Envoyer une demande a l'administrateur ?\n");
    printf("Il vous donnera un mot de passe temporaire.\n\n");
    printf("Envoyer la demande ? (1=Oui / 0=Non) : ");
    choix = saisirEntier();

    if (choix != 1) {
        printf("Demande annulee.\n");
        return AUTH_ERR_INVALID;
    }

    creerDossierData();
    FILE *f = fopen("data/demandes_reinit.txt", "a");
    if (!f) {
        printf("[ERREUR] Impossible d'enregistrer la demande.\n");
        return AUTH_ERR_IO;
    }
    fprintf(f, "%s|EN_ATTENTE\n", email);
    fclose(f);

    printf("\n[OK] Demande enregistree.\n");
    printf("     Contactez l'admin pour recevoir un mot de passe\n");
    printf("     temporaire, puis changez-le via votre menu.\n");
    return AUTH_OK;
}


/* ============================================================
 * CHANGEMENT DE MOT DE PASSE
 * ============================================================ */

/**
 * @brief Permet a l'utilisateur connecte de changer son mdp.
 *
 * Verifie l'ancien mot de passe, saisit le nouveau avec
 * confirmation, met a jour la liste et la session.
 * Disponible pour tous les roles (options 5, 5, 10).
 */
AuthStatus changerMotDePasse() {
    char ancienMdp[TAILLE_MDP];
    char nouveauMdp[TAILLE_MDP];
    char confirmation[TAILLE_MDP];

    printf("\n[CHANGER MOT DE PASSE]\n");
    printf("------------------------------\n");

    int index = rechercherUtilisateurParEmail(
                    sessionCourante.utilisateur.email);
    if (index == -1) {
        afficherErreurAuth(AUTH_ERR_NOTFOUND);
        return AUTH_ERR_NOTFOUND;
    }

    saisirMotDePasse("Ancien mot de passe  : ", ancienMdp, TAILLE_MDP);

    if (strcmp(listeUtilisateurs[index].motDePasse, ancienMdp) != 0) {
        printf("[ERREUR] Ancien mot de passe incorrect.\n");
        return AUTH_ERR_INVALID;
    }

    do {
        saisirMotDePasse("Nouveau mot de passe : ", nouveauMdp,   TAILLE_MDP);
        saisirMotDePasse("Confirmation         : ", confirmation, TAILLE_MDP);
        if (strcmp(nouveauMdp, confirmation) != 0)
            printf("[ERREUR] Les mots de passe ne correspondent pas.\n");
    } while (strcmp(nouveauMdp, confirmation) != 0);

    strncpy(listeUtilisateurs[index].motDePasse,
            nouveauMdp, TAILLE_MDP - 1);
    listeUtilisateurs[index].motDePasse[TAILLE_MDP - 1] = '\0';
    strncpy(sessionCourante.utilisateur.motDePasse,
            nouveauMdp, TAILLE_MDP - 1);
    sessionCourante.utilisateur.motDePasse[TAILLE_MDP - 1] = '\0';

    sauvegarderUtilisateurs();
    printf("[OK] Mot de passe change avec succes !\n");
    return AUTH_OK;
}


/* ============================================================
 * PERSONNALISATION DU PROFIL
 * ============================================================ */

/**
 * @brief Change le nom et le prenom de l'utilisateur connecte.
 *
 * Saisit le nouveau nom et prenom, met a jour la liste
 * et la session courante, puis sauvegarde.
 *
 * @return AUTH_OK si succes, AUTH_ERR_NOTFOUND si session corrompue.
 */
static AuthStatus changerNomPrenom() {
    char nouveauNom[TAILLE_NOM];
    char nouveauPrenom[TAILLE_NOM];

    printf("\n[CHANGER NOM ET PRENOM]\n");
    printf("------------------------------\n");
    printf("Nom actuel    : %s\n", sessionCourante.utilisateur.nom);
    printf("Prenom actuel : %s\n\n", sessionCourante.utilisateur.prenom);

    lire_ligne("Nouveau nom    : ", nouveauNom,    TAILLE_NOM);
    lire_ligne("Nouveau prenom : ", nouveauPrenom, TAILLE_NOM);

    int index = rechercherUtilisateurParEmail(
                    sessionCourante.utilisateur.email);
    if (index == -1) {
        afficherErreurAuth(AUTH_ERR_NOTFOUND);
        return AUTH_ERR_NOTFOUND;
    }

    /* Mettre a jour dans la liste */
    strncpy(listeUtilisateurs[index].nom,    nouveauNom,    TAILLE_NOM - 1);
    strncpy(listeUtilisateurs[index].prenom, nouveauPrenom, TAILLE_NOM - 1);
    listeUtilisateurs[index].nom[TAILLE_NOM - 1]    = '\0';
    listeUtilisateurs[index].prenom[TAILLE_NOM - 1] = '\0';

    /* Mettre a jour la session courante */
    strncpy(sessionCourante.utilisateur.nom,    nouveauNom,    TAILLE_NOM - 1);
    strncpy(sessionCourante.utilisateur.prenom, nouveauPrenom, TAILLE_NOM - 1);
    sessionCourante.utilisateur.nom[TAILLE_NOM - 1]    = '\0';
    sessionCourante.utilisateur.prenom[TAILLE_NOM - 1] = '\0';

    sauvegarderUtilisateurs();
    printf("[OK] Nom et prenom mis a jour : %s %s\n",
           nouveauPrenom, nouveauNom);
    return AUTH_OK;
}

/**
 * @brief Change le numero de telephone de l'utilisateur connecte.
 *
 * @return AUTH_OK si succes, AUTH_ERR_NOTFOUND si session corrompue.
 */
static AuthStatus changerTelephone() {
    char nouveauTel[TAILLE_TEL];

    printf("\n[CHANGER NUMERO DE TELEPHONE]\n");
    printf("------------------------------\n");
    printf("Numero actuel : %s\n\n", sessionCourante.utilisateur.telephone);

    lire_ligne("Nouveau numero : ", nouveauTel, TAILLE_TEL);

    int index = rechercherUtilisateurParEmail(
                    sessionCourante.utilisateur.email);
    if (index == -1) {
        afficherErreurAuth(AUTH_ERR_NOTFOUND);
        return AUTH_ERR_NOTFOUND;
    }

    strncpy(listeUtilisateurs[index].telephone, nouveauTel, TAILLE_TEL - 1);
    listeUtilisateurs[index].telephone[TAILLE_TEL - 1] = '\0';
    strncpy(sessionCourante.utilisateur.telephone, nouveauTel, TAILLE_TEL - 1);
    sessionCourante.utilisateur.telephone[TAILLE_TEL - 1] = '\0';

    sauvegarderUtilisateurs();
    printf("[OK] Numero mis a jour : %s\n", nouveauTel);
    return AUTH_OK;
}

/**
 * @brief Change l'email de connexion de l'utilisateur connecte.
 *
 * Verifie que le nouvel email n'est pas deja utilise
 * par un autre compte avant de mettre a jour.
 *
 * @return AUTH_OK si succes,
 *         AUTH_ERR_EXISTS si email deja pris,
 *         AUTH_ERR_INVALID si les emails ne correspondent pas.
 */
static AuthStatus changerEmail() {
    char nouvelEmail[TAILLE_EMAIL];
    char confirmation[TAILLE_EMAIL];

    printf("\n[CHANGER EMAIL DE CONNEXION]\n");
    printf("------------------------------\n");
    printf("Email actuel : %s\n\n", sessionCourante.utilisateur.email);

    lire_ligne("Nouvel email     : ", nouvelEmail,  TAILLE_EMAIL);
    lire_ligne("Confirmer email  : ", confirmation, TAILLE_EMAIL);

    if (strcmp(nouvelEmail, confirmation) != 0) {
        printf("[ERREUR] Les emails ne correspondent pas.\n");
        return AUTH_ERR_INVALID;
    }

    /* Verifier que le nouvel email n'est pas deja pris
     * (sauf si c'est le meme que l'actuel) */
    if (strcmp(nouvelEmail, sessionCourante.utilisateur.email) != 0 &&
        emailExiste(nouvelEmail)) {
        afficherErreurAuth(AUTH_ERR_EXISTS);
        return AUTH_ERR_EXISTS;
    }

    int index = rechercherUtilisateurParEmail(
                    sessionCourante.utilisateur.email);
    if (index == -1) {
        afficherErreurAuth(AUTH_ERR_NOTFOUND);
        return AUTH_ERR_NOTFOUND;
    }

    strncpy(listeUtilisateurs[index].email, nouvelEmail, TAILLE_EMAIL - 1);
    listeUtilisateurs[index].email[TAILLE_EMAIL - 1] = '\0';
    strncpy(sessionCourante.utilisateur.email, nouvelEmail, TAILLE_EMAIL - 1);
    sessionCourante.utilisateur.email[TAILLE_EMAIL - 1] = '\0';

    sauvegarderUtilisateurs();
    printf("[OK] Email mis a jour : %s\n", nouvelEmail);
    printf("     Utilisez ce nouvel email a la prochaine connexion.\n");
    return AUTH_OK;
}

/**
 * @brief Affiche le menu de personnalisation du profil.
 *
 * Regroupe toutes les operations de modification du profil
 * en un seul sous-menu accessible depuis n'importe quel role.
 * Pour l'admin, les options "email" et "code secret" sont
 * ajoutees automatiquement.
 *
 * Options communes (Locataire, Bailleur, Admin) :
 *   1. Changer nom et prenom
 *   2. Changer numero de telephone
 *   3. Changer mot de passe
 *   4. Changer email de connexion
 *
 * Options supplementaires (Admin uniquement) :
 *   5. Modifier le code secret
 */
void menuPersonnalisation() {
    int choix;
    int estAdmin = (sessionCourante.utilisateur.role == ROLE_ADMINISTRATEUR);

    do {
        printf("\n=== PERSONNALISATION DU PROFIL ===\n");
        printf("Compte : %s %s\n",
               sessionCourante.utilisateur.prenom,
               sessionCourante.utilisateur.nom);
        printf("Email  : %s\n", sessionCourante.utilisateur.email);
        printf("------------------------------\n");
        printf("1. Changer nom et prenom\n");
        printf("2. Changer numero de telephone\n");
        printf("3. Changer mot de passe\n");
        printf("4. Changer email de connexion\n");
        if (estAdmin) {
            printf("5. Modifier le code secret admin\n");
        }
        printf("0. Retour\n");
        printf("------------------------------\n");
        printf("Choix : ");
        choix = saisirEntier();

        if (choix == -99) {
            printf("[ERREUR] Saisie invalide.\n");
            continue;
        }

        switch (choix) {
            case 1: changerNomPrenom();  break;
            case 2: changerTelephone();  break;
            case 3: changerMotDePasse(); break;
            case 4: changerEmail();      break;
            case 5:
                if (estAdmin) modifierCodeSecret();
                else printf("[ERREUR] Choix invalide.\n");
                break;
            case 0: break;
            default:
                printf("[ERREUR] Choix invalide.\n");
        }
    } while (choix != 0);
}
