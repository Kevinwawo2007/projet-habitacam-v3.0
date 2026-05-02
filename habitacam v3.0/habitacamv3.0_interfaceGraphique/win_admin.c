/* ============================================================
 * @file    win_admin.c
 * @brief   Page Administrateur graphique — HabitatCam V3.0
 *
 * Ce fichier est le pendant graphique de administrateur.c.
 * Il remplace les printf/scanf par une interface Win32 native.
 *
 * INTEGRATION AU PROJET :
 *   1. Ajoutez ce fichier dans votre projet CodeBlocks / Makefile
 *   2. Dans main_win32.c, decommenter :
 *        gererClicAdmin(id);   dans WM_COMMAND
 *   3. Dans win_admin.h (a creer), declarer :
 *        void pageAdmin(void);
 *        void gererClicAdmin(int id);
 *   4. Les variables globales (listeUtilisateurs, listeLogements,
 *      sessionCourante, sauvegarderUtilisateurs, etc.) viennent
 *      de vos fichiers existants (auth.c, logement.c, structures.h)
 *
 * FONCTIONNALITES (miroir exact de administrateur.c) :
 *   - Voir tous les utilisateurs
 *   - Activer / Desactiver un compte
 *   - Reactiver un compte verrouille
 *   - Supprimer un utilisateur
 *   - Voir tous les logements
 *   - Supprimer un logement
 *   - Statistiques generales
 *   - Demandes de reinitialisation de mot de passe
 *   - Deconnexion
 *
 * COMPILATION STANDALONE (test sans le reste du projet) :
 *   gcc win_admin.c -o win_admin.exe -mwindows -lcomctl32
 *   (Voir bas du fichier : WinMain de test active par
 *    #define WIN_ADMIN_STANDALONE)
 *
 * @version 3.0
 * @author  TOGNANG — HabitatCam V3.0
 * ============================================================ */

/* ── Pour compiler en standalone sans le reste du projet ──
 * Decommenter la ligne suivante pour tester seul ce fichier.
 * Re-commenter avant d integrer au projet GitHub.           */
#define WIN_ADMIN_STANDALONE

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

/* ── En mode integration, ces includes viennent du projet ── */
#ifdef WIN_ADMIN_STANDALONE

/* ------- Reproduction minimale des structures du projet ------- */
#define MAX_UTILISATEURS  100
#define MAX_LOGEMENTS     200
#define MAX_RESERVATIONS  300
#define TAILLE_NOM        50
#define TAILLE_EMAIL      100
#define TAILLE_MDP        64
#define TAILLE_TITRE      100
#define TAILLE_VILLE      60
#define TAILLE_TYPE       30

typedef enum {
    ROLE_LOCATAIRE = 0,
    ROLE_BAILLEUR,
    ROLE_ADMINISTRATEUR
} Role;

typedef enum {
    STATUT_DISPONIBLE = 0,
    STATUT_RESERVE,
    STATUT_INDISPONIBLE
} StatutLogement;

typedef struct {
    int   id;
    char  nom[TAILLE_NOM];
    char  prenom[TAILLE_NOM];
    char  email[TAILLE_EMAIL];
    char  motDePasse[TAILLE_MDP];
    Role  role;
    int   actif;
    int   nbEchecs;
} Utilisateur;

typedef struct {
    int           id;
    char          titre[TAILLE_TITRE];
    char          type[TAILLE_TYPE];
    char          ville[TAILLE_VILLE];
    float         superficie;
    double        prixMensuel;
    StatutLogement statut;
    int           idBailleur;
} Logement;

typedef struct {
    int  idReservation;
    int  idLogement;
    int  idLocataire;
    char dateDebut[20];
    char dateFin[20];
} Reservation;

typedef struct {
    int         connecte;
    Utilisateur utilisateur;
} Session;

/* Variables globales du projet (definies dans auth.c / logement.c) */
Utilisateur listeUtilisateurs[MAX_UTILISATEURS] = {
    {1,"Admin",  "Super", "admin@habitatcam.cm",    "admin123",  ROLE_ADMINISTRATEUR,1,0},
    {2,"Dupont", "Jean",  "jean.dupont@gmail.com",  "pass1234",  ROLE_LOCATAIRE,     1,0},
    {3,"Mballa", "Claire","claire.m@yahoo.fr",      "claire99",  ROLE_LOCATAIRE,     0,3},
    {4,"Fotso",  "Paul",  "paul.fotso@hotmail.com", "fotso2024", ROLE_BAILLEUR,      1,1},
    {5,"Nguema", "Alice", "alice.n@gmail.com",      "alice007",  ROLE_LOCATAIRE,     0,2},
    {6,"Kamdem", "Pierre","pierre.k@cm.net",        "pierre123", ROLE_BAILLEUR,      1,0},
};
int nbUtilisateurs = 6;

Logement listeLogements[MAX_LOGEMENTS] = {
    {101,"Appartement T3 Bastos",      "Appartement","Yaounde", 80.0f, 150000,STATUT_DISPONIBLE,  4},
    {102,"Villa 4 pieces Bonamoussadi","Villa",       "Douala", 200.0f,350000,STATUT_RESERVE,     4},
    {103,"Studio meuble Nlongkak",     "Studio",      "Yaounde", 30.0f, 60000,STATUT_DISPONIBLE,  6},
    {104,"Duplex Bali",                "Duplex",      "Douala", 120.0f,220000,STATUT_INDISPONIBLE,6},
    {105,"Chambre self-contained",     "Chambre",     "Bafoussam",20.0f,35000,STATUT_DISPONIBLE,  4},
};
int nbLogements = 5;

Reservation listeReservations[MAX_RESERVATIONS];
int nbReservations = 2;

Session sessionCourante = {1,{1,"Admin","Super","admin@habitatcam.cm","",ROLE_ADMINISTRATEUR,1,0}};

/* Stubs des fonctions de sauvegarde */
void sauvegarderUtilisateurs(void) { /* integrer : appel reel */ }
void sauvegarderLogements(void)    { /* integrer : appel reel */ }
void deconnecterUtilisateur(void)  { sessionCourante.connecte = 0; }

/* Palette de couleurs (miroir de win_auth.h) */
#define CLR_VERT_FONCE  RGB(30,  90,  60)
#define CLR_VERT_CLAIR  RGB(220,245,225)
#define CLR_BLANC       RGB(255,255,255)
#define CLR_TEXTE       RGB(20,  20,  20)
#define CLR_GRIS_CLAIR  RGB(240,240,240)
#define CLR_OR          RGB(200,160,  0)
#define BANDEAU_HAUTEUR 60
#define FENETRE_LARGEUR 1100
#define FENETRE_HAUTEUR 700
#define MAX_CTRL        80
#define PAGE_CONN  0
#define PAGE_INSCR 1
#define PAGE_LOC   2
#define PAGE_BAIL  3
#define PAGE_ADMIN 4
typedef int Page;

/* Variables globales de la fenetre principale */
HWND   hWnd           = NULL;
HFONT  hFontTitre     = NULL;
HFONT  hFontSousTitre = NULL;
HFONT  hFontNormal    = NULL;
HFONT  hFontBold      = NULL;
HBRUSH hBrFond        = NULL;
HBRUSH hBrVert        = NULL;
HBRUSH hBrBlanc       = NULL;
HBRUSH hBrGrisClair   = NULL;
HWND ctrlAdm[MAX_CTRL]; int nbAdm = 0;
Page pageCourante = PAGE_ADMIN;

void ajout(HWND *tab, int *nb, HWND h){ if(*nb<MAX_CTRL) tab[(*nb)++]=h; }
void allerA(Page p){ (void)p; }

#else
/* ── Mode integration : includes reels du projet ── */
#include "structures.h"
#include "auth.h"
#include "logement.h"
#include "administrateur.h"
#include "win_auth.h"
#include "win_admin.h"
#endif /* WIN_ADMIN_STANDALONE */

/* ============================================================
 * IDENTIFIANTS DES CONTROLES DU PANNEAU ADMIN
 * Plage reservee : 3000 - 3099 (ne pas chevaucher win_auth.c)
 * ============================================================ */
#define ID_ADM_TITRE        3000
#define ID_ADM_NOM_ADMIN    3001
#define ID_ADM_STATUS_BAR   3002

/* Boutons de navigation */
#define ID_ADM_BTN_USERS    3010
#define ID_ADM_BTN_TOGGLE   3011
#define ID_ADM_BTN_REACTIV  3012
#define ID_ADM_BTN_DEL_USR  3013
#define ID_ADM_BTN_LOGS     3014
#define ID_ADM_BTN_DEL_LOG  3015
#define ID_ADM_BTN_STATS    3016
#define ID_ADM_BTN_DEMANDES 3017
#define ID_ADM_BTN_DECONN   3018

/* ListView */
#define ID_ADM_LISTVIEW     3020

/* Message interne : signale la fin d un sous-dialogue */
#define WM_SUBDLG_DONE  (WM_APP + 42)

/* Identifiants dans les sous-dialogues */
#define IDC_DLG_EDIT_ID     3030
#define IDC_DLG_EDIT_MDP    3031
#define IDC_DLG_COMBO       3032

/* ============================================================
 * VARIABLES INTERNES AU MODULE
 * ============================================================ */
static HWND g_hList    = NULL;   /* ListView principal         */
static HWND g_hStatus  = NULL;   /* Barre de statut bas        */
static HWND g_hSecTitle = NULL;  /* Titre de la section active */

/* Noms des classes Win32 pour les sous-dialogues */
static const char CLASS_DLG_ID[]    = "HCAdminDlgID";
static const char CLASS_DLG_COMBO[] = "HCAdminDlgCombo";

/* ============================================================
 * UTILITAIRES INTERNES
 * ============================================================ */
static const char* roleEnTexte(Role r){
    switch(r){
        case ROLE_LOCATAIRE:      return "Locataire";
        case ROLE_BAILLEUR:       return "Bailleur";
        case ROLE_ADMINISTRATEUR: return "Administrateur";
        default:                  return "Inconnu";
    }
}
static const char* statutEnTexte(StatutLogement s){
    switch(s){
        case STATUT_DISPONIBLE:   return "Disponible";
        case STATUT_RESERVE:      return "Reserve";
        case STATUT_INDISPONIBLE: return "Indisponible";
        default:                  return "Inconnu";
    }
}
static void setStatut(const char *msg){
    if(g_hStatus) SetWindowText(g_hStatus, msg);
}
static void viderListe(void){
    if(g_hList) ListView_DeleteAllItems(g_hList);
}
static void viderColonnes(void){
    if(!g_hList) return;
    while(ListView_DeleteColumn(g_hList, 0));
}
static void ajouterColonne(int idx, const char *titre, int largeur){
    LVCOLUMN c = {0};
    c.mask    = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    c.cx      = largeur;
    c.pszText = (LPSTR)titre;
    c.iSubItem = idx;
    ListView_InsertColumn(g_hList, idx, &c);
}
static void ajouterLigne(int lig, int nbCols, const char **vals){
    LVITEM li = {0};
    li.mask    = LVIF_TEXT;
    li.iItem   = lig;
    li.pszText = (LPSTR)vals[0];
    ListView_InsertItem(g_hList, &li);
    for(int c = 1; c < nbCols; c++)
        ListView_SetItemText(g_hList, lig, c, (LPSTR)vals[c]);
}

/* ============================================================
 * VUES DU LISTVIEW
 * ============================================================ */

/* --- Vue : liste des utilisateurs --- */
static void vueUtilisateurs(void){
    SetWindowText(g_hSecTitle, "  Gestion des Utilisateurs");
    viderColonnes(); viderListe();
    ajouterColonne(0,"ID",    45);
    ajouterColonne(1,"Nom",  110);
    ajouterColonne(2,"Prenom",110);
    ajouterColonne(3,"Email",185);
    ajouterColonne(4,"Role", 110);
    ajouterColonne(5,"Echecs",55);
    ajouterColonne(6,"Statut",85);
    for(int i = 0; i < nbUtilisateurs; i++){
        Utilisateur *u = &listeUtilisateurs[i];
        char wId[8], wEch[8];
        sprintf(wId,  "%d", u->id);
        sprintf(wEch, "%d", u->nbEchecs);
        const char *v[] = { wId, u->nom, u->prenom, u->email,
                             roleEnTexte(u->role), wEch,
                             u->actif ? "Actif" : "DESACTIVE" };
        ajouterLigne(i, 7, v);
    }
    char buf[64];
    sprintf(buf, "Total : %d utilisateur(s)", nbUtilisateurs);
    setStatut(buf);
}

/* --- Vue : liste des logements --- */
static void vueLogements(void){
    SetWindowText(g_hSecTitle, "  Gestion des Logements");
    viderColonnes(); viderListe();
    ajouterColonne(0,"ID",    45);
    ajouterColonne(1,"Titre",170);
    ajouterColonne(2,"Type",  80);
    ajouterColonne(3,"Ville", 80);
    ajouterColonne(4,"Surf.", 60);
    ajouterColonne(5,"Prix",  95);
    ajouterColonne(6,"Statut",85);
    for(int i = 0; i < nbLogements; i++){
        Logement *l = &listeLogements[i];
        char wId[8], wSurf[16], wPrix[20];
        sprintf(wId,   "%d",        l->id);
        sprintf(wSurf, "%.0f m2",   l->superficie);
        sprintf(wPrix, "%.0f FCFA", l->prixMensuel);
        const char *v[] = { wId, l->titre, l->type, l->ville,
                             wSurf, wPrix, statutEnTexte(l->statut) };
        ajouterLigne(i, 7, v);
    }
    char buf[64];
    sprintf(buf, "Total : %d logement(s)", nbLogements);
    setStatut(buf);
}

/* --- Vue : statistiques --- */
static void vueStatistiques(void){
    SetWindowText(g_hSecTitle, "  Tableau de Bord - Statistiques");
    viderColonnes(); viderListe();
    ajouterColonne(0,"Categorie",220);
    ajouterColonne(1,"Valeur",   95);
    ajouterColonne(2,"Detail",  255);

    int nLoc=0,nBaill=0,nAdm=0,nAct=0,nInact=0,nVerr=0;
    int nDispo=0,nRes=0,nInd=0;
    for(int i=0;i<nbUtilisateurs;i++){
        switch(listeUtilisateurs[i].role){
            case ROLE_LOCATAIRE:      nLoc++;   break;
            case ROLE_BAILLEUR:       nBaill++; break;
            case ROLE_ADMINISTRATEUR: nAdm++;   break;
        }
        if(listeUtilisateurs[i].actif) nAct++;
        else{ nInact++; if(listeUtilisateurs[i].nbEchecs>=3) nVerr++; }
    }
    for(int i=0;i<nbLogements;i++){
        switch(listeLogements[i].statut){
            case STATUT_DISPONIBLE:   nDispo++; break;
            case STATUT_RESERVE:      nRes++;   break;
            case STATUT_INDISPONIBLE: nInd++;   break;
        }
    }

    char b[16]; int r=0;
    #define LIG(cat,val,det){ sprintf(b,"%d",val); \
        const char *v[]={cat,b,det}; ajouterLigne(r++,3,v); }

    LIG("UTILISATEURS (total)", nbUtilisateurs, "")
    LIG("  Locataires",         nLoc,    "")
    LIG("  Bailleurs",          nBaill,  "")
    LIG("  Administrateurs",    nAdm,    "")
    LIG("  Comptes Actifs",     nAct,    "Connectables")
    LIG("  Comptes Desactives", nInact,  "Bloques par admin ou echecs")
    LIG("  Comptes Verrouilles",nVerr,   "Trop d'echecs de connexion (>=3)")
    { const char *v[]={"---","---","---"}; ajouterLigne(r++,3,v); }
    LIG("LOGEMENTS (total)",    nbLogements, "")
    LIG("  Disponibles",        nDispo,  "")
    LIG("  Reserves",           nRes,    "")
    LIG("  Indisponibles",      nInd,    "")
    { const char *v[]={"---","---","---"}; ajouterLigne(r++,3,v); }
    LIG("RESERVATIONS (total)", nbReservations, "")
    #undef LIG

    setStatut("Statistiques generales de la plateforme HabitatCam");
}

/* ============================================================
 * SOUS-DIALOGUES MODAUX STABLES
 *
 * Principe cle :
 *   - Parent desactive avec EnableWindow(hWnd, FALSE)
 *   - Sous-dialogue envoie WM_SUBDLG_DONE via PostMessage
 *     quand il est pret a fermer (OK, Annuler, croix X)
 *   - La boucle modale attend UNIQUEMENT WM_SUBDLG_DONE
 *   - Nettoyage (DestroyWindow + EnableWindow) GARANTI
 *     dans tous les cas (bloc final inconditonnel)
 *
 * Resultat stocke dans GWLP_USERDATA du sous-dialogue :
 *   -1  = annule / ferme
 *   >=0 = valeur retournee (ID ou index)
 * ============================================================ */

/* --- Procedure : saisie d un entier (ID) --- */
static LRESULT CALLBACK ProcDlgID(HWND hD, UINT msg,
                                    WPARAM wP, LPARAM lP){
    (void)lP;
    switch(msg){
    case WM_COMMAND:
        if(LOWORD(wP)==IDOK){
            char buf[16]={0};
            GetDlgItemText(hD, IDC_DLG_EDIT_ID, buf, 16);
            int val = atoi(buf);
            SetWindowLongPtr(hD, GWLP_USERDATA, (LONG_PTR)val);
            PostMessage(hD, WM_SUBDLG_DONE, 0, 0);
            return 0;
        }
        if(LOWORD(wP)==IDCANCEL){
            SetWindowLongPtr(hD, GWLP_USERDATA, (LONG_PTR)-1);
            PostMessage(hD, WM_SUBDLG_DONE, 0, 0);
            return 0;
        }
        break;
    case WM_CLOSE:
        SetWindowLongPtr(hD, GWLP_USERDATA, (LONG_PTR)-1);
        PostMessage(hD, WM_SUBDLG_DONE, 0, 0);
        return 0;
    case WM_KEYDOWN:
        if((int)wP == VK_ESCAPE){
            SetWindowLongPtr(hD, GWLP_USERDATA, (LONG_PTR)-1);
            PostMessage(hD, WM_SUBDLG_DONE, 0, 0);
        }
        if((int)wP == VK_RETURN){
            SendMessage(hD, WM_COMMAND, MAKEWPARAM(IDOK,BN_CLICKED), 0);
        }
        break;
    }
    return DefWindowProc(hD, msg, wP, lP);
}

/* --- Procedure : liste deroulante (+ mot de passe optionnel) --- */
static LRESULT CALLBACK ProcDlgCombo(HWND hD, UINT msg,
                                       WPARAM wP, LPARAM lP){
    (void)lP;
    switch(msg){
    case WM_COMMAND:
        if(LOWORD(wP)==IDOK){
            HWND hCb = GetDlgItem(hD, IDC_DLG_COMBO);
            int sel  = (int)SendMessage(hCb, CB_GETCURSEL, 0, 0);
            int idx  = (sel==CB_ERR) ? -1 :
                       (int)SendMessage(hCb, CB_GETITEMDATA, sel, 0);
            SetWindowLongPtr(hD, GWLP_USERDATA, (LONG_PTR)idx);
            PostMessage(hD, WM_SUBDLG_DONE, 0, 0);
            return 0;
        }
        if(LOWORD(wP)==IDCANCEL){
            SetWindowLongPtr(hD, GWLP_USERDATA, (LONG_PTR)-1);
            PostMessage(hD, WM_SUBDLG_DONE, 0, 0);
            return 0;
        }
        break;
    case WM_CLOSE:
        SetWindowLongPtr(hD, GWLP_USERDATA, (LONG_PTR)-1);
        PostMessage(hD, WM_SUBDLG_DONE, 0, 0);
        return 0;
    case WM_KEYDOWN:
        if((int)wP == VK_ESCAPE){
            SetWindowLongPtr(hD, GWLP_USERDATA, (LONG_PTR)-1);
            PostMessage(hD, WM_SUBDLG_DONE, 0, 0);
        }
        break;
    }
    return DefWindowProc(hD, msg, wP, lP);
}

/* --- Boucle modale : attend WM_SUBDLG_DONE --- */
static void boucleModale(HWND hD){
    MSG m;
    while(GetMessage(&m, NULL, 0, 0)){
        if(m.hwnd==hD && m.message==WM_SUBDLG_DONE) break;
        /* Propager normalement tous les autres messages */
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
}

/* --- Nettoyage garanti dans tous les cas --- */
static void nettoyerDlg(HWND hD){
    if(hD && IsWindow(hD)) DestroyWindow(hD);
    EnableWindow(hWnd, TRUE);
    SetForegroundWindow(hWnd);
    BringWindowToTop(hWnd);
    UpdateWindow(hWnd);
}

/* --- Centre un sous-dialogue sur la fenetre principale --- */
static void centrerSurParent(HWND hD, int dw, int dh){
    RECT rp;
    GetWindowRect(hWnd, &rp);
    int x = rp.left + (rp.right  - rp.left - dw) / 2;
    int y = rp.top  + (rp.bottom - rp.top  - dh) / 2;
    SetWindowPos(hD, HWND_TOP, x, y, dw, dh, 0);
}

/* ============================================================
 * DIALOGUE : SAISIE D UN ID NUMERIQUE
 * Retourne l ID saisi, ou -1 si annule.
 * ============================================================ */
static int demanderID(const char *titre, const char *label){
    HINSTANCE hI = GetModuleHandle(NULL);
    int dw=335, dh=120;

    HWND hD = CreateWindowEx(WS_EX_DLGMODALFRAME,
        CLASS_DLG_ID, titre,
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, dw, dh,
        NULL, NULL, hI, NULL);
    if(!hD) return -1;

    centrerSurParent(hD, dw, dh);
    SetWindowLongPtr(hD, GWLP_USERDATA, (LONG_PTR)-1);

    /* Controles */
    HWND hL = CreateWindow("STATIC", label,
        WS_CHILD|WS_VISIBLE|SS_LEFT,
        12,14,305,18, hD,NULL,hI,NULL);
    HWND hE = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
        WS_CHILD|WS_VISIBLE|ES_NUMBER,
        12,36,95,26, hD,(HMENU)(INT_PTR)IDC_DLG_EDIT_ID,hI,NULL);
    HWND hOK= CreateWindow("BUTTON","OK",
        WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
        120,36,88,26, hD,(HMENU)IDOK,hI,NULL);
    HWND hAN= CreateWindow("BUTTON","Annuler",
        WS_CHILD|WS_VISIBLE,
        218,36,100,26, hD,(HMENU)IDCANCEL,hI,NULL);

    if(hFontNormal){
        SendMessage(hL, WM_SETFONT,(WPARAM)hFontNormal,TRUE);
        SendMessage(hE, WM_SETFONT,(WPARAM)hFontNormal,TRUE);
        SendMessage(hOK,WM_SETFONT,(WPARAM)hFontNormal,TRUE);
        SendMessage(hAN,WM_SETFONT,(WPARAM)hFontNormal,TRUE);
    }

    EnableWindow(hWnd, FALSE);
    ShowWindow(hD, SW_SHOW);
    UpdateWindow(hD);
    SetFocus(hE);

    boucleModale(hD);

    int resultat = (int)GetWindowLongPtr(hD, GWLP_USERDATA);
    nettoyerDlg(hD);
    return resultat;
}

/* ============================================================
 * DIALOGUE : LISTE DEROULANTE (+ mot de passe optionnel)
 *
 * itemIds[i]  = valeur retournee si l item i est choisi
 * labels[i]   = texte affiche pour chaque item
 * n           = nombre d items
 * avecMdp     = 1 pour afficher le champ mot de passe
 * mdpBuf/len  = buffer pour recuperer le mot de passe saisi
 *
 * Retourne l itemIds de l item choisi, ou -1 si annule.
 * ============================================================ */
static int demanderChoixListe(const char *titre, const char *label,
                               int *itemIds,
                               char labels[][160], int n,
                               int avecMdp,
                               char *mdpBuf, int mdpLen){
    if(n == 0){
        MessageBox(hWnd, "Aucun element disponible.",
                    titre, MB_ICONINFORMATION);
        return -1;
    }
    HINSTANCE hI = GetModuleHandle(NULL);
    int dh = avecMdp ? 190 : 140;
    int dw = 470;

    HWND hD = CreateWindowEx(WS_EX_DLGMODALFRAME,
        CLASS_DLG_COMBO, titre,
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, dw, dh,
        NULL, NULL, hI, NULL);
    if(!hD) return -1;

    centrerSurParent(hD, dw, dh);
    SetWindowLongPtr(hD, GWLP_USERDATA, (LONG_PTR)-1);

    HWND hL = CreateWindow("STATIC", label,
        WS_CHILD|WS_VISIBLE|SS_LEFT,
        12,10,440,18, hD,NULL,hI,NULL);
    HWND hCb= CreateWindow("COMBOBOX","",
        WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
        12,32,440,200, hD,(HMENU)(INT_PTR)IDC_DLG_COMBO,hI,NULL);

    /* Peupler la liste */
    for(int i = 0; i < n; i++){
        int pos = (int)SendMessage(hCb, CB_ADDSTRING, 0, (LPARAM)labels[i]);
        SendMessage(hCb, CB_SETITEMDATA, pos, (LPARAM)i);
    }
    SendMessage(hCb, CB_SETCURSEL, 0, 0);

    /* Champ mot de passe (optionnel) */
    HWND hLm = NULL, hEm = NULL;
    if(avecMdp){
        hLm = CreateWindow("STATIC","Mot de passe temporaire :",
            WS_CHILD|WS_VISIBLE, 12,76,200,18, hD,NULL,hI,NULL);
        hEm = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT","",
            WS_CHILD|WS_VISIBLE|ES_PASSWORD,
            218,74,222,26, hD,(HMENU)(INT_PTR)IDC_DLG_EDIT_MDP,hI,NULL);
        if(hFontNormal){
            SendMessage(hLm,WM_SETFONT,(WPARAM)hFontNormal,TRUE);
            SendMessage(hEm,WM_SETFONT,(WPARAM)hFontNormal,TRUE);
        }
    }

    int yb = avecMdp ? 118 : 78;
    HWND hOK= CreateWindow("BUTTON","Valider",
        WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
        12,yb,95,28, hD,(HMENU)IDOK,hI,NULL);
    HWND hAN= CreateWindow("BUTTON","Annuler",
        WS_CHILD|WS_VISIBLE,
        118,yb,95,28, hD,(HMENU)IDCANCEL,hI,NULL);

    if(hFontNormal){
        SendMessage(hL, WM_SETFONT,(WPARAM)hFontNormal,TRUE);
        SendMessage(hCb,WM_SETFONT,(WPARAM)hFontNormal,TRUE);
        SendMessage(hOK,WM_SETFONT,(WPARAM)hFontNormal,TRUE);
        SendMessage(hAN,WM_SETFONT,(WPARAM)hFontNormal,TRUE);
    }

    EnableWindow(hWnd, FALSE);
    ShowWindow(hD, SW_SHOW);
    UpdateWindow(hD);
    SetFocus(hCb);

    boucleModale(hD);

    /* Recuperer l index selectionne */
    int selIdx = (int)GetWindowLongPtr(hD, GWLP_USERDATA);

    /* Recuperer le mot de passe si necessaire */
    if(avecMdp && hEm && mdpBuf && selIdx >= 0)
        GetWindowText(hEm, mdpBuf, mdpLen);

    nettoyerDlg(hD);

    /* Traduire l index en valeur ID */
    if(selIdx < 0 || selIdx >= n) return -1;
    return itemIds[selIdx];
}

/* ============================================================
 * ACTIONS ADMIN — Miroir exact de administrateur.c
 * ============================================================ */

/* --- Activer / Desactiver un compte --- */
static void actionToggleCompte(void){
    int id = demanderID(
        "Activer / Desactiver un compte",
        "Entrez l'ID du compte (colonne ID de la liste) :");
    if(id < 0) return;

    if(id == sessionCourante.utilisateur.id){
        MessageBox(hWnd,
            "Impossible de modifier votre propre compte.",
            "Erreur", MB_ICONWARNING); return;
    }
    for(int i = 0; i < nbUtilisateurs; i++){
        if(listeUtilisateurs[i].id == id){
            if(listeUtilisateurs[i].role == ROLE_ADMINISTRATEUR){
                MessageBox(hWnd,
                    "Impossible de modifier un autre administrateur.",
                    "Erreur", MB_ICONWARNING); return;
            }
            listeUtilisateurs[i].actif = !listeUtilisateurs[i].actif;
            if(listeUtilisateurs[i].actif)
                listeUtilisateurs[i].nbEchecs = 0;
            sauvegarderUtilisateurs();
            char msg[120];
            sprintf(msg, "Compte de %s : %s.",
                listeUtilisateurs[i].prenom,
                listeUtilisateurs[i].actif ? "ACTIVE" : "DESACTIVE");
            MessageBox(hWnd, msg, "Succes", MB_ICONINFORMATION);
            return;
        }
    }
    MessageBox(hWnd,
        "Aucun utilisateur avec cet ID.",
        "Erreur", MB_ICONERROR);
}

/* --- Reactiver un compte verrouille --- */
static void actionReactiverCompte(void){
    int     ids[MAX_UTILISATEURS];
    char    lbs[MAX_UTILISATEURS][160];
    int n = 0;
    for(int i = 0; i < nbUtilisateurs; i++){
        if(!listeUtilisateurs[i].actif){
            ids[n] = listeUtilisateurs[i].id;
            sprintf(lbs[n], "ID:%d  %s %s  (echecs: %d)",
                listeUtilisateurs[i].id,
                listeUtilisateurs[i].prenom,
                listeUtilisateurs[i].nom,
                listeUtilisateurs[i].nbEchecs);
            n++;
        }
    }
    int id = demanderChoixListe(
        "Reactiver un compte verrouille",
        "Selectionnez le compte a reactiver :",
        ids, lbs, n, 0, NULL, 0);
    if(id < 0) return;

    for(int i = 0; i < nbUtilisateurs; i++){
        if(listeUtilisateurs[i].id == id){
            listeUtilisateurs[i].actif    = 1;
            listeUtilisateurs[i].nbEchecs = 0;
            sauvegarderUtilisateurs();
            char msg[120];
            sprintf(msg, "Compte de %s reactive avec succes.",
                listeUtilisateurs[i].prenom);
            MessageBox(hWnd, msg, "Succes", MB_ICONINFORMATION);
            return;
        }
    }
}

/* --- Supprimer un utilisateur --- */
static void actionSupprimerUtilisateur(void){
    int id = demanderID(
        "Supprimer un utilisateur",
        "Entrez l'ID de l'utilisateur a supprimer :");
    if(id < 0) return;

    if(id == sessionCourante.utilisateur.id){
        MessageBox(hWnd,
            "Impossible de supprimer votre propre compte.",
            "Erreur", MB_ICONWARNING); return;
    }
    int idx = -1;
    for(int i = 0; i < nbUtilisateurs; i++)
        if(listeUtilisateurs[i].id == id){ idx = i; break; }

    if(idx < 0){
        MessageBox(hWnd,
            "Aucun utilisateur avec cet ID.",
            "Erreur", MB_ICONERROR); return;
    }
    if(listeUtilisateurs[idx].role == ROLE_ADMINISTRATEUR){
        MessageBox(hWnd,
            "Impossible de supprimer un administrateur.",
            "Erreur", MB_ICONWARNING); return;
    }
    char confirm[200];
    sprintf(confirm,
        "Supprimer definitivement %s %s ?\nCette action est irreversible.",
        listeUtilisateurs[idx].prenom,
        listeUtilisateurs[idx].nom);
    if(MessageBox(hWnd, confirm, "Confirmation",
                   MB_YESNO|MB_ICONWARNING) == IDYES){
        for(int i = idx; i < nbUtilisateurs - 1; i++)
            listeUtilisateurs[i] = listeUtilisateurs[i+1];
        nbUtilisateurs--;
        sauvegarderUtilisateurs();
        MessageBox(hWnd,
            "Utilisateur supprime avec succes.",
            "Succes", MB_ICONINFORMATION);
    }
}

/* --- Supprimer un logement --- */
static void actionSupprimerLogement(void){
    int id = demanderID(
        "Supprimer un logement",
        "Entrez l'ID du logement a supprimer :");
    if(id < 0) return;

    int idx = -1;
    for(int i = 0; i < nbLogements; i++)
        if(listeLogements[i].id == id){ idx = i; break; }

    if(idx < 0){
        MessageBox(hWnd,
            "Aucun logement avec cet ID.",
            "Erreur", MB_ICONERROR); return;
    }
    char confirm[200];
    sprintf(confirm,
        "Supprimer le logement :\n\"%s\" ?\nCette action est irreversible.",
        listeLogements[idx].titre);
    if(MessageBox(hWnd, confirm, "Confirmation",
                   MB_YESNO|MB_ICONWARNING) == IDYES){
        for(int i = idx; i < nbLogements - 1; i++)
            listeLogements[i] = listeLogements[i+1];
        nbLogements--;
        sauvegarderLogements();
        MessageBox(hWnd,
            "Logement supprime avec succes.",
            "Succes", MB_ICONINFORMATION);
    }
}

/* --- Demandes de reinitialisation de mot de passe --- */
static void actionGererDemandes(void){
    /* Lire le fichier demandes_reinit.txt */
    FILE *f = fopen("data/demandes_reinit.txt", "r");

    int     ids[50];
    char    lbs[50][160];
    char    emails[50][TAILLE_EMAIL];
    int n = 0;

    if(f){
        char ligne[TAILLE_EMAIL + 20];
        while(fgets(ligne, sizeof(ligne), f) && n < 50){
            ligne[strcspn(ligne,"\n\r")] = '\0';
            char *sep = strchr(ligne, '|');
            if(!sep) continue;
            *sep = '\0';
            char *statut = sep + 1;
            if(strcmp(statut,"EN_ATTENTE") != 0) continue;

            /* Trouver l utilisateur par email */
            for(int i = 0; i < nbUtilisateurs; i++){
                if(strcmp(listeUtilisateurs[i].email, ligne)==0){
                    ids[n]  = i;
                    strncpy(emails[n], ligne, TAILLE_EMAIL-1);
                    sprintf(lbs[n], "%s %s  <%s>",
                        listeUtilisateurs[i].prenom,
                        listeUtilisateurs[i].nom,
                        listeUtilisateurs[i].email);
                    n++;
                    break;
                }
            }
        }
        fclose(f);
    }

    /* Si pas de fichier ou pas de demandes,
       afficher les comptes desactives comme dans la V2 */
    if(n == 0){
        for(int i = 0; i < nbUtilisateurs; i++){
            if(!listeUtilisateurs[i].actif){
                ids[n] = i;
                sprintf(emails[n], "%s", listeUtilisateurs[i].email);
                sprintf(lbs[n], "%s %s  <%s>",
                    listeUtilisateurs[i].prenom,
                    listeUtilisateurs[i].nom,
                    listeUtilisateurs[i].email);
                n++;
            }
        }
    }

    char mdpTemp[TAILLE_MDP] = {0};
    int idx = demanderChoixListe(
        "Demandes de reinitialisation de mot de passe",
        "Selectionnez le compte a traiter :",
        ids, lbs, n, 1, mdpTemp, TAILLE_MDP);
    if(idx < 0) return;

    if(strlen(mdpTemp) == 0){
        MessageBox(hWnd,
            "Le mot de passe temporaire ne peut pas etre vide.\n"
            "Operation annulee.",
            "Attention", MB_ICONWARNING); return;
    }

    strncpy(listeUtilisateurs[idx].motDePasse, mdpTemp, TAILLE_MDP-1);
    listeUtilisateurs[idx].motDePasse[TAILLE_MDP-1] = '\0';
    listeUtilisateurs[idx].actif    = 1;
    listeUtilisateurs[idx].nbEchecs = 0;
    sauvegarderUtilisateurs();

    /* Mettre a jour le fichier demandes si present */
    if(f){
        FILE *fin  = fopen("data/demandes_reinit.txt","r");
        FILE *fout = fopen("data/demandes_reinit_tmp.txt","w");
        if(fin && fout){
            char buf[TAILLE_EMAIL+20];
            while(fgets(buf,sizeof(buf),fin)){
                buf[strcspn(buf,"\n\r")] = '\0';
                char *s = strchr(buf,'|');
                if(s){ *s='\0';
                    if(strcmp(buf, emails[idx])==0)
                        fprintf(fout,"%s|TRAITE\n",buf);
                    else
                        fprintf(fout,"%s|%s\n",buf,s+1);
                }
            }
            fclose(fin); fclose(fout);
            remove("data/demandes_reinit.txt");
            rename("data/demandes_reinit_tmp.txt",
                   "data/demandes_reinit.txt");
        }
    }

    char msg[200];
    sprintf(msg,
        "Mot de passe temporaire attribue a %s.\n"
        "Compte reactive. L'utilisateur doit changer\n"
        "son mot de passe a la prochaine connexion.",
        listeUtilisateurs[idx].prenom);
    MessageBox(hWnd, msg, "Succes", MB_ICONINFORMATION);
}

/* ============================================================
 * CREATION DE LA PAGE ADMIN
 * Appelee depuis WM_CREATE de la fenetre principale.
 * Tous les controles sont caches par defaut (SW_HIDE).
 * ============================================================ */

/* Couleurs de la sidebar */
#define CLR_SIDEBAR    RGB(20, 70, 45)
#define CLR_BTN_BLEU   RGB(0, 110, 200)
#define CLR_BTN_ROUGE  RGB(190, 40, 55)
#define CLR_BTN_VERT   RGB(30, 140, 60)
#define CLR_BTN_ORANGE RGB(200, 130, 0)
#define CLR_BTN_GRIS   RGB(60,  65, 70)

/* Pinceaux locaux pour la sidebar */
static HBRUSH g_brSidebar = NULL;
static HBRUSH g_brBtnBleu  = NULL;
static HBRUSH g_brBtnRouge = NULL;

/* Structure pour les boutons colores de la sidebar */
#define MAX_BTNS_ADM 12
typedef struct { COLORREF bg, fg; WNDPROC origProc; } BtnAdmStyle;
static BtnAdmStyle g_btnStyles[MAX_BTNS_ADM];
static int         g_btnCount = 0;

/* Sous-classement des boutons pour les colorer */
static LRESULT CALLBACK BtnAdmProc(HWND h, UINT msg,
                                    WPARAM wP, LPARAM lP){
    int i = (int)GetWindowLongPtr(h, GWLP_USERDATA);
    if(i<0||i>=g_btnCount)
        return DefWindowProc(h,msg,wP,lP);
    if(msg == WM_PAINT){
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        HBRUSH br = CreateSolidBrush(g_btnStyles[i].bg);
        FillRect(dc, &rc, br); DeleteObject(br);
        char t[128]; GetWindowText(h, t, 128);
        SetTextColor(dc, g_btnStyles[i].fg);
        SetBkMode(dc, TRANSPARENT);
        HFONT of = (HFONT)SelectObject(dc,
            (HFONT)SendMessage(h,WM_GETFONT,0,0));
        DrawText(dc,t,-1,&rc,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        SelectObject(dc,of);
        EndPaint(h,&ps); return 0;
    }
    return CallWindowProcA(g_btnStyles[i].origProc,h,msg,wP,lP);
}

static HWND creerBoutonColore(const char *txt, int id,
                               int x, int y, int w, int h,
                               COLORREF bg, COLORREF fg){
    HWND b = CreateWindow("BUTTON", txt,
        WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
        x,y,w,h, hWnd,(HMENU)(INT_PTR)id,
        GetModuleHandle(NULL),NULL);
    if(hFontNormal) SendMessage(b,WM_SETFONT,(WPARAM)hFontNormal,TRUE);
    if(g_btnCount < MAX_BTNS_ADM){
        g_btnStyles[g_btnCount].bg = bg;
        g_btnStyles[g_btnCount].fg = fg;
        g_btnStyles[g_btnCount].origProc =
            (WNDPROC)SetWindowLongPtr(b,GWLP_WNDPROC,(LONG_PTR)BtnAdmProc);
        SetWindowLongPtr(b,GWLP_USERDATA,(LONG_PTR)g_btnCount);
        g_btnCount++;
    }
    return b;
}

/**
 * @brief Cree tous les controles de la page administrateur.
 * Appelee une seule fois dans WM_CREATE.
 */
void pageAdmin(void){
    /* Pinceaux */
    g_brSidebar = CreateSolidBrush(CLR_SIDEBAR);
    g_brBtnBleu  = CreateSolidBrush(CLR_BTN_BLEU);
    g_brBtnRouge = CreateSolidBrush(CLR_BTN_ROUGE);

    /* ── SIDEBAR ── */
    /* Logo / Titre */
    HWND hLogo = CreateWindow("STATIC","HabitatCam — Admin",
        WS_CHILD|WS_VISIBLE|SS_CENTER,
        0, BANDEAU_HAUTEUR, 205, 40,
        hWnd,NULL,GetModuleHandle(NULL),NULL);
    if(hFontBold) SendMessage(hLogo,WM_SETFONT,(WPARAM)hFontBold,TRUE);
    ajout(ctrlAdm, &nbAdm, hLogo);

    /* Nom de l admin connecte */
    char nomAdm[80];
    sprintf(nomAdm, "Connecte : %s",
        sessionCourante.utilisateur.prenom);
    HWND hNomAdm = CreateWindow("STATIC", nomAdm,
        WS_CHILD|WS_VISIBLE|SS_CENTER,
        0, BANDEAU_HAUTEUR+42, 205, 20,
        hWnd,NULL,GetModuleHandle(NULL),NULL);
    if(hFontNormal) SendMessage(hNomAdm,WM_SETFONT,(WPARAM)hFontNormal,TRUE);
    ajout(ctrlAdm, &nbAdm, hNomAdm);
    g_hSecTitle = hNomAdm; /* sera mis a jour par les vues */

    /* Boutons de navigation */
    int bx=8, by=BANDEAU_HAUTEUR+70, bw=189, bh=36, gap=5;

    #define BTN(txt,id,bg,fg) { \
        HWND bb=creerBoutonColore(txt,id,bx,by,bw,bh,bg,fg); \
        ajout(ctrlAdm,&nbAdm,bb); by+=bh+gap; }

    BTN("Utilisateurs",             ID_ADM_BTN_USERS,   CLR_BTN_BLEU,  RGB(255,255,255))
    BTN("Activer / Desactiver",     ID_ADM_BTN_TOGGLE,  CLR_BTN_GRIS,  RGB(255,255,255))
    BTN("Reactiver un compte",      ID_ADM_BTN_REACTIV, CLR_BTN_GRIS,  RGB(255,255,255))
    BTN("Supprimer utilisateur",    ID_ADM_BTN_DEL_USR, CLR_BTN_ROUGE, RGB(255,255,255))
    by += 8; /* separateur visuel */
    BTN("Logements",                ID_ADM_BTN_LOGS,    CLR_BTN_BLEU,  RGB(255,255,255))
    BTN("Supprimer logement",       ID_ADM_BTN_DEL_LOG, CLR_BTN_ROUGE, RGB(255,255,255))
    by += 8;
    BTN("Statistiques",             ID_ADM_BTN_STATS,   CLR_BTN_VERT,  RGB(255,255,255))
    BTN("Demandes reinit. mdp",     ID_ADM_BTN_DEMANDES,CLR_BTN_ORANGE,RGB(255,255,255))
    by += 8;
    BTN("Se deconnecter",           ID_ADM_BTN_DECONN,  CLR_BTN_ROUGE, RGB(255,255,255))
    #undef BTN

    /* ── ZONE PRINCIPALE ── */
    /* Titre de la section active */
    g_hSecTitle = CreateWindow("STATIC",
        "  Bienvenue dans le Panneau Administrateur",
        WS_CHILD|WS_VISIBLE|SS_LEFT,
        210, BANDEAU_HAUTEUR+4, 870, 36,
        hWnd,NULL,GetModuleHandle(NULL),NULL);
    if(hFontBold) SendMessage(g_hSecTitle,WM_SETFONT,(WPARAM)hFontBold,TRUE);
    ajout(ctrlAdm, &nbAdm, g_hSecTitle);

    /* ListView principal */
    INITCOMMONCONTROLSEX icc = {sizeof(icc),ICC_LISTVIEW_CLASSES};
    InitCommonControlsEx(&icc);

    g_hList = CreateWindowEx(WS_EX_CLIENTEDGE,
        WC_LISTVIEW, "",
        WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS|LVS_SINGLESEL,
        210, BANDEAU_HAUTEUR+44,
        870, FENETRE_HAUTEUR - BANDEAU_HAUTEUR - 88,
        hWnd,(HMENU)(INT_PTR)ID_ADM_LISTVIEW,
        GetModuleHandle(NULL),NULL);
    if(hFontNormal) SendMessage(g_hList,WM_SETFONT,(WPARAM)hFontNormal,TRUE);
    ListView_SetExtendedListViewStyle(g_hList,
        LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP);
    ajout(ctrlAdm, &nbAdm, g_hList);

    /* Barre de statut */
    g_hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
        WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP,
        0,0,0,0,
        hWnd,NULL,GetModuleHandle(NULL),NULL);
    ajout(ctrlAdm, &nbAdm, g_hStatus);

    /* Vue initiale */
    vueUtilisateurs();

    /* Tous caches par defaut — allerA() les affichera */
    for(int i = 0; i < nbAdm; i++)
        ShowWindow(ctrlAdm[i], SW_HIDE);
}

/**
 * @brief Gere les clics de boutons de la page admin.
 * Appelee depuis WM_COMMAND de la fenetre principale.
 *
 * @param id Identifiant du bouton clique.
 */
void gererClicAdmin(int id){
    switch(id){

    case ID_ADM_BTN_USERS:
        vueUtilisateurs();
        break;

    case ID_ADM_BTN_TOGGLE:
        actionToggleCompte();
        vueUtilisateurs();
        break;

    case ID_ADM_BTN_REACTIV:
        actionReactiverCompte();
        vueUtilisateurs();
        break;

    case ID_ADM_BTN_DEL_USR:
        actionSupprimerUtilisateur();
        vueUtilisateurs();
        break;

    case ID_ADM_BTN_LOGS:
        vueLogements();
        break;

    case ID_ADM_BTN_DEL_LOG:
        actionSupprimerLogement();
        vueLogements();
        break;

    case ID_ADM_BTN_STATS:
        vueStatistiques();
        break;

    case ID_ADM_BTN_DEMANDES:
        actionGererDemandes();
        vueUtilisateurs();
        break;

    case ID_ADM_BTN_DECONN:
        if(MessageBox(hWnd,
            "Voulez-vous vraiment vous deconnecter ?",
            "Deconnexion", MB_YESNO|MB_ICONQUESTION) == IDYES){
            deconnecterUtilisateur();
            allerA(PAGE_CONN);
        }
        break;
    }
}

/* ============================================================
 * GESTION DES COULEURS DE LA SIDEBAR ADMIN
 * A appeler dans WM_CTLCOLORSTATIC / WM_CTLCOLORBTN
 * de la fenetre principale si pageCourante == PAGE_ADMIN.
 *
 * Exemple dans WndProc :
 *   case WM_DRAWITEM:
 *       if(pageCourante==PAGE_ADMIN)
 *           return gererDrawItemAdmin(wParam, lParam);
 *       break;
 * ============================================================ */
LRESULT gererDrawItemAdmin(WPARAM wP, LPARAM lP){
    DRAWITEMSTRUCT *d = (DRAWITEMSTRUCT*)lP;
    if(d->CtlType != ODT_BUTTON) return FALSE;
    int i = (int)GetWindowLongPtr(d->hwndItem, GWLP_USERDATA);
    if(i<0||i>=g_btnCount) return FALSE;
    HBRUSH br = CreateSolidBrush(g_btnStyles[i].bg);
    FillRect(d->hDC, &d->rcItem, br); DeleteObject(br);
    char t[128]; GetWindowText(d->hwndItem, t, 128);
    SetTextColor(d->hDC, g_btnStyles[i].fg);
    SetBkMode(d->hDC, TRANSPARENT);
    HFONT of=(HFONT)SelectObject(d->hDC,
        (HFONT)SendMessage(d->hwndItem,WM_GETFONT,0,0));
    DrawText(d->hDC,t,-1,&d->rcItem,
              DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    SelectObject(d->hDC, of);
    return TRUE;
}

/* ============================================================
 *  COMPILATION STANDALONE
 *  Tout ce qui suit n est compile QUE si WIN_ADMIN_STANDALONE
 *  est defini. Le supprimer / commenter avant integration.
 * ============================================================ */
#ifdef WIN_ADMIN_STANDALONE

/* Procedure de la fenetre standalone */
static LRESULT CALLBACK WndProcStandalone(HWND hwnd, UINT msg,
                                           WPARAM wP, LPARAM lP){
    switch(msg){

    case WM_CREATE:
        hWnd = hwnd;

        /* Polices */
        hFontTitre = CreateFontA(24,0,0,0,FW_BOLD,0,0,0,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,"Arial");
        hFontSousTitre = CreateFontA(14,0,0,0,FW_NORMAL,TRUE,0,0,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,"Arial");
        hFontNormal = CreateFontA(14,0,0,0,FW_NORMAL,0,0,0,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,"Arial");
        hFontBold = CreateFontA(14,0,0,0,FW_BOLD,0,0,0,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,"Arial");

        /* Pinceaux */
        hBrFond      = CreateSolidBrush(CLR_VERT_CLAIR);
        hBrVert      = CreateSolidBrush(CLR_VERT_FONCE);
        hBrBlanc     = CreateSolidBrush(CLR_BLANC);
        hBrGrisClair = CreateSolidBrush(CLR_GRIS_CLAIR);

        /* Creer la page admin */
        pageAdmin();

        /* Afficher tous les controles admin */
        for(int i=0;i<nbAdm;i++) ShowWindow(ctrlAdm[i],SW_SHOW);
        return 0;

    case WM_PAINT:{
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT r; GetClientRect(hwnd, &r);

        /* Fond general */
        FillRect(hdc, &r, hBrFond);

        /* Sidebar verte foncee */
        RECT rs = {0, BANDEAU_HAUTEUR, 205, r.bottom};
        FillRect(hdc, &rs, g_brSidebar);

        /* Bandeau superieur */
        RECT rb = {0,0,r.right,BANDEAU_HAUTEUR};
        FillRect(hdc, &rb, hBrVert);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255,255,255));
        SelectObject(hdc, hFontBold);
        RECT tl = {20,0,400,BANDEAU_HAUTEUR};
        DrawText(hdc,"HabitatCam V3.0  —  Panneau Administrateur",
                  -1,&tl,DT_LEFT|DT_VCENTER|DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC:{
        HDC dc=(HDC)wP; HWND hC=(HWND)lP;
        RECT rc; GetWindowRect(hC,&rc);
        POINT p={rc.left,rc.top}; ScreenToClient(hwnd,&p);
        if(p.x < 205){
            SetBkColor(dc,CLR_SIDEBAR);
            SetTextColor(dc,RGB(255,255,255));
            return (LRESULT)g_brSidebar;
        }
        SetBkColor(dc,CLR_VERT_CLAIR);
        SetTextColor(dc,CLR_TEXTE);
        return (LRESULT)hBrFond;
    }

    case WM_DRAWITEM:
        return gererDrawItemAdmin(wP, lP);

    case WM_SIZE:{
        int W=LOWORD(lP), H=HIWORD(lP);
        if(g_hList)
            SetWindowPos(g_hList,NULL,210,BANDEAU_HAUTEUR+44,
                W-215, H-BANDEAU_HAUTEUR-88, SWP_NOZORDER);
        if(g_hStatus) SendMessage(g_hStatus,WM_SIZE,0,0);
        if(g_hSecTitle)
            SetWindowPos(g_hSecTitle,NULL,210,BANDEAU_HAUTEUR+4,
                W-215,36,SWP_NOZORDER);
        return 0;
    }

    case WM_COMMAND:
        gererClicAdmin((int)LOWORD(wP));
        return 0;

    case WM_DESTROY:
        if(hFontTitre)    DeleteObject(hFontTitre);
        if(hFontSousTitre)DeleteObject(hFontSousTitre);
        if(hFontNormal)   DeleteObject(hFontNormal);
        if(hFontBold)     DeleteObject(hFontBold);
        if(hBrFond)       DeleteObject(hBrFond);
        if(hBrVert)       DeleteObject(hBrVert);
        if(hBrBlanc)      DeleteObject(hBrBlanc);
        if(hBrGrisClair)  DeleteObject(hBrGrisClair);
        if(g_brSidebar)   DeleteObject(g_brSidebar);
        if(g_brBtnBleu)   DeleteObject(g_brBtnBleu);
        if(g_brBtnRouge)  DeleteObject(g_brBtnRouge);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wP, lP);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev,
                   LPSTR lpCmd, int nShow){
    (void)hPrev; (void)lpCmd;

    /* Enregistrer les classes des sous-dialogues */
    WNDCLASSEX wdID = {0};
    wdID.cbSize        = sizeof(wdID);
    wdID.lpfnWndProc   = ProcDlgID;
    wdID.hInstance     = hInst;
    wdID.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wdID.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wdID.lpszClassName = CLASS_DLG_ID;
    RegisterClassEx(&wdID);

    WNDCLASSEX wdCb = {0};
    wdCb.cbSize        = sizeof(wdCb);
    wdCb.lpfnWndProc   = ProcDlgCombo;
    wdCb.hInstance     = hInst;
    wdCb.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wdCb.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wdCb.lpszClassName = CLASS_DLG_COMBO;
    RegisterClassEx(&wdCb);

    /* Fenetre principale standalone */
    WNDCLASSEX wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProcStandalone;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = "HabitatCamAdminStandalone";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(WS_EX_APPWINDOW,
        "HabitatCamAdminStandalone",
        "HabitatCam V3.0  -  Panneau Administrateur",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        FENETRE_LARGEUR, FENETRE_HAUTEUR,
        NULL, NULL, hInst, NULL);

    if(!hwnd){
        MessageBox(NULL,"Impossible de creer la fenetre.",
                    "Erreur",MB_ICONERROR); return 1;
    }

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while(GetMessage(&msg,NULL,0,0)>0){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

#endif /* WIN_ADMIN_STANDALONE */
