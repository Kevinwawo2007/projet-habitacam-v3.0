/* ============================================================
 * @file    win_auth.h
 * @brief   Interface graphique d authentification — HabitatCam V3.0
 *
 * Declarations de toutes les fonctions, constantes et
 * variables partagees entre main_win32.c et win_auth.c.
 *
 * Architecture :
 *   main_win32.c  -> fenetre principale, boucle messages, navigation
 *   win_auth.c    -> pages connexion, inscription, menus roles
 *   win_auth.h    -> declarations communes (ce fichier)
 *
 * @version 3.0
 * @author  SOUOPGUI
 * ============================================================ */

#ifndef WIN_AUTH_H_INCLUDED
#define WIN_AUTH_H_INCLUDED

#include <windows.h>
#include "structures.h"

/* ============================================================
 * DIMENSIONS DE LA FENETRE
 * ============================================================ */
#define FENETRE_LARGEUR   900
#define FENETRE_HAUTEUR   640
#define BANDEAU_HAUTEUR    64

/* ============================================================
 * PALETTE DE COULEURS HABITATCAM
 * ============================================================ */
#define CLR_VERT_FONCE     RGB(27,  94,  32)
#define CLR_VERT_MOYEN     RGB(46,  125,  50)
#define CLR_VERT_CLAIR     RGB(232, 245, 233)
#define CLR_OR             RGB(249, 168,  37)
#define CLR_BLANC          RGB(255, 255, 255)
#define CLR_GRIS_CLAIR     RGB(245, 245, 245)
#define CLR_GRIS           RGB(117, 117, 117)
#define CLR_TEXTE          RGB(33,  33,  33)
#define CLR_ROUGE          RGB(198,  40,  40)
#define CLR_BORDURE        RGB(189, 189, 189)

/* ============================================================
 * IDENTIFIANTS DES CONTROLES — Page connexion
 * ============================================================ */
#define ID_EMAIL_CONN       101
#define ID_MDP_CONN         102
#define ID_BTN_CONN         103
#define ID_BTN_VOIR_MDP     104
#define ID_BTN_ALLER_INSCR  105
#define ID_BTN_ACCES_ADMIN  106

/* ============================================================
 * IDENTIFIANTS DES CONTROLES — Page inscription
 * ============================================================ */
#define ID_NOM              201
#define ID_PRENOM           202
#define ID_EMAIL_INSCR      203
#define ID_TEL              204
#define ID_MDP_INSCR        205
#define ID_MDP_CONF         206
#define ID_COMBO_ROLE       207
#define ID_BTN_CREER        208
#define ID_BTN_RETOUR_CONN  209

/* ============================================================
 * IDENTIFIANTS DES CONTROLES — Menu Locataire
 * ============================================================ */
#define ID_LOC_LOG          301
#define ID_LOC_RECH         302
#define ID_LOC_RES          303
#define ID_LOC_MESRES       304
#define ID_LOC_FAV          305
#define ID_LOC_NOTE         306
#define ID_LOC_PROFIL       307
#define ID_LOC_DECO         308

/* ============================================================
 * IDENTIFIANTS DES CONTROLES — Menu Bailleur
 * ============================================================ */
#define ID_BAIL_PUB         401
#define ID_BAIL_ANN         402
#define ID_BAIL_RES         403
#define ID_BAIL_NOTIF       404
#define ID_BAIL_PROFIL      405
#define ID_BAIL_DECO        406

/* ============================================================
 * IDENTIFIANTS DES CONTROLES — Panneau Admin
 * ============================================================ */
#define ID_ADM_USERS        501
#define ID_ADM_TOGGLE       502
#define ID_ADM_LOG          503
#define ID_ADM_STATS        504
#define ID_ADM_DEM          505
#define ID_ADM_PROFIL       506
#define ID_ADM_DECO         507

/* ============================================================
 * PAGES DE L APPLICATION
 *
 * L application fonctionne comme un empilement de pages.
 * Une seule page est visible a la fois. allerA(page) cache
 * la page courante et affiche la nouvelle.
 * ============================================================ */
typedef enum {
    PAGE_CONN  = 0,
    PAGE_INSCR,
    PAGE_LOC,
    PAGE_BAIL,
    PAGE_ADMIN
} Page;

/* ============================================================
 * VARIABLES GLOBALES PARTAGEES
 * Definies dans main_win32.c — utilisees dans win_auth.c
 * ============================================================ */
#define MAX_CTRL 50

extern HWND   hWnd;
extern HFONT  hFontTitre;
extern HFONT  hFontSousTitre;
extern HFONT  hFontNormal;
extern HFONT  hFontBold;
extern HBRUSH hBrFond;
extern HBRUSH hBrVert;
extern HBRUSH hBrBlanc;
extern HBRUSH hBrGrisClair;

extern HWND ctrlConn[MAX_CTRL];  extern int nbConn;
extern HWND ctrlInscr[MAX_CTRL]; extern int nbInscr;
extern HWND ctrlLoc[MAX_CTRL];   extern int nbLoc;
extern HWND ctrlBail[MAX_CTRL];  extern int nbBail;
extern HWND ctrlAdm[MAX_CTRL];   extern int nbAdm;

extern Page pageCourante;
extern BOOL mdpVisible;

/* ============================================================
 * FONCTIONS UTILITAIRES — Definies dans main_win32.c
 * ============================================================ */

/**
 * @brief Navigue vers une page en cachant la page courante.
 * @param p La page destination.
 * @return void
 */
void allerA(Page p);

/**
 * @brief Enregistre un handle dans le tableau d une page.
 * @param tab Tableau de handles.
 * @param nb  Compteur.
 * @param h   Handle a ajouter.
 * @return void
 */
void ajout(HWND *tab, int *nb, HWND h);

/**
 * @brief Cree un label texte statique.
 * @param txt   Texte a afficher.
 * @param x, y  Position.
 * @param w, h  Taille.
 * @param font  Police.
 * @return HWND Handle du label cree.
 */
HWND mkLabel(const char *txt, int x, int y,
             int w, int h, HFONT font);

/**
 * @brief Cree un champ de saisie stylee.
 * @param id    Identifiant du controle.
 * @param mdp   TRUE pour masquer la saisie.
 * @param x, y  Position.
 * @param w, h  Taille.
 * @return HWND Handle du champ cree.
 */
HWND mkEdit(int id, BOOL mdp, int x, int y, int w, int h);

/**
 * @brief Cree un bouton standard.
 * @param txt   Libelle.
 * @param id    Identifiant.
 * @param x, y  Position.
 * @param w, h  Taille.
 * @return HWND Handle du bouton cree.
 */
HWND mkBtn(const char *txt, int id,
           int x, int y, int w, int h);

/* ============================================================
 * FONCTIONS D INTERFACE — Definies dans win_auth.c
 * ============================================================ */

/**
 * @brief Cree les controles de la page connexion.
 * @return void
 */
void pageConnexion(void);

/**
 * @brief Cree les controles de la page inscription.
 * @return void
 */
void pageInscription(void);

/**
 * @brief Cree les controles du menu locataire.
 * @return void
 */
void pageLocataire(void);

/**
 * @brief Cree les controles du menu bailleur.
 * @return void
 */
void pageBailleur(void);

/**
 * @brief Cree les controles du panneau admin.
 * @return void
 */
void pageAdmin(void);

/**
 * @brief Traite la connexion depuis les champs graphiques.
 * @return void
 */
void traiterConnexion(void);

/**
 * @brief Traite l inscription depuis le formulaire graphique.
 * @return void
 */
void traiterInscription(void);

/**
 * @brief Dispatche les clics de boutons lies a l auth.
 * @param idBtn ID du bouton clique.
 * @return void
 */
void gererClicAuth(int idBtn);

#endif /* WIN_AUTH_H_INCLUDED */
