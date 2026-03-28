/* ============================================================
 * @file    auth.c
 * @brief   Module d'authentification de HabitatCam V2.0.
 *
 * Ce fichier gere :
 *   - L'inscription des locataires et bailleurs
 *   - La connexion avec verrouillage apres 3 echecs
 *   - La deconnexion
 *   - L'acces administrateur securise (3 niveaux)
 *   - Le changement de mot de passe
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
 * _getch() lit une touche sans l'afficher et sans attendre
 * la touche Entree. Sur Windows elle existe dans conio.h.
 * Sur Linux on la recrée avec termios.h.
 * Le compilateur choisit automatiquement le bon bloc.
 * ============================================================ */
#ifdef _WIN32
#include <conio.h>

/* Creer le dossier data/ si absent — version Windows */
static void creerDossierData()
{
    system("if not exist data mkdir data");
}

#else
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

/**
 * @brief Lit une touche sans l'afficher (version Linux/Mac).
 *
 * Passe le terminal en mode "raw" : desactive l'attente
 * de la touche Entree (ICANON) et l'echo automatique (ECHO).
 * Lit 1 seul caractere puis remet le terminal en mode normal.
 *
 * @return Code ASCII de la touche appuyee.
 */
static int _getch()
{
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

/* Creer le dossier data/ si absent — version Linux */
static void creerDossierData()
{
    mkdir("data", 0755);
}

#endif

#include "structures.h"
#include "auth.h"


/* ============================================================
 * VARIABLES GLOBALES
 * ============================================================ */

/** @brief Tableau de tous les utilisateurs charges en memoire. */
Utilisateur listeUtilisateurs[MAX_UTILISATEURS];

/** @brief Nombre d'utilisateurs actuellement enregistres. */
int nbUtilisateurs = 0;

/** @brief Session de l'utilisateur actuellement connecte. */
Session sessionCourante = {0};


/* ============================================================
 * UTILITAIRES — SAISIE CLAVIER
 * ============================================================ */

/**
 * @brief Lit une ligne de texte depuis le clavier.
 *
 * Plus sure que scanf() : lit toute la ligne d'un coup et
 * supprime automatiquement les caracteres \n et \r qui
 * causent des bugs sur Windows.
 *
 * Exemple d'utilisation :
 *   lire_ligne("Votre nom : ", nom, sizeof(nom));
 *
 * @param invite  Message affiche avant la saisie.
 * @param buffer  Tableau ou stocker ce que l'utilisateur tape.
 * @param taille  Taille maximale du tableau.
 */
static void lire_ligne(const char *invite, char *buffer, int taille)
{
    printf("%s", invite);
    if (fgets(buffer, taille, stdin))
        buffer[strcspn(buffer, "\n\r")] = '\0';
}

/**
 * @brief Lit un nombre entier depuis le clavier.
 *
 * Si l'utilisateur tape une lettre ou un symbole invalide,
 * retourne -99 au lieu de planter le programme.
 *
 * Exemple :
 *   int choix = saisirEntier();
 *   if (choix == -99) printf("Saisie invalide\n");
 *
 * @return Le nombre saisi, ou -99 si saisie invalide.
 */
static int saisirEntier()
{
    char buffer[32];
    int  val;
    lire_ligne("", buffer, sizeof(buffer));
    if (sscanf(buffer, "%d", &val) != 1)
        return -99;
    return val;
}

/**
 * @brief Efface et reaffiche la ligne de saisie du mot de passe.
 *
 * Utilise \r pour revenir en debut de ligne, efface avec des
 * espaces, puis reaffiche le libelle + contenu dans le bon mode.
 *
 * En mode cache  : affiche des etoiles (****)
 * En mode visible : affiche le texte en clair
 *
 * @param libelle Le texte avant la saisie (ex: "Mot de passe : ").
 * @param mdp     Le mot de passe saisi jusqu'ici.
 * @param len     Nombre de caracteres deja saisis.
 * @param visible 1 = afficher en clair, 0 = afficher en etoiles.
 */
static void afficherLigneMdp(const char *libelle,
                             const char *mdp,
                             int len, int visible)
{
    int i;
    /* Revenir au debut de la ligne et tout effacer */
    printf("\r%s                                        \r%s",
           libelle, libelle);

    if (visible)
    {
        /* Mode visible : afficher le mot de passe en clair
         * + indicateur pour guider l'utilisateur */
        printf("[TAB=cacher] %s", mdp);
    }
    else
    {
        /* Mode cache : remplacer chaque caractere par une etoile
         * + indicateur pour informer l'utilisateur */
        printf("[TAB=voir  ] ");
        for (i = 0; i < len; i++) putchar('*');
    }
    fflush(stdout);
}

/**
 * @brief Saisit un mot de passe caractere par caractere.
 *
 * Utilise _getch() pour lire chaque touche sans l'afficher.
 * La touche TAB bascule entre mode cache (****) et visible.
 *
 * Pourquoi TAB et pas V ?
 *   La lettre V peut faire partie d'un mot de passe.
 *   TAB (code 9) ne peut jamais etre dans un mot de passe
 *   donc aucun risque de conflit.
 *
 * Comportement automatique de securite :
 *   - Si l'utilisateur active le mode visible pour verifier,
 *     puis recommence a taper, le mode repasse en cache auto.
 *   - Si l'utilisateur valide en mode visible, le mode repasse
 *     en cache avant de continuer.
 *
 * Touches reconnues :
 *   TAB       -> bascule visible / cache
 *   Backspace -> efface le dernier caractere
 *   Entree    -> valide la saisie
 *   Autres    -> ajoute le caractere au mot de passe
 *
 * Affichage exemple :
 *   Mot de passe : [TAB=voir  ] ****
 *   (apres TAB)
 *   Mot de passe : [TAB=cacher] test
 *   (apres avoir retape)
 *   Mot de passe : [TAB=voir  ] *****
 *
 * @param libelle Texte affiche avant la zone de saisie.
 * @param mdp     Tableau ou stocker le mot de passe saisi.
 * @param taille  Taille maximale du tableau.
 */
static void saisirMotDePasse(const char *libelle, char *mdp, int taille)
{
    int len     = 0;
    int visible = 0;
    int c;

    mdp[0] = '\0';

    /* Afficher le libelle et l'etat initial */
    printf("%s", libelle);
    afficherLigneMdp(libelle, mdp, len, visible);

    while (1)
    {
        c = _getch();

#ifdef _WIN32
        /* Sur Windows, les touches speciales (fleches, F1-F12...)
         * envoient d'abord 0 ou 0xE0 puis un 2eme code.
         * On ignore ces sequences pour eviter des bugs. */
        if (c == 0 || c == 0xE0)
        {
            _getch(); /* lire et ignorer le 2eme code */
            continue;
        }
#endif

        /* ── Entree (\r Windows ou \n Linux) : fin de saisie ── */
        if (c == '\r' || c == '\n')
        {
            /* Securite : si on etait en mode visible,
             * repasser en cache avant de continuer */
            if (visible)
            {
                visible = 0;
                afficherLigneMdp(libelle, mdp, len, visible);
            }
            printf("\n");
            break;
        }

        /* ── TAB (code 9) : basculer visible / cache ── */
        if (c == 9)
        {
            visible = !visible;
            afficherLigneMdp(libelle, mdp, len, visible);
            continue;
        }

        /* ── Backspace (8 sur Windows, 127 sur Linux) ── */
        if ((c == 8 || c == 127) && len > 0)
        {
            len--;
            mdp[len] = '\0';
            afficherLigneMdp(libelle, mdp, len, visible);
            continue;
        }

        /* ── Caractere normal (codes 32 a 126) ── */
        if (c >= 32 && c <= 126 && len < taille - 1)
        {
            /* Si on etait en mode visible, repasser en cache
             * automatiquement des que l'utilisateur recommence
             * a taper — le mot de passe ne reste pas visible */
            if (visible)
                visible = 0;

            mdp[len++] = (char)c;
            mdp[len]   = '\0';
            afficherLigneMdp(libelle, mdp, len, visible);
        }
    }
}


/* ============================================================
 * UTILITAIRES — MESSAGES ET RECHERCHE
 * ============================================================ */

/**
 * @brief Affiche un message d'erreur precis selon le code retour.
 *
 * Au lieu d'afficher toujours le meme "[ERREUR]", on explique
 * exactement ce qui s'est passe. Inspire du module du prof.
 *
 * @param st Le code AuthStatus a interpreter.
 */
void afficherErreurAuth(AuthStatus st)
{
    switch (st)
    {
    case AUTH_ERR_NOTFOUND:
        printf("[ERREUR] Aucun compte trouve avec cet email.\n");
        break;
    case AUTH_ERR_INVALID:
        printf("[ERREUR] Mot de passe incorrect ou compte desactive.\n");
        break;
    case AUTH_ERR_EXISTS:
        printf("[ERREUR] Cet email est deja utilise par un autre compte.\n");
        break;
    case AUTH_ERR_IO:
        printf("[ERREUR] Probleme d'acces au fichier utilisateurs.\n");
        break;
    default:
        printf("[ERREUR] Une erreur inconnue s'est produite.\n");
    }
}

/**
 * @brief Verifie si un email est deja utilise par un compte.
 *
 * @param email Email a verifier.
 * @return 1 si l'email existe deja, 0 sinon.
 */
int emailExiste(const char *email)
{
    int i;
    for (i = 0; i < nbUtilisateurs; i++)
    {
        if (strcmp(listeUtilisateurs[i].email, email) == 0)
            return 1;
    }
    return 0;
}

/**
 * @brief Recherche un utilisateur par son email uniquement.
 *
 * On cherche d'abord par email (etape 1), puis on verifie
 * le mot de passe separement (etape 2). Ca permet d'afficher
 * "email inconnu" ou "mauvais mot de passe" au lieu du
 * message generique "identifiants incorrects".
 *
 * @param email Email a rechercher.
 * @return Index dans listeUtilisateurs si trouve, -1 sinon.
 */
int rechercherUtilisateurParEmail(const char *email)
{
    int i;
    for (i = 0; i < nbUtilisateurs; i++)
    {
        if (strcmp(listeUtilisateurs[i].email, email) == 0)
            return i;
    }
    return -1;
}

/**
 * @brief Genere un nouvel ID unique pour un utilisateur.
 *
 * Trouve le plus grand ID existant et retourne ce nombre + 1.
 * Ainsi chaque nouveau compte a un ID different des autres.
 *
 * @return Nouvel ID unique.
 */
int genererIdUtilisateur()
{
    int maxId = 0, i;
    for (i = 0; i < nbUtilisateurs; i++)
    {
        if (listeUtilisateurs[i].id > maxId)
            maxId = listeUtilisateurs[i].id;
    }
    return maxId + 1;
}


/* ============================================================
 * PERSISTANCE — LECTURE ET ECRITURE DU FICHIER
 * ============================================================ */

/**
 * @brief Charge les utilisateurs depuis data/utilisateurs.txt.
 *
 * Lit le fichier ligne par ligne et remplit listeUtilisateurs[].
 * Si le fichier est absent (premier lancement du programme),
 * cree automatiquement un compte administrateur par defaut :
 *   Email        : admin@habitatcam.cm
 *   Mot de passe : admin123
 *
 * Format d'une ligne dans le fichier :
 *   id|nom|prenom|email|telephone|motDePasse|role|nbEchecs|actif
 *
 * @warning Appeler cette fonction au demarrage dans main.c
 *          avant d'utiliser toute autre fonction d'auth.
 */
void chargerUtilisateurs()
{
    FILE *f = fopen(FICHIER_UTILISATEURS, "r");
    if (f == NULL)
    {
        /* Premier lancement : creer l'admin par defaut */
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

        listeUtilisateurs[0] = admin;
        nbUtilisateurs       = 1;
        sauvegarderUtilisateurs();
        printf("[INFO] Premier lancement — Admin cree :\n");
        printf("       Email    : admin@habitatcam.cm\n");
        printf("       Mot de passe : admin123\n");
        printf("       Changez ce mot de passe apres connexion !\n");
        return;
    }

    /* Lire chaque ligne du fichier */
    nbUtilisateurs = 0;
    Utilisateur u;
    while (fscanf(f,
                  "%d|%49[^|]|%49[^|]|%79[^|]|%19[^|]|%49[^|]|%d|%d|%d\n",
                  &u.id, u.nom, u.prenom, u.email, u.telephone,
                  u.motDePasse, (int*)&u.role,
                  &u.nbEchecs, &u.actif) == 9)
    {
        listeUtilisateurs[nbUtilisateurs++] = u;
        if (nbUtilisateurs >= MAX_UTILISATEURS) break;
    }
    fclose(f);
}

/**
 * @brief Sauvegarde tous les utilisateurs dans data/utilisateurs.txt.
 *
 * Ecrase le fichier existant avec le contenu actuel du tableau
 * listeUtilisateurs[]. Cree le dossier data/ si absent.
 *
 * Format d'une ligne :
 *   id|nom|prenom|email|telephone|motDePasse|role|nbEchecs|actif
 *
 * @note Appeler apres toute modification de listeUtilisateurs[].
 */
void sauvegarderUtilisateurs()
{
    int  i;
    creerDossierData(); /* Cree data/ si absent — cross-platform */

    FILE *f = fopen(FICHIER_UTILISATEURS, "w");
    if (f == NULL)
    {
        printf("[ERREUR] Impossible de sauvegarder les utilisateurs.\n");
        return;
    }
    for (i = 0; i < nbUtilisateurs; i++)
    {
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
 * Demande toutes les informations necessaires, verifie que
 * l'email n'est pas deja utilise, confirme le mot de passe,
 * puis enregistre le compte.
 *
 * Note : Le role Administrateur n'est PAS disponible ici.
 * Pour se connecter en tant qu'admin, utiliser l'option 3
 * du menu principal (Acces restreint).
 *
 * @return AUTH_OK si inscription reussie,
 *         AUTH_ERR_EXISTS si email deja pris,
 *         AUTH_ERR_IO si nombre max d'utilisateurs atteint,
 *         AUTH_ERR_INVALID si choix de role invalide.
 */
AuthStatus inscrireUtilisateur()
{
    Utilisateur nouveau;
    char        confirmation[TAILLE_MDP];
    int         roleChoix;

    if (nbUtilisateurs >= MAX_UTILISATEURS)
    {
        printf("[ERREUR] Nombre maximum d'utilisateurs atteint.\n");
        return AUTH_ERR_IO;
    }

    printf("\n[INSCRIPTION]\n");
    printf("------------------------------\n");

    lire_ligne("Nom        : ", nouveau.nom,       TAILLE_NOM);
    lire_ligne("Prenom     : ", nouveau.prenom,    TAILLE_NOM);
    lire_ligne("Email      : ", nouveau.email,     TAILLE_EMAIL);

    if (emailExiste(nouveau.email))
    {
        afficherErreurAuth(AUTH_ERR_EXISTS);
        return AUTH_ERR_EXISTS;
    }

    lire_ligne("Telephone  : ", nouveau.telephone, TAILLE_TEL);

    /* Saisie du mot de passe avec confirmation */
    do
    {
        saisirMotDePasse("Mot de passe : ", nouveau.motDePasse, TAILLE_MDP);
        saisirMotDePasse("Confirmation : ", confirmation,       TAILLE_MDP);
        if (strcmp(nouveau.motDePasse, confirmation) != 0)
            printf("[ERREUR] Les mots de passe ne correspondent pas.\n");
    }
    while (strcmp(nouveau.motDePasse, confirmation) != 0);

    /* Choix du role — Admin non disponible ici */
    printf("\nVotre role :\n");
    printf("1. Locataire\n");
    printf("2. Bailleur\n");
    printf("3. Administratur\n");
    printf("Choix : ");
    roleChoix = saisirEntier();

    if      (roleChoix == 1) nouveau.role = ROLE_LOCATAIRE;
    else if (roleChoix == 2) nouveau.role = ROLE_BAILLEUR;
    else
    {
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
 * @brief Connecte un Locataire ou Bailleur existant.
 *
 * Fonctionnement en 3 etapes :
 *   1. Chercher le compte par email
 *   2. Verifier que le compte est actif
 *   3. Verifier le mot de passe (3 tentatives max)
 *
 * Si 3 echecs consecutifs :
 *   - Le compte est verrouille automatiquement (actif = 0)
 *   - L'option "Mot de passe oublie" est proposee
 *   - Seul l'admin peut reactiver le compte
 *
 * Le compteur d'echecs (nbEchecs) est sauvegarde dans le
 * fichier — il persiste meme apres fermeture du programme.
 *
 * @return AUTH_OK si connexion reussie,
 *         AUTH_ERR_NOTFOUND si email inconnu,
 *         AUTH_ERR_INVALID si mot de passe incorrect ou inactif.
 */
AuthStatus connecterUtilisateur()
{
    char email[TAILLE_EMAIL];
    char mdp[TAILLE_MDP];
    int  tentatives = 0;

    printf("\n[CONNEXION]\n");
    printf("------------------------------\n");

    /* Etape 1 : recherche par email */
    lire_ligne("Email      : ", email, TAILLE_EMAIL);
    int index = rechercherUtilisateurParEmail(email);
    if (index == -1)
    {
        afficherErreurAuth(AUTH_ERR_NOTFOUND);
        return AUTH_ERR_NOTFOUND;
    }

    /* Etape 2 : compte actif ? */
    if (listeUtilisateurs[index].actif == 0)
    {
        printf("[ERREUR] Ce compte est verrouille.\n");
        printf("         Contactez l'administrateur pour le reactiver.\n");
        return AUTH_ERR_INVALID;
    }

    /* Etape 3 : verification du mot de passe */
    while (tentatives < 3)
    {
        saisirMotDePasse("Mot de passe : ", mdp, TAILLE_MDP);

        if (strcmp(listeUtilisateurs[index].motDePasse, mdp) == 0)
        {
            /* Succes : reinitialiser le compteur et ouvrir session */
            listeUtilisateurs[index].nbEchecs = 0;
            sauvegarderUtilisateurs();
            sessionCourante.connecte    = 1;
            sessionCourante.utilisateur = listeUtilisateurs[index];
            printf("[OK] Bienvenue, %s %s !\n",
                   sessionCourante.utilisateur.prenom,
                   sessionCourante.utilisateur.nom);
            return AUTH_OK;
        }

        /* Echec : incrementer le compteur */
        tentatives++;
        listeUtilisateurs[index].nbEchecs++;
        sauvegarderUtilisateurs();
        printf("[ERREUR] Mot de passe incorrect. (%d/3)\n", tentatives);
    }

    /* 3 echecs : verrouiller le compte */
    listeUtilisateurs[index].actif = 0;
    sauvegarderUtilisateurs();
    printf("\n[SECURITE] Compte verrouille apres 3 echecs.\n");
    printf("------------------------------\n");

    /* Proposer l'option mot de passe oublie */
    return motDePasseOublie(email);
}


/* ============================================================
 * DECONNEXION
 * ============================================================ */

/**
 * @brief Deconnecte l'utilisateur et reinitialise la session.
 *
 * Remet sessionCourante.connecte a 0 et efface les donnees
 * de l'utilisateur en session avec memset().
 */
void deconnecterUtilisateur()
{
    printf("[OK] Au revoir, %s !\n",
           sessionCourante.utilisateur.prenom);
    sessionCourante.connecte = 0;
    memset(&sessionCourante.utilisateur, 0, sizeof(Utilisateur));
}


/* ============================================================
 * MENU PRINCIPAL D'AUTHENTIFICATION
 * ============================================================ */

/* Declaration anticipee — authentifierAdmin est definie plus bas
 * mais utilisee ici dans le switch. En C, on doit declarer
 * une fonction avant de l'utiliser. */
static AuthStatus authentifierAdmin();

/**
 * @brief Affiche le menu de bienvenue et gere l'authentification.
 *
 * Boucle jusqu'a ce qu'un utilisateur soit connecte ou que
 * l'utilisateur choisisse de quitter (option 0).
 *
 * Options :
 *   1 -> Connexion standard (Locataire / Bailleur)
 *   2 -> Inscription d'un nouveau compte
 *   3 -> Acces restreint (Admin — affichage discret)
 *   0 -> Quitter l'application
 *
 * L'option 3 est intentionnellement vague ("Acces restreint")
 * pour ne pas signaler l'existence d'un acces admin.
 */
void menuAuthentification()
{
    int choix;
    do
    {
        printf("\n=== HABITATCAM v2.0 ===\n");
        printf("1. Se connecter\n");
        printf("2. Creer un compte\n");
        printf("3. Acces restreint\n");
        printf("0. Quitter\n");
        printf("Choix : ");
        choix = saisirEntier();

        if (choix == -99)
        {
            printf("[ERREUR] Entrez un chiffre valide (0, 1, 2 ou 3).\n");
            continue;
        }

        switch (choix)
        {
        case 1:
            connecterUtilisateur();
            break;
        case 2:
            inscrireUtilisateur();
            break;
        case 3:
            authentifierAdmin();
            break;
        case 0:
            printf("Au revoir !\n");
            break;
        default:
            printf("[ERREUR] Le choix %d n'existe pas.\n", choix);
            printf("         Choisir 0, 1, 2 ou 3.\n");
        }
    }
    while (choix != 0 && sessionCourante.connecte == 0);
}


/* ============================================================
 * ACCES ADMINISTRATEUR — AUTHENTIFICATION A 3 NIVEAUX
 * ============================================================ */

/**
 * @brief Lit le code secret admin depuis data/admin_config.txt.
 *
 * Ce fichier contient une seule ligne :
 *   CODE_SECRET=HABITAT2025
 *
 * Si le fichier est absent, il est cree avec le code par defaut
 * "HABITAT2025". L'admin doit le changer depuis son panneau.
 *
 * @param codeSecret Tableau ou ecrire le code lu.
 * @param taille     Taille du tableau.
 */
static void lireCodeSecret(char *codeSecret, int taille)
{
    FILE *f = fopen("data/admin_config.txt", "r");

    if (f == NULL)
    {
        /* Fichier absent : le creer avec la valeur par defaut */
        creerDossierData();
        f = fopen("data/admin_config.txt", "w");
        if (f)
        {
            fprintf(f, "CODE_SECRET=HABITAT2025\n");
            fclose(f);
        }
        strncpy(codeSecret, "HABITAT2025", taille - 1);
        codeSecret[taille - 1] = '\0';
        return;
    }

    /* Chercher la ligne "CODE_SECRET=valeur" */
    char ligne[128];
    while (fgets(ligne, sizeof(ligne), f))
    {
        ligne[strcspn(ligne, "\n\r")] = '\0';
        if (strncmp(ligne, "CODE_SECRET=", 12) == 0)
        {
            strncpy(codeSecret, ligne + 12, taille - 1);
            codeSecret[taille - 1] = '\0';
            fclose(f);
            return;
        }
    }

    /* Ligne non trouvee : utiliser la valeur par defaut */
    strncpy(codeSecret, "HABITAT2025", taille - 1);
    codeSecret[taille - 1] = '\0';
    fclose(f);
}

/**
 * @brief Authentifie un administrateur avec 3 niveaux de securite.
 *
 * Niveau 1 : Code secret (fichier data/admin_config.txt)
 *   -> Seul l'admin connait ce code — 3 tentatives max
 *
 * Niveau 2 : Email du compte administrateur
 *   -> Verifie que l'email existe ET est de role Admin
 *
 * Niveau 3 : Mot de passe avec toggle visibilite
 *   -> 3 tentatives max, compte verrouille apres
 *
 * Si un niveau echoue, on n'accede pas aux suivants.
 *
 * @return AUTH_OK si les 3 niveaux sont passes,
 *         AUTH_ERR_INVALID sinon.
 */
static AuthStatus authentifierAdmin()
{
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
    while (tentatives < 3)
    {
        saisirMotDePasse("Code secret  : ", codeSaisi, sizeof(codeSaisi));
        if (strcmp(codeSaisi, codeSecret) == 0)
        {
            codeOk = 1;
            break;
        }
        tentatives++;
        printf("[ERREUR] Code incorrect. (%d/3)\n", tentatives);
    }
    if (!codeOk)
    {
        printf("[SECURITE] Acces refuse.\n");
        return AUTH_ERR_INVALID;
    }

    /* Niveau 2 : email admin */
    lire_ligne("Email admin  : ", email, TAILLE_EMAIL);
    index = rechercherUtilisateurParEmail(email);
    if (index == -1)
    {
        afficherErreurAuth(AUTH_ERR_NOTFOUND);
        return AUTH_ERR_NOTFOUND;
    }
    if (listeUtilisateurs[index].role != ROLE_ADMINISTRATEUR)
    {
        printf("[ERREUR] Ce compte n'est pas un administrateur.\n");
        return AUTH_ERR_INVALID;
    }
    if (listeUtilisateurs[index].actif == 0)
    {
        printf("[ERREUR] Ce compte administrateur est verrouille.\n");
        return AUTH_ERR_INVALID;
    }

    /* Niveau 3 : mot de passe */
    tentatives = 0;
    while (tentatives < 3)
    {
        saisirMotDePasse("Mot de passe : ", mdp, TAILLE_MDP);
        if (strcmp(listeUtilisateurs[index].motDePasse, mdp) == 0)
        {
            listeUtilisateurs[index].nbEchecs = 0;
            sauvegarderUtilisateurs();
            sessionCourante.connecte    = 1;
            sessionCourante.utilisateur = listeUtilisateurs[index];
            printf("[OK] Bienvenue, %s ! Acces administrateur accorde.\n",
                   sessionCourante.utilisateur.prenom);
            return AUTH_OK;
        }
        tentatives++;
        listeUtilisateurs[index].nbEchecs++;
        sauvegarderUtilisateurs();
        printf("[ERREUR] Mot de passe incorrect. (%d/3)\n", tentatives);
    }

    /* 3 echecs : verrouiller le compte admin */
    listeUtilisateurs[index].actif = 0;
    sauvegarderUtilisateurs();
    printf("[SECURITE] Compte admin verrouille apres 3 echecs.\n");
    return AUTH_ERR_INVALID;
}

/**
 * @brief Modifie le code secret admin dans data/admin_config.txt.
 *
 * Verifie l'ancien code avant d'autoriser le changement.
 * Permet a l'admin de changer le code sans modifier le code
 * source ni recompiler l'application.
 *
 * Accessible depuis le panneau admin (option 8).
 *
 * @return AUTH_OK si mise a jour reussie,
 *         AUTH_ERR_INVALID si ancien code incorrect,
 *         AUTH_ERR_IO si probleme d'ecriture.
 */
AuthStatus modifierCodeSecret()
{
    char ancien[64], nouveau1[64], nouveau2[64], codeActuel[64];

    printf("\n[MODIFIER LE CODE SECRET]\n");
    printf("------------------------------\n");

    lireCodeSecret(codeActuel, sizeof(codeActuel));
    saisirMotDePasse("Ancien code  : ", ancien, sizeof(ancien));

    if (strcmp(ancien, codeActuel) != 0)
    {
        printf("[ERREUR] Ancien code incorrect.\n");
        return AUTH_ERR_INVALID;
    }

    do
    {
        saisirMotDePasse("Nouveau code : ", nouveau1, sizeof(nouveau1));
        saisirMotDePasse("Confirmation : ", nouveau2, sizeof(nouveau2));
        if (strcmp(nouveau1, nouveau2) != 0)
            printf("[ERREUR] Les codes ne correspondent pas.\n");
    }
    while (strcmp(nouveau1, nouveau2) != 0);

    creerDossierData();
    FILE *f = fopen("data/admin_config.txt", "w");
    if (!f)
    {
        printf("[ERREUR] Impossible d'ecrire dans admin_config.txt.\n");
        return AUTH_ERR_IO;
    }
    fprintf(f, "CODE_SECRET=%s\n", nouveau1);
    fclose(f);

    printf("[OK] Code secret mis a jour avec succes.\n");
    return AUTH_OK;
}


/* ============================================================
 * MOT DE PASSE OUBLIE
 * ============================================================ */

/**
 * @brief Enregistre une demande de reinitialisation de mot de passe.
 *
 * Appelee automatiquement apres 3 echecs de connexion.
 *
 * Pourquoi ne pas laisser l'utilisateur reinitialiser lui-meme ?
 *   C'est une faille de securite : n'importe qui connaissant
 *   le nom et le telephone pourrait changer le mot de passe.
 *   A la place, on enregistre la demande et l'admin attribue
 *   un mot de passe temporaire manuellement.
 *
 * Processus :
 *   1. L'utilisateur confirme qu'il veut envoyer une demande
 *   2. La demande est enregistree dans data/demandes_reinit.txt
 *   3. L'admin voit la demande dans son panneau (option 9)
 *   4. L'admin attribue un mot de passe temporaire
 *   5. L'utilisateur se connecte avec ce mot de passe temporaire
 *   6. L'utilisateur change son mot de passe via option du menu
 *
 * @note La notification en temps reel sera implementee
 *       dans notification.c (V2.0).
 *
 * @param email Email du compte concerne.
 * @return AUTH_OK si demande enregistree,
 *         AUTH_ERR_INVALID si l'utilisateur refuse,
 *         AUTH_ERR_IO si probleme fichier.
 */
AuthStatus motDePasseOublie(const char *email)
{
    int choix;

    printf("\n[MOT DE PASSE OUBLIE]\n");
    printf("------------------------------\n");
    printf("Votre compte a ete verrouille apres 3 tentatives.\n\n");
    printf("Vous pouvez envoyer une demande a l'administrateur.\n");
    printf("Il vous attribuera un mot de passe temporaire que\n");
    printf("vous changerez ensuite via [Changer mon mot de passe].\n\n");
    printf("Envoyer la demande ? (1=Oui / 0=Non) : ");
    choix = saisirEntier();

    if (choix != 1)
    {
        printf("Demande annulee.\n");
        return AUTH_ERR_INVALID;
    }

    creerDossierData();
    FILE *f = fopen("data/demandes_reinit.txt", "a");
    if (!f)
    {
        printf("[ERREUR] Impossible d'enregistrer la demande.\n");
        return AUTH_ERR_IO;
    }
    fprintf(f, "%s|EN_ATTENTE\n", email);
    fclose(f);

    printf("\n[OK] Demande enregistree.\n");
    printf("     L'administrateur va traiter votre demande.\n");
    printf("     Connectez-vous avec le mot de passe temporaire\n");
    printf("     qu'il vous communiquera, puis changez-le.\n");
    return AUTH_OK;
}


/* ============================================================
 * CHANGEMENT DE MOT DE PASSE
 * ============================================================ */

/**
 * @brief Permet a l'utilisateur connecte de changer son mot de passe.
 *
 * Inspire de auth_change_password() du module du prof :
 * separation claire entre saisie et verification.
 *
 * Etapes :
 *   1. Saisir l'ancien mot de passe — verification immediate.
 *      Si incorrect : echec, on ne continue pas.
 *   2. Saisir le nouveau mot de passe.
 *   3. Confirmer le nouveau mot de passe.
 *      Si les deux different : redemander jusqu'a OK.
 *   4. Mettre a jour et sauvegarder.
 *
 * La session courante est mise a jour immediatement apres
 * pour que l'utilisateur n'ait pas a se reconnecter.
 *
 * Disponible dans le menu de chaque role :
 *   Locataire      : option 5
 *   Bailleur       : option 5
 *   Administrateur : option 10
 *
 * @return AUTH_OK si changement reussi,
 *         AUTH_ERR_INVALID si ancien mot de passe incorrect,
 *         AUTH_ERR_NOTFOUND si session corrompue.
 */
AuthStatus changerMotDePasse()
{
    char ancienMdp[TAILLE_MDP];
    char nouveauMdp[TAILLE_MDP];
    char confirmation[TAILLE_MDP];

    printf("\n[CHANGER MOT DE PASSE]\n");
    printf("------------------------------\n");

    /* Trouver l'utilisateur connecte dans la liste */
    int index = rechercherUtilisateurParEmail(
                    sessionCourante.utilisateur.email);
    if (index == -1)
    {
        afficherErreurAuth(AUTH_ERR_NOTFOUND);
        return AUTH_ERR_NOTFOUND;
    }

    /* Etape 1 : verifier l'ancien mot de passe */
    saisirMotDePasse("Ancien mot de passe  : ", ancienMdp, TAILLE_MDP);
    if (strcmp(listeUtilisateurs[index].motDePasse, ancienMdp) != 0)
    {
        printf("[ERREUR] Ancien mot de passe incorrect.\n");
        return AUTH_ERR_INVALID;
    }

    /* Etapes 2 et 3 : nouveau mot de passe + confirmation */
    do
    {
        saisirMotDePasse("Nouveau mot de passe : ", nouveauMdp,    TAILLE_MDP);
        saisirMotDePasse("Confirmation         : ", confirmation,  TAILLE_MDP);
        if (strcmp(nouveauMdp, confirmation) != 0)
            printf("[ERREUR] Les mots de passe ne correspondent pas.\n");
    }
    while (strcmp(nouveauMdp, confirmation) != 0);

    /* Etape 4 : mettre a jour et sauvegarder */
    strncpy(listeUtilisateurs[index].motDePasse,
            nouveauMdp, TAILLE_MDP - 1);
    listeUtilisateurs[index].motDePasse[TAILLE_MDP - 1] = '\0';

    /* Mettre a jour aussi la session courante */
    strncpy(sessionCourante.utilisateur.motDePasse,
            nouveauMdp, TAILLE_MDP - 1);
    sessionCourante.utilisateur.motDePasse[TAILLE_MDP - 1] = '\0';

    sauvegarderUtilisateurs();
    printf("[OK] Mot de passe change avec succes !\n");
    return AUTH_OK;
}
