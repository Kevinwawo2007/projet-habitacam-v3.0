/* ============================================================
 * @file    matching.c
 * @brief   Module de recommandations intelligentes V2.0.
 *
 * Explications simples :
 *   - Ce module fonctionne comme un conseiller automatique.
 *   - Il se souvient de ce que le locataire a deja recherche.
 *   - A chaque connexion, il compare ces preferences avec
 *     tous les logements disponibles et donne un score.
 *   - Plus le score est eleve, plus le logement correspond.
 *
 * @version 2.0
 * @author  SOUOPGUI
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structures.h"
#include "auth.h"
#include "logement.h"
#include "matching.h"

/* ============================================================
 * UTILITAIRES INTERNES
 * ============================================================ */

/**
 * @brief Cree le dossier data/ si absent.
 * Compatible Windows et Linux.
 */
#ifndef _WIN32
#include <sys/stat.h>
#endif

static void creerDossier() {
#ifdef _WIN32
    system("if not exist data mkdir data");
#else
    mkdir("data", 0755);
#endif
}

/**
 * @brief Lit une ligne depuis le clavier.
 * Supprime le \n et \r en fin de chaine.
 */
static void lireLigne(const char *invite, char *buffer, int taille) {
    printf("%s", invite);
    if (fgets(buffer, taille, stdin))
        buffer[strcspn(buffer, "\n\r")] = '\0';
}




/* ============================================================
 * PERSISTANCE DU PROFIL
 * ============================================================ */

/**
 * @brief Sauvegarde le profil de recherche dans data/profils.txt.
 *
 * Si un profil existe deja pour ce locataire, il est remplace.
 * Sinon il est ajoute a la fin du fichier.
 *
 * Format d'une ligne :
 *   idLocataire|ville|budgetMax|surfaceMin
 *
 * Exemple :
 *   2|Yaounde|60000.00|25.00
 *
 * @param profil Le profil a sauvegarder.
 */
void sauvegarderProfil(ProfilRecherche profil) {
    creerDossier();

    /* Lire tous les profils existants */
    ProfilRecherche profils[MAX_UTILISATEURS];
    int             nbProfils = 0;
    int             trouve    = 0;
    int             i;

    FILE *f = fopen(FICHIER_PROFILS, "r");
    if (f != NULL) {
        ProfilRecherche p;
        while (fscanf(f, "%d|%49[^|]|%f|%f\n",
                      &p.idLocataire, p.ville,
                      &p.budgetMax, &p.surfaceMin) == 4) {
            if (p.idLocataire == profil.idLocataire) {
                /* Remplacer le profil existant par le nouveau */
                profils[nbProfils++] = profil;
                trouve = 1;
            } else {
                profils[nbProfils++] = p;
            }
            if (nbProfils >= MAX_UTILISATEURS) break;
        }
        fclose(f);
    }

    /* Si pas trouve, ajouter le nouveau profil */
    if (!trouve)
        profils[nbProfils++] = profil;

    /* Reecrire tout le fichier */
    f = fopen(FICHIER_PROFILS, "w");
    if (f == NULL) {
        printf("[ERREUR] Impossible de sauvegarder le profil.\n");
        return;
    }
    for (i = 0; i < nbProfils; i++) {
        fprintf(f, "%d|%s|%.2f|%.2f\n",
                profils[i].idLocataire,
                profils[i].ville,
                profils[i].budgetMax,
                profils[i].surfaceMin);
    }
    fclose(f);
}

/**
 * @brief Charge le profil de recherche d'un locataire.
 *
 * Lit data/profils.txt et cherche la ligne correspondant
 * a idLocataire. Si trouve, remplit la structure profil.
 *
 * @param idLocataire ID du locataire dont on veut le profil.
 * @param profil      Structure ou stocker le profil charge.
 * @return 1 si profil trouve et charge, 0 sinon.
 */
int chargerProfil(int idLocataire, ProfilRecherche *profil) {
    FILE *f = fopen(FICHIER_PROFILS, "r");
    if (f == NULL) return 0; /* Fichier absent = aucun profil */

    ProfilRecherche p;
    while (fscanf(f, "%d|%49[^|]|%f|%f\n",
                  &p.idLocataire, p.ville,
                  &p.budgetMax, &p.surfaceMin) == 4) {
        if (p.idLocataire == idLocataire) {
            *profil = p;
            fclose(f);
            return 1; /* Profil trouve */
        }
    }

    fclose(f);
    return 0; /* Profil non trouve */
}


/* ============================================================
 * CALCUL DU SCORE
 * ============================================================ */

/**
 * @brief Calcule le score de compatibilite entre un logement et un profil.
 *
 * Criteres evalues (chacun vaut environ 33 points) :
 *
 *   Critere 1 — Ville (33 points)
 *     Le nom de la ville du logement contient-il la ville
 *     recherchee ? On utilise strstr() pour une recherche
 *     partielle (ex: "Bastos" trouve "Yaounde-Bastos").
 *
 *   Critere 2 — Budget (34 points)
 *     Le prix mensuel du logement est-il <= budgetMax ?
 *     Si le budget est 0 (non defini), ce critere est ignore.
 *
 *   Critere 3 — Superficie (33 points)
 *     La superficie du logement est-elle >= surfaceMin ?
 *     Si la surface min est 0 (non definie), ce critere est ignore.
 *
 * Score total = somme des points obtenus sur 100.
 *
 * @param l      Le logement a evaluer.
 * @param profil Le profil de preferences du locataire.
 * @return Score entre 0 et 100.
 */
int calculerScore(const Logement *l, const ProfilRecherche *profil) {
    int score = 0;

    /* Critere 1 : ville (33 points)
     * strstr cherche si la ville du profil apparait dans
     * le nom de la ville du logement — insensible a la casse
     * non, mais partiel : "Yaounde" trouve "Yaounde-Bastos" */
    if (strlen(profil->ville) > 0 &&
        strstr(l->ville, profil->ville) != NULL) {
        score += 33;
    }

    /* Critere 2 : budget (34 points)
     * Le logement doit couter au maximum budgetMax FCFA.
     * Si budgetMax est 0, ce critere est ignore (non defini). */
    if (profil->budgetMax > 0) {
        if (l->prixMensuel <= profil->budgetMax)
            score += 34;
    } else {
        score += 34; /* Critere non defini = point accorde */
    }

    /* Critere 3 : superficie (33 points)
     * Le logement doit avoir au minimum surfaceMin m2.
     * Si surfaceMin est 0, ce critere est ignore (non defini). */
    if (profil->surfaceMin > 0) {
        if (l->superficie >= profil->surfaceMin)
            score += 33;
    } else {
        score += 33; /* Critere non defini = point accorde */
    }

    return score;
}


/* ============================================================
 * AFFICHAGE DES RECOMMANDATIONS
 * ============================================================ */

/**
 * @brief Affiche les logements recommandes au locataire connecte.
 *
 * Fonctionnement etape par etape :
 *   1. Charger le profil du locataire depuis data/profils.txt
 *      Si pas de profil -> ne rien afficher (premiere connexion)
 *   2. Pour chaque logement DISPONIBLE, calculer son score
 *   3. Trier les logements par score decroissant (tri a bulles)
 *   4. Afficher les MAX_RECOMMANDATIONS meilleurs
 *
 * Exemple d'affichage :
 *   === LOGEMENTS RECOMMANDES POUR VOUS ===
 *   Base sur : Yaounde | max 60000 FCFA | min 25 m2
 *   ----------------------------------------
 *   [100%] Studio Bastos    | 45000 FCFA | 35m2 | Yaounde
 *   [ 67%] Appart Melen     | 55000 FCFA | 20m2 | Yaounde
 *   [ 33%] Villa Omnisports | 70000 FCFA | 50m2 | Douala
 *
 * @param idLocataire ID du locataire connecte.
 */
void afficherRecommandations(int idLocataire) {
    ProfilRecherche profil;

    /* Etape 1 : charger le profil */
    if (!chargerProfil(idLocataire, &profil)) {
        /* Pas de profil = premiere connexion, ne rien afficher */
        return;
    }

    /* Etape 2 : calculer le score de chaque logement disponible */

    /* On cree deux tableaux paralleles :
     * scores[i] = score du logement listeLogements[idx[i]]
     * idx[i]    = index dans listeLogements[] */
    int scores[MAX_LOGEMENTS];
    int idx[MAX_LOGEMENTS];
    int nbCandidats = 0;
    int i, j;

    for (i = 0; i < nbLogements; i++) {
        /* On ne recommande que les logements disponibles */
        if (listeLogements[i].statut != STATUT_DISPONIBLE) continue;

        scores[nbCandidats] = calculerScore(&listeLogements[i], &profil);
        idx[nbCandidats]    = i;
        nbCandidats++;
    }

    if (nbCandidats == 0) return; /* Aucun logement disponible */

    /* Etape 3 : trier par score decroissant (tri a bulles simple)
     * Le tri a bulles compare chaque paire adjacente et les echange
     * si necessaire. Simple a comprendre pour des debutants. */
    for (i = 0; i < nbCandidats - 1; i++) {
        for (j = 0; j < nbCandidats - i - 1; j++) {
            if (scores[j] < scores[j + 1]) {
                /* Echanger les scores */
                int tmpScore  = scores[j];
                scores[j]     = scores[j + 1];
                scores[j + 1] = tmpScore;
                /* Echanger les indices correspondants */
                int tmpIdx = idx[j];
                idx[j]     = idx[j + 1];
                idx[j + 1] = tmpIdx;
            }
        }
    }

    /* Etape 4 : afficher les meilleurs resultats */
    int nbAffiches = nbCandidats < MAX_RECOMMANDATIONS
                     ? nbCandidats : MAX_RECOMMANDATIONS;

    printf("\n=== LOGEMENTS RECOMMANDES POUR VOUS ===\n");
    printf("Base sur  : %s", strlen(profil.ville) > 0
                              ? profil.ville : "toutes villes");
    if (profil.budgetMax > 0)
        printf(" | max %.0f FCFA", profil.budgetMax);
    if (profil.surfaceMin > 0)
        printf(" | min %.0f m2", profil.surfaceMin);
    printf("\n");
    printf("----------------------------------------\n");

    for (i = 0; i < nbAffiches; i++) {
        Logement *l = &listeLogements[idx[i]];
        printf("[%3d%%] %-20s | %6.0f FCFA | %5.1fm2 | %s\n",
               scores[i],
               l->titre,
               l->prixMensuel,
               l->superficie,
               l->ville);
    }
    printf("----------------------------------------\n");
    printf("Faites une recherche pour affiner ces resultats.\n");
}


/* ============================================================
 * RECHERCHE AVANCEE
 * ============================================================ */

/**
 * @brief Recherche avancee avec sauvegarde du profil.
 *
 * Cette fonction remplace rechercherLogement() pour les
 * locataires. Elle fait la meme chose mais en plus :
 *   - Elle sauvegarde les criteres dans data/profils.txt
 *   - Ces criteres serviront aux prochaines recommandations
 *
 * L'utilisateur peut laisser un champ vide (ou entrer 0)
 * pour ignorer ce critere dans la recherche.
 *
 * Exemple :
 *   Ville (vide=toutes)   : Yaounde
 *   Budget max (0=tous)   : 60000
 *   Surface min (0=tous)  : 25
 *   -> Affiche les logements correspondants
 *   -> Sauvegarde : 2|Yaounde|60000|25 dans profils.txt
 */
void rechercheAvancee(void) {
    ProfilRecherche profil;
    char            budgetBuf[32], surfBuf[32];
    int             trouve = 0;
    int             i;

    profil.idLocataire = sessionCourante.utilisateur.id;

    printf("\n=== RECHERCHE AVANCEE ===\n");
    printf("(Laissez vide ou entrez 0 pour ignorer un critere)\n");
    printf("------------------------------\n");

    /* Saisie des criteres */
    lireLigne("Ville (vide=toutes)  : ", profil.ville, TAILLE_VILLE);

    printf("Budget max en FCFA (0=tous) : ");
    lireLigne("", budgetBuf, sizeof(budgetBuf));
    profil.budgetMax = (atof(budgetBuf) > 0) ? atof(budgetBuf) : 0;

    printf("Surface min en m2  (0=tous) : ");
    lireLigne("", surfBuf, sizeof(surfBuf));
    profil.surfaceMin = (atof(surfBuf) > 0) ? atof(surfBuf) : 0;

    /* Effectuer la recherche */
    printf("\n--- RESULTATS ---\n");

    for (i = 0; i < nbLogements; i++) {
        Logement *l = &listeLogements[i];

        if (l->statut != STATUT_DISPONIBLE) continue;

        /* Verifier chaque critere saisi */
        int villeOk  = (strlen(profil.ville) == 0) ||
                       (strstr(l->ville, profil.ville) != NULL);
        int budgetOk = (profil.budgetMax == 0) ||
                       (l->prixMensuel <= profil.budgetMax);
        int surfOk   = (profil.surfaceMin == 0) ||
                       (l->superficie >= profil.surfaceMin);

        if (villeOk && budgetOk && surfOk) {
            printf("\nID    : %d", l->id);
            printf("\nTitre : %s", l->titre);
            printf("\nVille : %s - %s", l->ville, l->quartier);
            printf("\nPrix  : %.0f FCFA/mois", l->prixMensuel);
            printf("\nSurf  : %.1f m2 | %d pieces", l->superficie, l->nbPieces);
            printf("\n-----------------------------\n");
            trouve = 1;
        }
    }

    if (!trouve) {
        printf("Aucun logement trouve pour ces criteres.\n");
    }

    /* Sauvegarder le profil MEME si aucun resultat
     * pour que les futures recommandations en tiennent compte */
    sauvegarderProfil(profil);
    printf("\n[INFO] Vos criteres ont ete sauvegardes pour\n");
    printf("       les prochaines recommandations.\n");
}
