/* ============================================================
 * @file    main_win32.c
 * @brief   Fenetre principale et navigation — HabitatCam V3.0
 *
 * Point d entree de l application graphique Win32.
 * Responsabilites :
 *   - Creer et configurer la fenetre principale
 *   - Definir les polices, pinceaux et couleurs
 *   - Dessiner le bandeau superieur (WM_PAINT)
 *   - Orchestrer la navigation entre les pages
 *   - Dispatcher les messages vers win_auth.c et futurs modules
 *
 * Chaque membre de l equipe travaille dans son propre fichier :
 *   win_auth.c    -> SOUOPGUI  : connexion + inscription + menus
 *   win_logements.c -> FOMEKONG : page logements + photos
 *   win_admin.c   -> TOGNANG  : page admin + statistiques
 *   win_paiement.c -> NANDA   : paiement + factures PDF
 *   win_carte.c   -> CHEDJOU  : geolocalisation + carte
 *
 * @version 3.0
 * @author  SOUOPGUI
 * ============================================================ */

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "structures.h"
#include "auth.h"
#include "logement.h"
#include "locataire.h"
#include "bailleur.h"
#include "administrateur.h"
#include "matching.h"
#include "favoris.h"
#include "notation.h"
#include "notification.h"
#include "win_auth.h"

/* ============================================================
 * VARIABLES GLOBALES
 * Declarees ici — utilisees dans win_auth.c via extern
 * ============================================================ */
HWND   hWnd         = NULL;
HFONT  hFontTitre   = NULL;
HFONT  hFontSousTitre = NULL;
HFONT  hFontNormal  = NULL;
HFONT  hFontBold    = NULL;
HBRUSH hBrFond      = NULL;
HBRUSH hBrVert      = NULL;
HBRUSH hBrBlanc     = NULL;
HBRUSH hBrGrisClair = NULL;

/* Tableaux de controles par page */
HWND ctrlConn[MAX_CTRL];  int nbConn  = 0;
HWND ctrlInscr[MAX_CTRL]; int nbInscr = 0;
HWND ctrlLoc[MAX_CTRL];   int nbLoc   = 0;
HWND ctrlBail[MAX_CTRL];  int nbBail  = 0;
HWND ctrlAdm[MAX_CTRL];   int nbAdm   = 0;

Page pageCourante = PAGE_CONN;
BOOL mdpVisible   = FALSE;

/* ============================================================
 * FONCTIONS UTILITAIRES — Partagees avec win_auth.c
 * ============================================================ */

/**
 * @brief Enregistre un handle dans le tableau d une page.
 * @param tab Tableau de handles.
 * @param nb  Compteur.
 * @param h   Handle a ajouter.
 * @return void
 */
void ajout(HWND *tab, int *nb, HWND h) {
    if (*nb < MAX_CTRL) tab[(*nb)++] = h;
}

/**
 * @brief Cache tous les controles d un tableau.
 * @param tab Tableau de handles.
 * @param nb  Nombre de controles.
 * @return void
 */
static void cacher(HWND *tab, int nb) {
    int i;
    for (i = 0; i < nb; i++) ShowWindow(tab[i], SW_HIDE);
}

/**
 * @brief Affiche tous les controles d un tableau.
 * @param tab Tableau de handles.
 * @param nb  Nombre de controles.
 * @return void
 */
static void montrer(HWND *tab, int nb) {
    int i;
    for (i = 0; i < nb; i++) ShowWindow(tab[i], SW_SHOW);
}

/**
 * @brief Navigue vers une page.
 *
 * Cache tous les controles de la page courante,
 * affiche ceux de la nouvelle page et force le repaint
 * pour que le fond soit correctement redessinee.
 *
 * @param p La page de destination.
 * @return void
 */
void allerA(Page p) {
    switch (pageCourante) {
        case PAGE_CONN:  cacher(ctrlConn,  nbConn);  break;
        case PAGE_INSCR: cacher(ctrlInscr, nbInscr); break;
        case PAGE_LOC:   cacher(ctrlLoc,   nbLoc);   break;
        case PAGE_BAIL:  cacher(ctrlBail,  nbBail);  break;
        case PAGE_ADMIN: cacher(ctrlAdm,   nbAdm);   break;
    }
    pageCourante = p;
    switch (pageCourante) {
        case PAGE_CONN:  montrer(ctrlConn,  nbConn);  break;
        case PAGE_INSCR: montrer(ctrlInscr, nbInscr); break;
        case PAGE_LOC:   montrer(ctrlLoc,   nbLoc);   break;
        case PAGE_BAIL:  montrer(ctrlBail,  nbBail);  break;
        case PAGE_ADMIN: montrer(ctrlAdm,   nbAdm);   break;
    }
    InvalidateRect(hWnd, NULL, TRUE);
}

/**
 * @brief Cree un label texte statique.
 * @param txt   Texte a afficher.
 * @param x, y  Position.
 * @param w, h  Taille.
 * @param font  Police.
 * @return HWND Handle du label cree.
 */
HWND mkLabel(const char *txt, int x, int y,
             int w, int h, HFONT font) {
    HWND hw = CreateWindow("STATIC", txt,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, w, h, hWnd, NULL,
        GetModuleHandle(NULL), NULL);
    SendMessage(hw, WM_SETFONT, (WPARAM)font, TRUE);
    return hw;
}

/**
 * @brief Cree un champ de saisie.
 * @param id    Identifiant.
 * @param mdp   TRUE pour masquer.
 * @param x, y  Position.
 * @param w, h  Taille.
 * @return HWND Handle cree.
 */
HWND mkEdit(int id, BOOL mdp, int x, int y, int w, int h) {
    DWORD style = WS_CHILD | WS_VISIBLE |
                  WS_BORDER | ES_AUTOHSCROLL;
    if (mdp) style |= ES_PASSWORD;
    HWND hw = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
        style, x, y, w, h, hWnd,
        (HMENU)(size_t)id,
        GetModuleHandle(NULL), NULL);
    SendMessage(hw, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    return hw;
}

/**
 * @brief Cree un bouton.
 * @param txt   Libelle.
 * @param id    Identifiant.
 * @param x, y  Position.
 * @param w, h  Taille.
 * @return HWND Handle cree.
 */
HWND mkBtn(const char *txt, int id,
           int x, int y, int w, int h) {
    HWND hw = CreateWindow("BUTTON", txt,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, w, h, hWnd,
        (HMENU)(size_t)id,
        GetModuleHandle(NULL), NULL);
    SendMessage(hw, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    return hw;
}

/* ============================================================
 * INITIALISATION DES RESSOURCES
 * ============================================================ */

/**
 * @brief Cree toutes les polices de l application.
 *
 * Polices utilisees dans toute l interface :
 *   hFontTitre     : Arial 32 Gras   — titres de page
 *   hFontSousTitre : Arial 16 Normal — sous-titres
 *   hFontNormal    : Arial 15 Normal — labels et champs
 *   hFontBold      : Arial 15 Gras   — boutons importants
 *
 * @return void
 */
static void creerPolices(void) {
    hFontTitre = CreateFont(
        32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Arial");

    hFontSousTitre = CreateFont(
        16, 0, 0, 0, FW_NORMAL, TRUE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Arial");

    hFontNormal = CreateFont(
        15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Arial");

    hFontBold = CreateFont(
        15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Arial");
}

/**
 * @brief Cree tous les pinceaux de couleur de l application.
 * @return void
 */
static void creerPinceaux(void) {
    hBrFond     = CreateSolidBrush(CLR_VERT_CLAIR);
    hBrVert     = CreateSolidBrush(CLR_VERT_FONCE);
    hBrBlanc    = CreateSolidBrush(CLR_BLANC);
    hBrGrisClair= CreateSolidBrush(CLR_GRIS_CLAIR);
}

/**
 * @brief Libere toutes les ressources GDI allouees.
 * Appelee dans WM_DESTROY pour eviter les fuites memoire.
 * @return void
 */
static void libererRessources(void) {
    if (hFontTitre)    { DeleteObject(hFontTitre);    hFontTitre    = NULL; }
    if (hFontSousTitre){ DeleteObject(hFontSousTitre);hFontSousTitre= NULL; }
    if (hFontNormal)   { DeleteObject(hFontNormal);   hFontNormal   = NULL; }
    if (hFontBold)     { DeleteObject(hFontBold);     hFontBold     = NULL; }
    if (hBrFond)       { DeleteObject(hBrFond);       hBrFond       = NULL; }
    if (hBrVert)       { DeleteObject(hBrVert);       hBrVert       = NULL; }
    if (hBrBlanc)      { DeleteObject(hBrBlanc);      hBrBlanc      = NULL; }
    if (hBrGrisClair)  { DeleteObject(hBrGrisClair);  hBrGrisClair  = NULL; }
}

/* ============================================================
 * DESSIN DU BANDEAU SUPERIEUR
 * ============================================================ */

/**
 * @brief Dessine le bandeau vert en haut de la fenetre.
 *
 * Contenu du bandeau :
 *   - Fond vert fonce sur toute la largeur
 *   - Texte "HabitatCam | Plateforme de logement" a gauche
 *   - Nom de l utilisateur connecte a droite (si connecte)
 *   - Ligne de separation fine en bas du bandeau
 *
 * @param hdc Handle du contexte graphique (Device Context).
 * @return void
 */
static void dessinerBandeau(HDC hdc) {
    RECT r;
    GetClientRect(hWnd, &r);

    /* Fond vert du bandeau */
    RECT bandeau = { 0, 0, r.right, BANDEAU_HAUTEUR };
    FillRect(hdc, &bandeau, hBrVert);

    /* Ligne de separation fine */
    HPEN pen = CreatePen(PS_SOLID, 2, CLR_OR);
    HPEN old = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, 0,       BANDEAU_HAUTEUR - 1, NULL);
    LineTo  (hdc, r.right, BANDEAU_HAUTEUR - 1);
    SelectObject(hdc, old);
    DeleteObject(pen);

    /* Texte du bandeau */
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, CLR_BLANC);
    SelectObject(hdc, hFontBold);

    RECT tl = { 20, 0, 500, BANDEAU_HAUTEUR };
    DrawText(hdc, "HabitatCam", -1, &tl,
             DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hFontNormal);
    RECT sl = { 160, 0, 560, BANDEAU_HAUTEUR };
    DrawText(hdc, "|  Plateforme de logement au Cameroun",
             -1, &sl, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    /* Nom utilisateur a droite si connecte */
    if (sessionCourante.connecte) {
        char nom[100];
        snprintf(nom, sizeof(nom), "%s %s",
                 sessionCourante.utilisateur.prenom,
                 sessionCourante.utilisateur.nom);
        RECT nr = { r.right - 240, 0, r.right - 16, BANDEAU_HAUTEUR };
        DrawText(hdc, nom, -1, &nr,
                 DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }
}

/* ============================================================
 * ACCES ADMINISTRATEUR — Raccourci secret Ctrl+Shift+A
 * ============================================================ */

/**
 * @brief Procedure de la fenetre modale d acces admin.
 *
 * Fenetre a 3 etapes sequentielles :
 *   etapeAdmin == 0 : saisie du code secret
 *   etapeAdmin == 1 : saisie de l email admin
 *   etapeAdmin == 2 : saisie du mot de passe
 *
 * Titre neutre a chaque etape — aucune mention "admin".
 *
 * @param hwnd   Handle de la fenetre modale.
 * @param msg    Message Windows.
 * @param wParam Parametre 1.
 * @param lParam Parametre 2.
 * @return LRESULT Code de retour.
 */
/* Prototype anticipe — ouvrirAccesAdmin est definie apres procDlgAdmin
 * mais appelee dedans lors des transitions entre etapes. */
static void ouvrirAccesAdmin(void);

static int   etapeAdmin = 0;
static char  emailAdmin[TAILLE_EMAIL];
static HWND  hDlgAdmin  = NULL;

static LRESULT CALLBACK procDlgAdmin(HWND hwnd, UINT msg,
                                      WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {

        case WM_CREATE: {
            HFONT f = hFontNormal;
            /* Label neutre selon l etape */
            const char *lbl =
                etapeAdmin == 0 ? "Code de verification :" :
                etapeAdmin == 1 ? "Identifiant :"         :
                                  "Confirmation :"        ;
            HWND hl = CreateWindow("STATIC", lbl,
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 24, 290, 22, hwnd, NULL,
                GetModuleHandle(NULL), NULL);
            SendMessage(hl, WM_SETFONT, (WPARAM)f, TRUE);

            /* Champ masque */
            HWND he = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER |
                ES_AUTOHSCROLL | ES_PASSWORD,
                20, 54, 290, 30, hwnd,
                (HMENU)901, GetModuleHandle(NULL), NULL);
            SendMessage(he, WM_SETFONT, (WPARAM)f, TRUE);

            /* Boutons */
            HWND hok = CreateWindow("BUTTON", "Valider",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 104, 135, 32, hwnd,
                (HMENU)904, GetModuleHandle(NULL), NULL);
            HWND han = CreateWindow("BUTTON", "Annuler",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                175, 104, 135, 32, hwnd,
                (HMENU)905, GetModuleHandle(NULL), NULL);
            SendMessage(hok, WM_SETFONT, (WPARAM)f, TRUE);
            SendMessage(han, WM_SETFONT, (WPARAM)f, TRUE);
            (void)hl; (void)he; (void)hok; (void)han;

            /* Centrer sur la fenetre principale */
            RECT rp; GetWindowRect(hWnd, &rp);
            int dw = 340, dh = 160;
            SetWindowPos(hwnd, HWND_TOP,
                rp.left + (rp.right  - rp.left - dw) / 2,
                rp.top  + (rp.bottom - rp.top  - dh) / 2,
                dw, dh, SWP_NOSIZE);
            return 0;
        }

        case WM_COMMAND: {
            int id = (int)LOWORD(wParam);
            if (id == 905) { /* Annuler */
                DestroyWindow(hwnd);
                hDlgAdmin = NULL;
                etapeAdmin = 0;
                return 0;
            }
            if (id == 904) { /* Valider */
                char saisi[TAILLE_EMAIL];
                GetDlgItemText(hwnd, 901, saisi, sizeof(saisi));

                if (!strlen(saisi)) {
                    MessageBox(hwnd, "Champ vide.",
                               "Erreur", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                if (etapeAdmin == 0) {
                    /* Lire le code secret depuis data/admin_config.txt */
                    char codeSecret[64] = "HABITAT2025";
                    FILE *fcs = fopen("data/admin_config.txt", "r");
                    if (fcs) {
                        char ligne[128];
                        while (fgets(ligne, sizeof(ligne), fcs)) {
                            ligne[strcspn(ligne, "\r\n")] = 0;
                            if (strncmp(ligne, "CODE_SECRET=", 12) == 0) {
                                strncpy(codeSecret, ligne + 12,
                                        sizeof(codeSecret) - 1);
                                break;
                            }
                        }
                        fclose(fcs);
                    }
                    if (strcmp(saisi, codeSecret) != 0) {
                        MessageBox(hwnd, "Code incorrect.",
                                   "Erreur", MB_OK | MB_ICONERROR);
                        SetDlgItemText(hwnd, 901, "");
                        return 0;
                    }
                    etapeAdmin = 1;
                    DestroyWindow(hwnd); hDlgAdmin = NULL;
                    ouvrirAccesAdmin(); return 0;
                }

                if (etapeAdmin == 1) {
                    int idx = rechercherUtilisateurParEmail(saisi);
                    if (idx == -1 ||
                        listeUtilisateurs[idx].role != ROLE_ADMINISTRATEUR) {
                        MessageBox(hwnd, "Identifiant inconnu.",
                                   "Erreur", MB_OK | MB_ICONERROR);
                        SetDlgItemText(hwnd, 901, "");
                        return 0;
                    }
                    strncpy(emailAdmin, saisi, TAILLE_EMAIL - 1);
                    etapeAdmin = 2;
                    DestroyWindow(hwnd); hDlgAdmin = NULL;
                    ouvrirAccesAdmin(); return 0;
                }

                if (etapeAdmin == 2) {
                    int idx = rechercherUtilisateurParEmail(emailAdmin);
                    if (idx == -1) return 0;
                    if (strcmp(listeUtilisateurs[idx].motDePasse, saisi) != 0) {
                        listeUtilisateurs[idx].nbEchecs++;
                        sauvegarderUtilisateurs();
                        MessageBox(hwnd, "Confirmation incorrecte.",
                                   "Erreur", MB_OK | MB_ICONERROR);
                        SetDlgItemText(hwnd, 901, "");
                        return 0;
                    }
                    /* Succes — ouvrir session admin */
                    listeUtilisateurs[idx].nbEchecs = 0;
                    sauvegarderUtilisateurs();
                    sessionCourante.connecte    = 1;
                    sessionCourante.utilisateur = listeUtilisateurs[idx];
                    DestroyWindow(hwnd);
                    hDlgAdmin = NULL; etapeAdmin = 0;
                    char msg2[100];
                    snprintf(msg2, sizeof(msg2), "Bienvenue, %s %s !",
                             sessionCourante.utilisateur.prenom,
                             sessionCourante.utilisateur.nom);
                    MessageBox(hWnd, msg2, "Acces accorde",
                               MB_OK | MB_ICONINFORMATION);
                    allerA(PAGE_ADMIN);
                    return 0;
                }
            }
            return 0;
        }
        case WM_DESTROY:
            hDlgAdmin = NULL; etapeAdmin = 0;
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/**
 * @brief Ouvre la fenetre modale d acces administrateur.
 *
 * Declenchee uniquement par Ctrl+Shift+A — totalement invisible
 * pour un utilisateur ordinaire. Fenetre a 3 etapes avec
 * titres neutres (pas de mention "Admin").
 *
 * @return void
 */
static void ouvrirAccesAdmin(void) {
    if (hDlgAdmin) { SetForegroundWindow(hDlgAdmin); return; }

    /* Enregistrer la classe (ignoree si deja faite) */
    WNDCLASSEX wcD;
    memset(&wcD, 0, sizeof(wcD));
    wcD.cbSize        = sizeof(WNDCLASSEX);
    wcD.lpfnWndProc   = procDlgAdmin;
    wcD.hInstance     = GetModuleHandle(NULL);
    wcD.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcD.lpszClassName = "HCAdminDlg";
    wcD.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClassEx(&wcD);

    const char *titre =
        etapeAdmin == 0 ? "Verification"     :
        etapeAdmin == 1 ? "Authentification" : "Confirmation";

    hDlgAdmin = CreateWindowEx(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "HCAdminDlg", titre,
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, 340, 160,
        hWnd, NULL, GetModuleHandle(NULL), NULL);

    if (hDlgAdmin) { ShowWindow(hDlgAdmin, SW_SHOW); UpdateWindow(hDlgAdmin); }
}
/* ============================================================
 * PROCEDURE PRINCIPALE DE LA FENETRE
 * ============================================================ */

/**
 * @brief Procedure principale Win32 — gere tous les evenements.
 *
 * Windows envoie ici chaque evenement de l application.
 * On traite les messages importants et on delegue les autres
 * a DefWindowProc (comportement standard Windows).
 *
 * Messages traites :
 *   WM_CREATE   : initialiser ressources + creer les pages
 *   WM_PAINT    : dessiner le bandeau superieur
 *   WM_CTLCOLORSTATIC : couleur de fond des labels
 *   WM_CTLCOLOREDIT   : couleur de fond des champs
 *   WM_COMMAND  : dispatcher les clics de boutons
 *   WM_DESTROY  : liberer les ressources et quitter
 *
 * @param hwnd   Handle de la fenetre.
 * @param msg    Type du message Windows.
 * @param wParam Parametre 1 (varie selon le message).
 * @param lParam Parametre 2 (varie selon le message).
 * @return LRESULT Code de retour du message.
 */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
                          WPARAM wParam, LPARAM lParam) {
    switch (msg) {

        /* ── Initialisation ── */
        case WM_CREATE:
            hWnd = hwnd;
            creerPolices();
            creerPinceaux();

            /* Charger les donnees V2.0 */
            chargerUtilisateurs();
            chargerLogements();
            chargerReservations();

            /* Creer toutes les pages depuis win_auth.c */
            pageConnexion();
            pageInscription();
            pageLocataire();
            pageBailleur();
            pageAdmin();

            /* Demarrer sur la page connexion */
            pageCourante = PAGE_CONN;
            cacher(ctrlInscr, nbInscr);
            cacher(ctrlLoc,   nbLoc);
            cacher(ctrlBail,  nbBail);
            cacher(ctrlAdm,   nbAdm);
            return 0;

        /* ── Dessin ── */
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            /* Fond general vert clair */
            RECT r;
            GetClientRect(hwnd, &r);
            FillRect(hdc, &r, hBrFond);

            /* Bandeau superieur */
            dessinerBandeau(hdc);

            EndPaint(hwnd, &ps);
            return 0;
        }

        /* ── Couleur fond des labels ── */
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, CLR_VERT_CLAIR);
            SetTextColor(hdc, CLR_TEXTE);
            return (LRESULT)hBrFond;
        }

        /* ── Couleur fond des champs de saisie ── */
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, CLR_BLANC);
            SetTextColor(hdc, CLR_TEXTE);
            return (LRESULT)hBrBlanc;
        }

        /* ── Raccourci clavier secret : Ctrl + Shift + A ──
         *
         * C est la seule facon d acceder au panneau admin.
         * Aucun bouton visible — un utilisateur lambda
         * ne peut pas deviner que ce raccourci existe.
         * GetKeyState retourne < 0 si la touche est enfoncee.
         * ── */
        case WM_KEYDOWN: {
            BOOL ctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            BOOL shift = (GetKeyState(VK_SHIFT)   & 0x8000) != 0;
            BOOL aKey  = ((int)wParam == 'A');

            if (ctrl && shift && aKey) {
                ouvrirAccesAdmin();
            }
            return 0;
        }

        /* ── Clics de boutons et notifications ── */
        case WM_COMMAND: {
            int id       = (int)LOWORD(wParam);
            int notifCode= (int)HIWORD(wParam);

            /* Recacher automatiquement le mdp quand l utilisateur
             * retape apres avoir active la visibilite.
             * EN_CHANGE est envoye a chaque frappe dans le champ. */
            if (id == ID_MDP_CONN &&
                notifCode == EN_CHANGE && mdpVisible) {
                mdpVisible = FALSE;
                SendDlgItemMessage(hWnd, ID_MDP_CONN,
                    EM_SETPASSWORDCHAR, (WPARAM)'*', 0);
                InvalidateRect(
                    GetDlgItem(hWnd, ID_MDP_CONN), NULL, TRUE);
            }

            /* Deleguer vers win_auth.c */
            gererClicAuth(id);

            /*
             * Les futurs modules ajouteront leurs appels ici :
             * gererClicLogements(id);   <- FOMEKONG
             * gererClicAdmin(id);       <- TOGNANG
             * gererClicPaiement(id);    <- NANDA
             * gererClicCarte(id);       <- CHEDJOU
             */

            return 0;
        }

        /* ── Fermeture ── */
        case WM_DESTROY:
            libererRessources();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ============================================================
 * POINT D ENTREE WIN32
 * ============================================================ */

/**
 * @brief Point d entree de l application Win32.
 *
 * Remplace main() pour les applications graphiques Windows.
 * Etapes :
 *   1. Enregistrer la classe de fenetre
 *   2. Creer la fenetre principale
 *   3. L afficher
 *   4. Lancer la boucle de messages
 *
 * @param hInst   Instance de l application.
 * @param hPrev   Toujours NULL (historique Win16).
 * @param lpCmd   Arguments de la ligne de commande.
 * @param nShow   Mode d affichage initial.
 * @return int    Code de retour de l application.
 */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev,
                   LPSTR lpCmd, int nShow) {
    (void)hPrev;
    (void)lpCmd;

    /* Enregistrer la classe de fenetre */
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(WNDCLASSEX));
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "HabitatCamV3";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Erreur d'enregistrement de la fenetre.",
                   "Erreur critique", MB_OK | MB_ICONERROR);
        return 1;
    }

    /* Creer la fenetre principale */
    HWND hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        "HabitatCamV3",
        "HabitatCam V3.0  -  Plateforme de logement",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        FENETRE_LARGEUR, FENETRE_HAUTEUR,
        NULL, NULL, hInst, NULL);

    if (!hwnd) {
        MessageBox(NULL, "Impossible de creer la fenetre.",
                   "Erreur critique", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    /* Boucle de messages — coeur de l application Win32 */
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
