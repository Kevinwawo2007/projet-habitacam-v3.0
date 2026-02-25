#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int id;
    char ville[50];
    char description[100];
    float prix;
    char nom_bailleur[50];
    char telephone_bailleur[20];
    int disponible; // 1 = disponible, 0 = reserve
} Logement;


/* ========================= */
/*      AJOUTER LOGEMENT     */
/* ========================= */
void ajouterLogement() {
    Logement l;
    FILE *f = fopen("logements.txt", "a");

    if (f == NULL) {
        printf("Erreur ouverture fichier\n");
        return;
    }

    printf("\nID du logement: ");
    scanf("%d", &l.id);
    getchar();

    printf("Ville: ");
    fgets(l.ville, sizeof(l.ville), stdin);
    l.ville[strcspn(l.ville, "\n")] = 0;

    printf("Description: ");
    fgets(l.description, sizeof(l.description), stdin);
    l.description[strcspn(l.description, "\n")] = 0;

    printf("Prix: ");
    scanf("%f", &l.prix);
    getchar();

    printf("Nom du bailleur: ");
    fgets(l.nom_bailleur, sizeof(l.nom_bailleur), stdin);
    l.nom_bailleur[strcspn(l.nom_bailleur, "\n")] = 0;

    printf("Telephone du bailleur: ");
    fgets(l.telephone_bailleur, sizeof(l.telephone_bailleur), stdin);
    l.telephone_bailleur[strcspn(l.telephone_bailleur, "\n")] = 0;

    l.disponible = 1;

    fprintf(f, "%d|%s|%s|%.2f|%s|%s|%d\n",
            l.id, l.ville, l.description,
            l.prix, l.nom_bailleur,
            l.telephone_bailleur,
            l.disponible);

    fclose(f);

    printf("\nAnnonce ajoutee avec succes !\n");
}


/* ========================= */
/*     AFFICHER LOGEMENTS    */
/* ========================= */
void afficherLogements() {
    Logement l;
    FILE *f = fopen("logements.txt", "r");

    if (f == NULL) {
        printf("\nAucune annonce disponible.\n");
        return;
    }

    while (fscanf(f, "%d|%49[^|]|%99[^|]|%f|%49[^|]|%19[^|]|%d\n",
                  &l.id, l.ville, l.description,
                  &l.prix, l.nom_bailleur,
                  l.telephone_bailleur,
                  &l.disponible) != EOF) {

        if (l.disponible == 1) {
            printf("\n=============================");
            printf("\nID: %d", l.id);
            printf("\nVille: %s", l.ville);
            printf("\nDescription: %s", l.description);
            printf("\nPrix: %.2f", l.prix);
            printf("\n=============================\n");
        }
    }

    fclose(f);
}


/* ========================= */
/*     CONTACTER BAILLEUR    */
/* ========================= */
void contacterBailleur() {
    int idRecherche;
    Logement l;
    FILE *f = fopen("logements.txt", "r");

    if (f == NULL) {
        printf("\nErreur ouverture fichier\n");
        return;
    }

    printf("\nEntrez l'ID du logement: ");
    scanf("%d", &idRecherche);

    while (fscanf(f, "%d|%49[^|]|%99[^|]|%f|%49[^|]|%19[^|]|%d\n",
                  &l.id, l.ville, l.description,
                  &l.prix, l.nom_bailleur,
                  l.telephone_bailleur,
                  &l.disponible) != EOF) {

        if (l.id == idRecherche && l.disponible == 1) {
            printf("\n----- Contact Bailleur -----");
            printf("\nNom: %s", l.nom_bailleur);
            printf("\nTelephone: %s\n", l.telephone_bailleur);
            fclose(f);
            return;
        }
    }

    printf("\nLogement non trouve ou indisponible.\n");
    fclose(f);
}


/* ========================= */
/*           MENU            */
/* ========================= */
void menu() {
    int choix;

    do {
        printf("\n========= MENU =========");
        printf("\n1. Ajouter une annonce (Bailleur)");
        printf("\n2. Voir annonces disponibles (Client)");
        printf("\n3. Contacter un bailleur");
        printf("\n4. Quitter");
        printf("\nChoix: ");
        scanf("%d", &choix);

        switch(choix) {
            case 1:
                ajouterLogement();
                break;
            case 2:
                afficherLogements();
                break;
            case 3:
                contacterBailleur();
                break;
            case 4:
                printf("\nAu revoir !\n");
                break;
            default:
                printf("\nChoix invalide.\n");
        }

    } while(choix != 4);
}


/* ========================= */
/*            MAIN           */
/* ========================= */
int main() {
    menu();
    return 0;
}
