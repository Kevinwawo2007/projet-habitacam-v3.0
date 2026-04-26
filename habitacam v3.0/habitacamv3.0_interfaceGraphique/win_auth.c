/* ============================================================
 * @file    win_auth.c
 * @brief   Pages d authentification graphique — HabitatCam V3.0
 *
 * Gere toutes les pages de connexion, inscription et menus.
 * Utilise les fonctions de auth.c (V2.0) pour la logique.
 *
 * @version 3.0
 * @author  SOUOPGUI
 * ============================================================ */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "structures.h"
#include "auth.h"
#include "win_auth.h"

/* ============================================================
 * UTILITAIRE INTERNE — Sous-titre de section
 * ============================================================ */

/**
 * @brief Cree un label de section avec style discret.
 * @param txt   Texte du sous-titre.
 * @param x, y  Position.
 * @param w     Largeur.
 * @return HWND Handle du label cree.
 */
static HWND mkSectionLabel(const char *txt, int x, int y, int w) {
    return mkLabel(txt, x, y, w, 18, hFontNormal);
}

/* ============================================================
 * PAGE CONNEXION
 * ============================================================ */

/**
 * @brief Cree la page de connexion centree.
 *
 * Disposition :
 *   - Carte blanche centree (largeur 360px)
 *   - Logo texte HABITATCAM en grand
 *   - Champ Email avec label
 *   - Champ Mot de passe avec bouton oeil
 *   - Bouton principal Se connecter (vert)
 *   - Lien Creer un compte
 *   - Lien discret Acces restreint (admin)
 *
 * @return void
 */
void pageConnexion(void) {
    /* Centre de la fenetre */
    int cw = 360;                          /* largeur carte         */
    int cx = (FENETRE_LARGEUR - cw) / 2;  /* marge gauche          */
    int cy = BANDEAU_HAUTEUR + 30;        /* marge depuis bandeau  */
    HWND h;

    /* ── Logo ── */
    h = mkLabel("HABITATCAM", cx, cy, cw, 48, hFontTitre);
    ajout(ctrlConn, &nbConn, h);

    h = mkLabel("Plateforme de logement au Cameroun",
                cx, cy + 52, cw, 20, hFontNormal);
    ajout(ctrlConn, &nbConn, h);

    /* ── Separateur ── */
    h = mkLabel("", cx, cy + 82, cw, 1, hFontNormal);
    ajout(ctrlConn, &nbConn, h);

    /* ── Champ Email ── */
    h = mkSectionLabel("Adresse email", cx, cy + 100, cw);
    ajout(ctrlConn, &nbConn, h);
    h = mkEdit(ID_EMAIL_CONN, FALSE, cx, cy + 120, cw, 34);
    ajout(ctrlConn, &nbConn, h);

    /* ── Champ Mot de passe + bouton oeil ── */
    h = mkSectionLabel("Mot de passe", cx, cy + 170, cw);
    ajout(ctrlConn, &nbConn, h);
    h = mkEdit(ID_MDP_CONN, TRUE, cx, cy + 190, cw - 44, 34);
    ajout(ctrlConn, &nbConn, h);
    h = mkBtn("o", ID_BTN_VOIR_MDP,
              cx + cw - 40, cy + 190, 40, 34);
    ajout(ctrlConn, &nbConn, h);

    /* ── Bouton Se connecter ── */
    h = mkBtn("Se connecter", ID_BTN_CONN,
              cx, cy + 244, cw, 40);
    ajout(ctrlConn, &nbConn, h);

    /* ── Lien inscription ── */
    h = mkBtn("Pas de compte ?  Creer un compte",
              ID_BTN_ALLER_INSCR, cx, cy + 296, cw, 30);
    ajout(ctrlConn, &nbConn, h);

    /* NOTE SECURITE :
     * Il n y a pas de bouton admin visible.
     * L acces administrateur se fait uniquement via
     * le raccourci clavier secret : Ctrl + Shift + A
     * Ce raccourci est gere dans main_win32.c (WM_KEYDOWN).
     * Un utilisateur lambda ne peut pas savoir qu il existe. */
}

/* ============================================================
 * PAGE INSCRIPTION
 * ============================================================ */

/**
 * @brief Cree le formulaire d inscription.
 *
 * Grille a deux colonnes : label gauche, champ droite.
 * Combo de role en bas, deux boutons d action.
 *
 * @return void
 */
void pageInscription(void) {
    int lw  = 150;   /* largeur label  */
    int fw  = 280;   /* largeur champ  */
    int fh  = 30;    /* hauteur champ  */
    int gap = 42;    /* espacement     */
    /* Centrer la grille (lw + 8 + fw = 438) */
    int cx  = (FENETRE_LARGEUR - lw - 8 - fw) / 2;
    int cy  = BANDEAU_HAUTEUR + 20;
    HWND h;

    /* Titre */
    h = mkLabel("Creer un compte", cx, cy, lw + fw, 42, hFontTitre);
    ajout(ctrlInscr, &nbInscr, h);

    h = mkLabel("Remplissez tous les champs ci-dessous",
                cx, cy + 46, lw + fw, 18, hFontNormal);
    ajout(ctrlInscr, &nbInscr, h);

    /* Definition des lignes */
    struct { const char *lbl; int id; BOOL p; } C[] = {
        { "Nom",            ID_NOM,        FALSE },
        { "Prenom",         ID_PRENOM,     FALSE },
        { "Email",          ID_EMAIL_INSCR,FALSE },
        { "Telephone",      ID_TEL,        FALSE },
        { "Mot de passe",   ID_MDP_INSCR,  TRUE  },
        { "Confirmation",   ID_MDP_CONF,   TRUE  },
    };

    int i;
    for (i = 0; i < 6; i++) {
        int y = cy + 78 + i * gap;
        h = mkLabel(C[i].lbl, cx, y + 6, lw, 18, hFontNormal);
        ajout(ctrlInscr, &nbInscr, h);
        h = mkEdit(C[i].id, C[i].p, cx + lw + 8, y, fw, fh);
        ajout(ctrlInscr, &nbInscr, h);
    }

    /* Combo role */
    int yR = cy + 78 + 6 * gap;
    h = mkLabel("Role", cx, yR + 6, lw, 18, hFontNormal);
    ajout(ctrlInscr, &nbInscr, h);

    HWND cb = CreateWindow("COMBOBOX", NULL,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        cx + lw + 8, yR, fw, 120,
        hWnd, (HMENU)ID_COMBO_ROLE,
        GetModuleHandle(NULL), NULL);
    SendMessage(cb, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"Locataire");
    SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)"Bailleur");
    SendMessage(cb, CB_SETCURSEL, 0, 0);
    ajout(ctrlInscr, &nbInscr, cb);

    /* Boutons */
    int yB = yR + gap + 6;
    h = mkBtn("Creer mon compte", ID_BTN_CREER,
              cx + lw + 8, yB, fw, 36);
    ajout(ctrlInscr, &nbInscr, h);

    h = mkBtn("Retour connexion", ID_BTN_RETOUR_CONN,
              cx, yB, lw, 36);
    ajout(ctrlInscr, &nbInscr, h);
}

/* ============================================================
 * PAGES MENUS
 * ============================================================ */

/* Structure pour definir un bouton de menu.
 * Declaree en dehors de la fonction pour etre visible
 * par toutes les fonctions qui l utilisent. */
typedef struct {
    const char *t;  /* Texte du bouton  */
    int id;         /* ID du controle   */
    int col;        /* Colonne (0 ou 1) */
    int row;        /* Ligne            */
} BoutonMenu;

/**
 * @brief Construit un menu generique a deux colonnes.
 *
 * Factorisation : les trois menus (locataire, bailleur, admin)
 * ont la meme structure — titre + grille de boutons + deconnexion.
 *
 * @param titre    Titre affiche en haut du menu.
 * @param btns     Tableau de BoutonMenu.
 * @param nbBtns   Nombre de boutons dans le tableau.
 * @param idDeco   ID du bouton Se deconnecter.
 * @param tab      Tableau de handles de la page.
 * @param nb       Compteur du tableau.
 * @return void
 */
static void construireMenu(
    const char *titre,
    BoutonMenu *btns,
    int nbBtns, int idDeco,
    HWND *tab, int *nb)
{
    int bw  = 352;   /* largeur bouton  */
    int bh  = 46;    /* hauteur bouton  */
    int gap = 12;    /* espace boutons  */
    int cx  = (FENETRE_LARGEUR - bw * 2 - gap) / 2;
    int cy  = BANDEAU_HAUTEUR + 20;
    HWND h;

    h = mkLabel(titre, cx, cy, bw * 2 + gap, 42, hFontTitre);
    ajout(tab, nb, h);

    /* Ligne de separation */
    h = mkLabel("", cx, cy + 48, bw * 2 + gap, 1, hFontNormal);
    ajout(tab, nb, h);

    int i;
    for (i = 0; i < nbBtns; i++) {
        int x = cx + btns[i].col * (bw + gap);
        int y = cy + 64 + btns[i].row * (bh + gap);
        h = mkBtn(btns[i].t, btns[i].id, x, y, bw, bh);
        ajout(tab, nb, h);
    }

    /* Trouver le nombre de lignes */
    int maxRow = 0;
    for (i = 0; i < nbBtns; i++)
        if (btns[i].row > maxRow) maxRow = btns[i].row;

    /* Bouton deconnexion pleine largeur */
    int yD = cy + 64 + (maxRow + 1) * (bh + gap) + 6;
    h = mkBtn("Se deconnecter", idDeco,
              cx, yD, bw * 2 + gap, 36);
    ajout(tab, nb, h);
}

/**
 * @brief Cree les controles du menu locataire.
 * @return void
 */
void pageLocataire(void) {
    BoutonMenu B[] = {
        { "Voir les logements disponibles", ID_LOC_LOG,   0, 0 },
        { "Recherche avancee",              ID_LOC_RECH,  1, 0 },
        { "Reserver un logement",           ID_LOC_RES,   0, 1 },
        { "Mes reservations",               ID_LOC_MESRES,1, 1 },
        { "Mes favoris",                    ID_LOC_FAV,   0, 2 },
        { "Notations et avis",              ID_LOC_NOTE,  1, 2 },
        { "Mon profil",                     ID_LOC_PROFIL,0, 3 },
    };
    construireMenu("Menu Locataire", B, 7,
                   ID_LOC_DECO, ctrlLoc, &nbLoc);
}

/**
 * @brief Cree les controles du menu bailleur.
 * @return void
 */
void pageBailleur(void) {
    BoutonMenu B[] = {
        { "Publier une annonce",  ID_BAIL_PUB,   0, 0 },
        { "Mes annonces",         ID_BAIL_ANN,   1, 0 },
        { "Mes reservations",     ID_BAIL_RES,   0, 1 },
        { "Notifications",        ID_BAIL_NOTIF, 1, 1 },
        { "Mon profil",           ID_BAIL_PROFIL,0, 2 },
    };
    construireMenu("Menu Bailleur", B, 5,
                   ID_BAIL_DECO, ctrlBail, &nbBail);
}

/**
 * @brief Cree les controles du panneau administrateur.
 * @return void
 */
void pageAdmin(void) {
    BoutonMenu B[] = {
        { "Voir les utilisateurs",       ID_ADM_USERS,  0, 0 },
        { "Activer / Desactiver compte", ID_ADM_TOGGLE, 1, 0 },
        { "Voir les logements",          ID_ADM_LOG,    0, 1 },
        { "Statistiques",                ID_ADM_STATS,  1, 1 },
        { "Demandes reinitialisation",   ID_ADM_DEM,    0, 2 },
        { "Mon profil",                  ID_ADM_PROFIL, 1, 2 },
    };
    construireMenu("Panneau Administrateur", B, 6,
                   ID_ADM_DECO, ctrlAdm, &nbAdm);
}

/* ============================================================
 * ACTION — CONNEXION
 * ============================================================ */

/**
 * @brief Traite la tentative de connexion.
 *
 * 1. Lit les champs email et mdp
 * 2. Valide qu ils ne sont pas vides
 * 3. Recherche l email dans listeUtilisateurs[]
 * 4. Verifie que le compte est actif
 * 5. Compare le mot de passe
 * 6. Incremente nbEchecs et verrouille a 3
 * 7. Ouvre la session et navigue vers le bon menu
 *
 * @return void
 */
void traiterConnexion(void) {
    char email[TAILLE_EMAIL], mdp[TAILLE_MDP], msg[140];

    GetDlgItemText(hWnd, ID_EMAIL_CONN, email, TAILLE_EMAIL);
    GetDlgItemText(hWnd, ID_MDP_CONN,   mdp,   TAILLE_MDP);

    if (!strlen(email) || !strlen(mdp)) {
        MessageBox(hWnd,
            "Veuillez saisir votre email et votre mot de passe.",
            "Champs requis", MB_OK | MB_ICONWARNING);
        return;
    }

    int idx = rechercherUtilisateurParEmail(email);
    if (idx == -1) {
        MessageBox(hWnd,
            "Aucun compte associe a cet email.\n"
            "Verifiez l'adresse ou creez un compte.",
            "Compte introuvable", MB_OK | MB_ICONERROR);
        return;
    }

    if (!listeUtilisateurs[idx].actif) {
        MessageBox(hWnd,
            "Ce compte est verrouille.\n"
            "Contactez l'administrateur pour le reactiver.",
            "Compte bloque", MB_OK | MB_ICONERROR);
        return;
    }

    if (strcmp(listeUtilisateurs[idx].motDePasse, mdp) != 0) {
        listeUtilisateurs[idx].nbEchecs++;
        if (listeUtilisateurs[idx].nbEchecs >= 3) {
            listeUtilisateurs[idx].actif = 0;
            sauvegarderUtilisateurs();
            MessageBox(hWnd,
                "3 tentatives incorrectes.\n"
                "Votre compte est maintenant verrouille.\n"
                "Contactez l'administrateur.",
                "Securite", MB_OK | MB_ICONERROR);
        } else {
            sauvegarderUtilisateurs();
            snprintf(msg, sizeof(msg),
                "Mot de passe incorrect.  (%d / 3)\n"
                "Apres 3 echecs, le compte sera verrouille.",
                listeUtilisateurs[idx].nbEchecs);
            MessageBox(hWnd, msg, "Erreur de connexion",
                       MB_OK | MB_ICONWARNING);
        }
        return;
    }

    /* Connexion reussie */
    listeUtilisateurs[idx].nbEchecs = 0;
    sauvegarderUtilisateurs();
    sessionCourante.connecte    = 1;
    sessionCourante.utilisateur = listeUtilisateurs[idx];

    SetDlgItemText(hWnd, ID_EMAIL_CONN, "");
    SetDlgItemText(hWnd, ID_MDP_CONN,   "");

    snprintf(msg, sizeof(msg), "Bienvenue, %s %s  !",
             sessionCourante.utilisateur.prenom,
             sessionCourante.utilisateur.nom);
    MessageBox(hWnd, msg, "Connexion reussie",
               MB_OK | MB_ICONINFORMATION);

    switch (sessionCourante.utilisateur.role) {
        case ROLE_LOCATAIRE:      allerA(PAGE_LOC);   break;
        case ROLE_BAILLEUR:       allerA(PAGE_BAIL);  break;
        case ROLE_ADMINISTRATEUR: allerA(PAGE_ADMIN); break;
        default: break;
    }
}

/* ============================================================
 * ACTION — INSCRIPTION
 * ============================================================ */

/**
 * @brief Traite l inscription d un nouvel utilisateur.
 *
 * 1. Lit tous les champs du formulaire
 * 2. Valide que rien n est vide
 * 3. Verifie que les mots de passe correspondent
 * 4. Verifie longueur minimale du mot de passe (4 car.)
 * 5. Verifie que l email n est pas deja pris
 * 6. Cree et sauvegarde le compte
 * 7. Redirige vers la page connexion
 *
 * @return void
 */
void traiterInscription(void) {
    char nom[TAILLE_NOM],    prenom[TAILLE_NOM];
    char email[TAILLE_EMAIL],tel[TAILLE_TEL];
    char mdp[TAILLE_MDP],    conf[TAILLE_MDP];
    int ids[6] = { ID_NOM, ID_PRENOM, ID_EMAIL_INSCR,
                   ID_TEL, ID_MDP_INSCR, ID_MDP_CONF };
    int i;

    GetDlgItemText(hWnd, ID_NOM,         nom,    TAILLE_NOM);
    GetDlgItemText(hWnd, ID_PRENOM,      prenom, TAILLE_NOM);
    GetDlgItemText(hWnd, ID_EMAIL_INSCR, email,  TAILLE_EMAIL);
    GetDlgItemText(hWnd, ID_TEL,         tel,    TAILLE_TEL);
    GetDlgItemText(hWnd, ID_MDP_INSCR,   mdp,    TAILLE_MDP);
    GetDlgItemText(hWnd, ID_MDP_CONF,    conf,   TAILLE_MDP);

    if (!strlen(nom)||!strlen(prenom)||!strlen(email)||
        !strlen(tel)||!strlen(mdp)) {
        MessageBox(hWnd, "Tous les champs sont obligatoires.",
                   "Champs requis", MB_OK | MB_ICONWARNING);
        return;
    }
    if (strcmp(mdp, conf) != 0) {
        MessageBox(hWnd,
            "Les mots de passe ne correspondent pas.\n"
            "Verifiez la confirmation.",
            "Erreur", MB_OK | MB_ICONWARNING);
        return;
    }
    if ((int)strlen(mdp) < 4) {
        MessageBox(hWnd,
            "Le mot de passe doit contenir au moins 4 caracteres.",
            "Mot de passe trop court", MB_OK | MB_ICONWARNING);
        return;
    }
    if (emailExiste(email)) {
        MessageBox(hWnd,
            "Cet email est deja associe a un compte existant.",
            "Email indisponible", MB_OK | MB_ICONERROR);
        return;
    }

    int roleIdx = (int)SendDlgItemMessage(hWnd, ID_COMBO_ROLE,
                                          CB_GETCURSEL, 0, 0);
    Utilisateur u;
    memset(&u, 0, sizeof(Utilisateur));
    strncpy(u.nom,        nom,    TAILLE_NOM   - 1);
    strncpy(u.prenom,     prenom, TAILLE_NOM   - 1);
    strncpy(u.email,      email,  TAILLE_EMAIL - 1);
    strncpy(u.telephone,  tel,    TAILLE_TEL   - 1);
    strncpy(u.motDePasse, mdp,    TAILLE_MDP   - 1);
    u.role     = (roleIdx == 0) ? ROLE_LOCATAIRE : ROLE_BAILLEUR;
    u.id       = genererIdUtilisateur();
    u.nbEchecs = 0;
    u.actif    = 1;

    listeUtilisateurs[nbUtilisateurs++] = u;
    sauvegarderUtilisateurs();

    MessageBox(hWnd,
        "Compte cree avec succes !\n"
        "Vous pouvez maintenant vous connecter.",
        "Inscription reussie", MB_OK | MB_ICONINFORMATION);

    for (i = 0; i < 6; i++)
        SetDlgItemText(hWnd, ids[i], "");
    allerA(PAGE_CONN);
}

/* ============================================================
 * DISPATCHER DES CLICS
 * ============================================================ */

/**
 * @brief Dispatche les clics de boutons lies a l auth.
 *
 * Appelee depuis WndProc (WM_COMMAND) dans main_win32.c.
 * Chaque ID de bouton correspond a une action precise.
 *
 * @param idBtn ID du bouton clique (LOWORD de wParam).
 * @return void
 */
void gererClicAuth(int idBtn) {
    /* Recacher automatiquement le mdp quand l utilisateur retape
     * apres avoir active la visibilite (bouton oeil).
     * EN_CHANGE est envoye par Windows a chaque frappe. */
    switch (idBtn) {
        case ID_BTN_CONN:          traiterConnexion();  break;
        case ID_BTN_CREER:         traiterInscription();break;
        case ID_BTN_ALLER_INSCR:   allerA(PAGE_INSCR);  break;
        case ID_BTN_RETOUR_CONN:   allerA(PAGE_CONN);   break;

        case ID_BTN_VOIR_MDP:
            /* Basculer visibilite */
            mdpVisible = !mdpVisible;
            SendDlgItemMessage(hWnd, ID_MDP_CONN,
                EM_SETPASSWORDCHAR,
                mdpVisible ? 0 : (WPARAM)'*', 0);
            InvalidateRect(
                GetDlgItem(hWnd, ID_MDP_CONN), NULL, TRUE);
            break;

        /* Note : EN_CHANGE est gere dans main_win32.c
         * via WM_COMMAND avec notifCode == EN_CHANGE */

        case ID_BTN_ACCES_ADMIN:
            MessageBox(hWnd,
                "Acces Administrateur\n\n"
                "Email       :  admin@habitatcam.cm\n"
                "Mot de passe:  admin123\n\n"
                "Entrez ces identifiants dans les champs\n"
                "de connexion puis cliquez Se connecter.",
                "Acces Restreint",
                MB_OK | MB_ICONINFORMATION);
            break;

        case ID_LOC_DECO:
        case ID_BAIL_DECO:
        case ID_ADM_DECO:
            sessionCourante.connecte = 0;
            allerA(PAGE_CONN);
            break;

        default: break;
    }
}
