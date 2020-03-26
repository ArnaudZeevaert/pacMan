#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "GrilleSDL.h"
#include "Ressources.h"

#define BLOQUER -1

// Dimensions de la grille de jeu
#define NB_LIGNE 21
#define NB_COLONNE 17

// Macros utilisees dans le tableau tab
#define VIDE 0
#define MUR 1
#define PACMAN -2
#define PACGOM -3
#define SUPERPACGOM -4
#define BONUS -5
#define SPAWN 19
#define NID 29

// Autres macros
#define LENTREE 15
#define CENTREE 8

struct Coordonnees
{
    int x;
    int y;
};

typedef struct
{
    int L;
    int C;
    int couleur;
    int cache;
} S_FANTOME;

//Var global
int L, C, DIR, nbPacGom, delai, score, nbRouge, nbVert, nbMauve, nbOrange, mode, nbVies;
bool MAJ_Score;
pthread_mutex_t mutexTab, mutexNbPacGom, mutexDelai, mutexScore, mutexNbFantomes, mutexMode;
pthread_cond_t condNbPacGom, condScore, condNbFantomes, condMode;

pthread_t tidPacMan, tidEvent, tidPacGom, tidThreadScore, tidThreadBonus, tidThreadCompteurFantomes, tidThreadVies, tidThreadTimeOut;
pthread_key_t cle;
pthread_once_t controleur = PTHREAD_ONCE_INIT;

//Fonctions
void *threadPacMan(void *);
void *threadEvent(void *);
void *threadPacGom(void *);
void *threadScore(void *);
void *threadBonus(void *);
void *threadCompteurFantomes(void *);
void *threadFantome(void *);
void *threadVies(void *);
void *threadTimeOut(void *);

//int ChangementDeDir(S_FANTOME *, int);
void MonEffacerFantome(S_FANTOME *);
void MonDessineFantome(int, S_FANTOME *);
void MonDessineBlueFantome(int, S_FANTOME *);
void MonDessineBonus(int l, int c);
void MonDessinePacMan(int l, int c, int dir);
void MonDessinePacGom(int l, int c);
void MonDessineSuperPacGom(int l, int c);
void MonEffacerCarre(int l, int c);
void HandlerSIGCHLD(int);
void HandlerInt(int);
void HandlerHup(int);
void HandlerUsr1(int);
void HandlerUsr2(int);
void HandlerAlarm(int);
void ArmerTousLesSignaux(void);
void ArmerSIGCHLD(void);
void MasquerSIGCHLD(void);
void DeMasquerSIGCHLD(void);
void MasquerTousLesSignaux(void);
void DeMasquerTousLesSignaux(void);

void CreationDesRessources(void);
void DestructionDesRessources(void);
void initCle(void);
void destructeur(void *);

int tab[NB_LIGNE][NB_COLONNE] = {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                                 {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
                                 {1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1},
                                 {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                                 {1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1},
                                 {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1},
                                 {1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1},
                                 {1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1},
                                 {1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1},
                                 {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0},
                                 {1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1},
                                 {1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1},
                                 {1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1},
                                 {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
                                 {1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1},
                                 {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
                                 {1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1},
                                 {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1},
                                 {1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1},
                                 {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                                 {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

void DessineGrilleBase();
void Attente(int milli);

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    //EVENT_GRILLE_SDL event;
    //char ok;

    srand((unsigned)time(NULL));

    // Ouverture de la fenetre graphique
    printf("(MAIN %d) Ouverture de la fenetre graphique\n", pthread_self());
    fflush(stdout);
    if (OuvertureFenetreGraphique() < 0)
    {
        printf("Erreur de OuvrirGrilleSDL\n");
        fflush(stdout);
        exit(1);
    }

    DessineGrilleBase();

    //Debut de notre prog
    printf("[Main][Debut]Pid: %d\n", getpid());

    //Masquer tous les signaux pour le main
    MasquerTousLesSignaux();

    CreationDesRessources();

    //Au finial ne faire un join que sur l'event
    // pthread_join(tidPacMan,NULL);
    pthread_join(tidEvent, NULL);

    DestructionDesRessources();

    //Fin de notre prog

    // Fermeture de la fenetre
    printf("(MAIN %d) Fermeture de la fenetre graphique...", pthread_self());
    fflush(stdout);
    FermetureFenetreGraphique();
    printf("OK\n");
    fflush(stdout);

    exit(0);
}

void CreationDesRessources(void)
{
    printf("[CreationDesRessources] debut...");

    ArmerTousLesSignaux();

    //mutex
    if (pthread_mutex_init(&mutexTab, NULL))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_init sur mutexTab\n");
        exit(1);
    }
    if (pthread_mutex_init(&mutexNbPacGom, NULL))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_init sur mutexNbPacGom\n");
        exit(1);
    }
    if (pthread_mutex_init(&mutexDelai, NULL))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_init sur mutexDelai\n");
        exit(1);
    }
    if (pthread_mutex_init(&mutexScore, NULL))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_init sur mutexScore\n");
        exit(1);
    }
    if (pthread_mutex_init(&mutexNbFantomes, NULL))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_init sur mutexNbFantomes\n");
        exit(1);
    }
    if (pthread_mutex_init(&mutexMode, NULL))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_init sur mutexMode\n");
        exit(1);
    }

    //variable de condition
    if (pthread_cond_init(&condNbPacGom, NULL))
    {
        perror("[CreationDesRessources]error pthread_cond_init condNbPacGom");
        exit(1);
    }
    if (pthread_cond_init(&condScore, NULL))
    {
        perror("[CreationDesRessources]error pthread_cond_init condScore");
        exit(1);
    }
    if (pthread_cond_init(&condNbFantomes, NULL))
    {
        perror("[CreationDesRessources]error pthread_cond_init condNbFantomes");
        exit(1);
    }
    if (pthread_cond_init(&condMode, NULL))
    {
        perror("[CreationDesRessources]error pthread_cond_init condMode");
        exit(1);
    }

    //variable spécifique
    pthread_once(&controleur, initCle);

    //initialisation de la variable delai à 300 ms (ou 0,3 sec)
    if (pthread_mutex_lock(&mutexDelai))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_lock on mutexDelai\n");
        exit(1);
    }
    delai = 300;
    if (pthread_mutex_unlock(&mutexDelai))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_unlock on mutexDelai\n");
        exit(1);
    }

    //initialisation de la variable bool MAJ_Score
    if (pthread_mutex_lock(&mutexScore))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_lock on mutexDelai\n");
        exit(1);
    }
    MAJ_Score = false;
    if (pthread_mutex_unlock(&mutexScore))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_unlock on mutexDelai\n");
        exit(1);
    }

    //initialisation de la variable mode à 1
    if (pthread_mutex_lock(&mutexMode))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_lock on mutexMode\n");
        exit(1);
    }
    mode = 1;
    if (pthread_mutex_unlock(&mutexMode))
    {
        perror("[CreationDesRessources][Erreur]pthread_mutex_unlock on mutexMode\n");
        exit(1);
    }

    //initialisation du nombre de vies de pacMan à 3
    nbVies = 3;

    //Ouverture des threads
    pthread_create(&tidPacGom, NULL, threadPacGom, NULL);

    pthread_create(&tidThreadVies, NULL, threadVies, NULL);

    pthread_create(&tidEvent, NULL, threadEvent, NULL);

    pthread_create(&tidThreadScore, NULL, threadScore, NULL);

    pthread_create(&tidThreadBonus, NULL, threadBonus, NULL);

    pthread_create(&tidThreadCompteurFantomes, NULL, threadCompteurFantomes, NULL);

    printf("[CreationDesRessources] fin...");
}

void DestructionDesRessources(void)
{
    printf("[DestructionDesRessources] debut...");
    if (pthread_mutex_destroy(&mutexTab))
    {
        perror("[Main][Erreur]pthread_mutex_destroy on mutexTab\n");
        exit(1);
    }

    if (pthread_mutex_destroy(&mutexNbPacGom))
    {
        perror("[Main][Erreur]pthread_mutex_destroy on mutexNbPacGom\n");
        exit(1);
    }

    if (pthread_mutex_destroy(&mutexDelai))
    {
        perror("[Main][Erreur]pthread_mutex_destroy on mutexDelai\n");
        exit(1);
    }

    if (pthread_mutex_destroy(&mutexScore))
    {
        perror("[Main][Erreur]pthread_mutex_destroy on mutexScore\n");
        exit(1);
    }

    if (pthread_mutex_destroy(&mutexMode))
    {
        perror("[Main][Erreur]pthread_mutex_destroy on mutexMode\n");
        exit(1);
    }

    if (pthread_cond_destroy(&condNbPacGom))
    {
        perror("[DestructionDesRessources]error pthread_cond_destroy condNbPacGom");
        exit(1);
    }
    if (pthread_cond_destroy(&condScore))
    {
        perror("[DestructionDesRessources]error pthread_cond_destroy condScore");
        exit(1);
    }
    if (pthread_cond_destroy(&condNbFantomes))
    {
        perror("[DestructionDesRessources]error pthread_cond_destroy condNbFantomes");
        exit(1);
    }
    if (pthread_cond_destroy(&condMode))
    {
        perror("[DestructionDesRessources]error pthread_cond_destroy condMode");
        exit(1);
    }
    printf("[DestructionDesRessources] fin...");
}

//*********************************************************************************************
void Attente(int milli)
{
    struct timespec del;
    del.tv_sec = milli / 1000;
    del.tv_nsec = (milli % 1000) * 1000000;
    nanosleep(&del, NULL);
}

//*********************************************************************************************
void DessineGrilleBase()
{
    for (int l = 0; l < NB_LIGNE; l++)
        for (int c = 0; c < NB_COLONNE; c++)
        {
            if (tab[l][c] == VIDE)
                EffaceCarre(l, c);
            if (tab[l][c] == MUR)
                DessineMur(l, c);
        }
}

void *threadPacMan(void *param)
{
    printf("[threadPacMan][Debut]Tid: %d\n", pthread_self());
    int mangerPacGom, mangerSuperPacGom, mangerBonus, delaiLOCAL, nextL, nextC, mort, modifier, etat, res;
    bool spawnPacMan = false;

    //Etablier le mode de fermeture du thread
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &etat);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &etat);

    while (!spawnPacMan)
    {
        if (pthread_mutex_lock(&mutexTab))
        {

            perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexTab\n");
            exit(1);
        }

        if (tab[15][8] == VIDE)
        {
            MonDessinePacMan(15, 8, GAUCHE);
            DIR = GAUCHE;
            spawnPacMan = true;
        }

        if (pthread_mutex_unlock(&mutexTab))
        {
            perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexTab\n");
            exit(1);
        }
    }

    printf("[DEBUG] avant While1\n");

    while (1)
    {
        mangerPacGom = 0;
        mangerSuperPacGom = 0;
        mangerBonus = 0;
        mort = 0;
        modifier = 0;

        if (pthread_mutex_lock(&mutexDelai))
        {
            perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexDelai\n");
            exit(1);
        }
        delaiLOCAL = delai;
        if (pthread_mutex_unlock(&mutexDelai))
        {
            perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexDelai\n");
            exit(1);
        }
        Attente(delaiLOCAL);

        //sigprocmask --> pour "démasquer" tous les signaux
        DeMasquerTousLesSignaux();
        //sigprocmask --> pour "masquer" tous les signaux
        MasquerTousLesSignaux();

        if (pthread_mutex_lock(&mutexTab))
        {
            perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexTab\n");
            exit(1);
        }

        switch (DIR)
        {
        case HAUT:
            printf("[threadPacMan][DIR] --> HAUT\n");
            nextL = L - 1;
            nextC = C;
            break;
        case BAS:
            printf("[threadPacMan][DIR] --> BAS\n");
            nextL = L + 1;
            nextC = C;
            break;
        case GAUCHE:
            printf("[threadPacMan][DIR] --> GAUCHE\n");
            nextL = L;
            nextC = C - 1;
            if ((C - 1) < 0)
            {
                nextL = L;
                nextC = NB_COLONNE - 1;
            }
            break;
        case DROITE:
            printf("[threadPacMan][DIR] --> DROITE\n");
            nextL = L;
            nextC = C + 1;
            if ((C + 1) >= NB_COLONNE)
            {
                nextL = L;
                nextC = 0;
            }
            break;
        }

        if (pthread_mutex_lock(&mutexMode))
        {
            perror("[CreationDesRessources][Erreur]pthread_mutex_lock on mutexMode\n");
            exit(1);
        }
        //Modification
        if (tab[nextL][nextC] != MUR)
        {

            if (tab[nextL][nextC] == PACGOM)
            {
                mangerPacGom++;
            }
            else if (tab[nextL][nextC] == SUPERPACGOM)
            {
                mangerSuperPacGom++;
            }
            else if (tab[nextL][nextC] == BONUS)
            {
                mangerBonus++;
            }
            else if (tab[nextL][nextC] != VIDE && mode == 1)
            {
                printf("[threadPacMan]Mort: %d\n", tab[nextL][nextC]);
                mort++;
            }
            else if (tab[nextL][nextC] != VIDE && mode == 2)
            {
                printf("[threadPacMan]Manger Fantome: %d\n", tab[nextL][nextC]);
                pthread_kill(tab[nextL][nextC], SIGCHLD);
            }
            modifier++;
        }
        if (pthread_mutex_unlock(&mutexMode))
        {
            perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexMode\n");
            exit(1);
        }

        if (modifier && !mort)
        {
            MonEffacerCarre(L, C);
            MonDessinePacMan(nextL, nextC, DIR);
        }

        if (mangerBonus && !mort)
        {
            printf("[threadPacMan] mangerBonus\n");
            if (pthread_mutex_lock(&mutexScore))
            {
                perror("error mutex lock on mutexScore");
                exit(1);
            }
            score += 30;
            MAJ_Score = true;
            if (pthread_mutex_unlock(&mutexScore))
            {
                perror("error mutex unlock on mutexScore");
                exit(1);
            }
            pthread_cond_signal(&condScore);
        }
        if (mangerPacGom && !mort)
        {
            printf("[threadPacMan] mangerPacGom\n");
            if (pthread_mutex_lock(&mutexScore))
            {
                perror("error mutex lock on mutexScore");
                exit(1);
            }
            score++;
            MAJ_Score = true;
            if (pthread_mutex_unlock(&mutexScore))
            {
                perror("error mutex unlock on mutexScore");
                exit(1);
            }
            pthread_cond_signal(&condScore);
        }
        if (mangerSuperPacGom && !mort)
        {
            printf("[threadPacMan] mangerSuperPacGom\n");
            if (pthread_mutex_lock(&mutexScore))
            {
                perror("error mutex lock on mutexScore");
                exit(1);
            }
            score += 5;
            MAJ_Score = true;
            if (pthread_mutex_unlock(&mutexScore))
            {
                perror("error mutex unlock on mutexScore");
                exit(1);
            }
            pthread_cond_signal(&condScore);

            //Fantomes Comestibles
            if ((res = alarm(0)) == 0)
            {
                if (pthread_mutex_lock(&mutexMode))
                {
                    perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexMode\n");
                    exit(1);
                }
                mode = 2;
                if (pthread_mutex_unlock(&mutexMode))
                {
                    perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexMode\n");
                    exit(1);
                }
                pthread_cond_signal(&condMode);

                if (pthread_create(&tidThreadTimeOut, NULL, threadTimeOut, NULL))
                {
                    printf("[threadPacMan: %d][Erreur]pthread_create on threadTimeOut\n", pthread_self());
                    exit(1);
                }
            }
            else
            {
                //Si une alarm est deja en cours on l'arrete et recupere le nombre de secondes restantes
                pthread_kill(tidThreadTimeOut, SIGQUIT);
                if (pthread_create(&tidThreadTimeOut, NULL, threadTimeOut, &res))
                {
                    printf("[threadPacMan: %d][Erreur]pthread_create on threadTimeOut\n", pthread_self());
                    exit(1);
                }
            }
        }
        if ((mangerPacGom || mangerSuperPacGom) && !mort)
        {
            printf("[threadPacMan] nbPacGom--\n");
            if (pthread_mutex_lock(&mutexNbPacGom))
            {
                perror("error mutex lock on mutexNbPacGom");
                exit(1);
            }
            nbPacGom--;
            if (pthread_mutex_unlock(&mutexNbPacGom))
            {
                perror("error mutex unlock on mutexNbPacGom");
                exit(1);
            }
            pthread_cond_signal(&condNbPacGom);
        }
        if (mort)
        {
            //pthread_testcancel();
            MonEffacerCarre(L, C);
            pthread_cancel(tidPacMan);
        }

        if (pthread_mutex_unlock(&mutexTab))
        {
            perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexTab\n");
            exit(1);
        }
        pthread_testcancel();
    }

    printf("[threadPacMan][Fin]\n");
    pthread_exit(NULL);
}

void *threadEvent(void *param)
{
    printf("[threadEvent][Debut]Tid: %d\n", pthread_self());

    MasquerTousLesSignaux();

    EVENT_GRILLE_SDL event;

    while (1)
    {
        event = ReadEvent();

        switch (event.type)
        {
        case CROIX:
            pthread_exit(NULL);
            break;
        case CLAVIER:
            switch (event.touche)
            {
            case KEY_UP:
                //sigusr1
                //printf("\n[DEBUG][threadEvent] : KEY_UP\n");
                pthread_kill(tidPacMan, SIGUSR1);
                break;
            case KEY_DOWN:
                //sigusr2
                //printf("\n[DEBUG][threadEvent] : KEY_DOWN\n");
                pthread_kill(tidPacMan, SIGUSR2);
                break;
            case KEY_LEFT:
                //sigint
                //printf("\n[DEBUG][threadEvent] : KEY_LEFT\n");
                pthread_kill(tidPacMan, SIGINT);
                break;
            case KEY_RIGHT:
                //sighup
                //printf("\n[DEBUG][threadEvent] : KEY_RIGHT\n");
                pthread_kill(tidPacMan, SIGHUP);
                break;
            }
            break;
        }
    }

    printf("[threadEvent][Fin]\n");
}

void *threadPacGom(void *param)
{
    MasquerTousLesSignaux();
    int val, niveau = 0;
    while (1)
    {
        if (pthread_mutex_lock(&mutexTab))
        {
            perror("[threadPacGom][Erreur]pthread_mutex_lock on mutexTab\n");
            exit(1);
        }
        if (pthread_mutex_lock(&mutexNbPacGom))
        {
            perror("error mutex lock");
            exit(1);
        }

        DessineChiffre(14, 22, niveau);

        for (int x = 0; x < NB_LIGNE; x++)
        {
            for (int y = 0; y < NB_COLONNE; y++)
            {
                if (tab[x][y] == VIDE)
                {
                    if (!(x == 15 && y == 8) && !(x == 8 && y == 8) && !(x == 9 && y == 8) && !(x == 2 && y == 1) && !(x == 2 && y == 15) && !(x == 15 && y == 1) && !(x == 15 && y == 15))
                    {
                        DessinePacGom(x, y);
                        tab[x][y] = PACGOM;
                        nbPacGom++;
                    }
                }
            }
        }
        DessineSuperPacGom(2, 1);
        tab[2][1] = SUPERPACGOM;
        DessineSuperPacGom(2, 15);
        tab[2][15] = SUPERPACGOM;
        DessineSuperPacGom(15, 1);
        tab[15][1] = SUPERPACGOM;
        DessineSuperPacGom(15, 15);
        tab[15][15] = SUPERPACGOM;
        nbPacGom += 4;

        if (pthread_mutex_unlock(&mutexNbPacGom))
        {
            perror("error mutex lock");
            exit(1);
        }
        if (pthread_mutex_unlock(&mutexTab))
        {
            perror("[threadPacGom][Erreur]pthread_mutex_unlock on mutexTab\n");
            exit(1);
        }

        if (pthread_mutex_lock(&mutexNbPacGom))
        {
            perror("[threadPacGom][Erreur] error mutex lock on mutexNbPacGom");
            exit(1);
        }
        printf("\n1 DEBUG nbPacGom = %d", nbPacGom);
        while (nbPacGom > 0)
        {
            pthread_cond_wait(&condNbPacGom, &mutexNbPacGom);

            printf("\nDEBUG nbPacGom = %d", nbPacGom);
            val = nbPacGom / 100; //centaine
            DessineChiffre(12, 22, val);
            val = (nbPacGom / 10) % 10; //dizaine
            DessineChiffre(12, 23, val);
            val = nbPacGom % 10; //unité
            DessineChiffre(12, 24, val);
        }
        if (pthread_mutex_unlock(&mutexNbPacGom))
        {
            perror("[threadPacGom][Erreur] error mutex lock on mutexNbPacGom");
            exit(1);
        }

        niveau++;
        DessineChiffre(14, 22, niveau);

        if (pthread_mutex_lock(&mutexDelai))
        {
            perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexDelai\n");
            exit(1);
        }
        delai = delai / 2;
        if (pthread_mutex_unlock(&mutexDelai))
        {
            perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexDelai\n");
            exit(1);
        }
    }
    pthread_exit(NULL);
}

void *threadScore(void *param)
{
    MasquerTousLesSignaux();
    int val;
    while (1)
    {
        if (pthread_mutex_lock(&mutexScore))
        {
            perror("[threadScore][Erreur] error mutex lock on mutexScore");
            exit(1);
        }
        while (MAJ_Score == false)
        {
            pthread_cond_wait(&condScore, &mutexScore);
        }

        printf("\nDEBUG [threadScore] score  = %d", score);

        val = score / 1000; //milliers
        DessineChiffre(16, 22, val);
        val = score / 100; //centaine
        DessineChiffre(16, 23, val);
        val = (score / 10) % 10; //dizaine
        DessineChiffre(16, 24, val);
        val = score % 10; //unité
        DessineChiffre(16, 25, val);

        MAJ_Score = false;
        if (pthread_mutex_unlock(&mutexScore))
        {
            perror("[threadScore][Erreur]pthread_mutex_unlock on mutexScore\n");
            exit(1);
        }
    }
    pthread_exit(NULL);
}

void *threadBonus(void *param)
{
    MasquerTousLesSignaux();
    int tempsAleatoire, x, y;
    struct Coordonnees CoordonneesBonus;
    bool BonusOK;

    while (1)
    {
        BonusOK = false;
        //attendre un temps aléatoire entre 10 et 20 sec
        //nombreAleatoire = (rand() % (MAX - MIN + 1)) + MIN;
        tempsAleatoire = (rand() % (20 - 10 + 1)) + 10;
        printf("\n[DEBUG][threadBonus] nombre aleatoire entre 10 et 20 = %d", tempsAleatoire);
        tempsAleatoire = tempsAleatoire * 1000;
        Attente(tempsAleatoire);

        if (pthread_mutex_lock(&mutexTab))
        {
            perror("[threadBonus][Erreur]pthread_mutex_lock on mutexTab\n");
            exit(1);
        }

        //rechercher une case vide
        while (!BonusOK)
        {
            x = (rand() % (NB_LIGNE - 0 + 1)) + 0;
            y = (rand() % (NB_COLONNE - 0 + 1)) + 0;

            if (tab[x][y] == VIDE)
            {
                CoordonneesBonus.x = x;
                CoordonneesBonus.y = y;
                MonDessineBonus(x, y);
                BonusOK = true;
            }
        }

        if (pthread_mutex_unlock(&mutexTab))
        {
            perror("[threadBonus][Erreur]pthread_mutex_unlock on mutexTab\n");
            exit(1);
        }

        if (BonusOK)
        {
            //attendre 10 sec
            Attente(10000);

            if (pthread_mutex_lock(&mutexTab))
            {
                perror("[threadBonus][Erreur]pthread_mutex_lock on mutexTab\n");
                exit(1);
            }

            //si au bout de 10 sec le bonnus n'a pas été mangé, on le supprime
            if (tab[CoordonneesBonus.x][CoordonneesBonus.y] == BONUS)
            {
                MonEffacerCarre(CoordonneesBonus.x, CoordonneesBonus.y);
            }

            if (pthread_mutex_unlock(&mutexTab))
            {
                perror("[threadBonus][Erreur]pthread_mutex_unlock on mutexTab\n");
                exit(1);
            }
        }
    }
    pthread_exit(NULL);
}

void *threadCompteurFantomes(void *param)
{
    S_FANTOME *paramThreadFantome;
    //pthread_t tidFantome;

    MasquerTousLesSignaux();

    while (1)
    {
        //pour ne pas créer des fantomes lorsque l'on est en mode 2
        if (pthread_mutex_lock(&mutexMode))
        {
            printf("[threadCompteurFantomes: %d][Erreur]pthread_mutex_lock on mutexMode", pthread_self());
            exit(1);
        }
        while (mode == 2)
        {
            pthread_cond_wait(&condMode, &mutexMode);
        }
        if (pthread_mutex_unlock(&mutexMode))
        {
            printf("[threadCompteurFantomes: %d][Erreur]pthread_mutex_unlock on mutexMode", pthread_self());
            exit(1);
        }

        if (pthread_mutex_lock(&mutexNbFantomes))
        {
            perror("[threadCompteurFantomes][Erreur]pthread_mutex_lock on mutexNbFantomes");
            exit(1);
        }

        while (nbRouge == 2 && nbVert == 2 && nbOrange == 2 && nbMauve == 2)
        {
            pthread_cond_wait(&condNbFantomes, &mutexNbFantomes);
        }

        while (nbRouge < 2)
        {
            paramThreadFantome = (S_FANTOME *)malloc(sizeof(S_FANTOME));
            //paramThreadFantome = new S_FANTOME;
            paramThreadFantome->L = 9;
            paramThreadFantome->C = 8;
            paramThreadFantome->couleur = ROUGE;
            paramThreadFantome->cache = 0;
            pthread_create(NULL, NULL, threadFantome, paramThreadFantome);
            nbRouge++;
        }
        while (nbVert < 2)
        {
            paramThreadFantome = (S_FANTOME *)malloc(sizeof(S_FANTOME));
            //paramThreadFantome = new S_FANTOME;
            paramThreadFantome->L = 9;
            paramThreadFantome->C = 8;
            paramThreadFantome->couleur = VERT;
            paramThreadFantome->cache = 0;
            pthread_create(NULL, NULL, threadFantome, paramThreadFantome);
            nbVert++;
        }
        while (nbOrange < 2)
        {
            paramThreadFantome = (S_FANTOME *)malloc(sizeof(S_FANTOME));
            //paramThreadFantome = new S_FANTOME;
            paramThreadFantome->L = 9;
            paramThreadFantome->C = 8;
            paramThreadFantome->couleur = ORANGE;
            paramThreadFantome->cache = 0;
            pthread_create(NULL, NULL, threadFantome, paramThreadFantome);
            nbOrange++;
        }
        while (nbMauve < 2)
        {
            paramThreadFantome = (S_FANTOME *)malloc(sizeof(S_FANTOME));
            //paramThreadFantome = new S_FANTOME;
            paramThreadFantome->L = 9;
            paramThreadFantome->C = 8;
            paramThreadFantome->couleur = MAUVE;
            paramThreadFantome->cache = 0;
            pthread_create(NULL, NULL, threadFantome, paramThreadFantome);
            nbMauve++;
        }

        if (pthread_mutex_unlock(&mutexNbFantomes))
        {
            perror("[threadCompteurFantomes][Erreur]pthread_mutex_unlock on mutexNbFantomes");
            exit(1);
        }
    }
    pthread_exit(NULL);
}

void *TerminaisonFantome(void *param)
{
    int mangerPacGom = 0, mangerSuperPacGom = 0, mangerBonus = 0;
    //Incrementer le score de 50
    if (pthread_mutex_lock(&mutexScore))
    {
        printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_lock on mutexScore", pthread_self());
        exit(1);
    }
    score += 50;
    MAJ_Score = true;
    if (pthread_mutex_unlock(&mutexScore))
    {
        printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_unlock on mutexScore", pthread_self());
        exit(1);
    }
    pthread_cond_signal(&condScore);

    //Incrementer le socre avec le contenu du cache
    S_FANTOME *pFantome = NULL;
    pFantome = (S_FANTOME *)pthread_getspecific(cle);
    if (pFantome == NULL)
    {
        printf("[TerminaisonFantomes: %d]pFantome == NULL\n", pthread_self());
        exit(1);
    }

    switch (pFantome->cache)
    {
    case VIDE:
        break;
    case PACGOM:
        mangerPacGom++;
        break;
    case SUPERPACGOM:
        mangerSuperPacGom++;
        break;
    case BONUS:
        mangerBonus++;
        break;
    default:
        printf("[TerminaisonFantomes: %d]switch(pFantome->cache) default\n", pthread_self());
        break;
    }
    if (mangerBonus)
    {
        if (pthread_mutex_lock(&mutexScore))
        {
            perror("error mutex lock on mutexScore");
            exit(1);
        }
        score += 30;
        MAJ_Score = true;
        if (pthread_mutex_unlock(&mutexScore))
        {
            perror("error mutex unlock on mutexScore");
            exit(1);
        }
        pthread_cond_signal(&condScore);
    }
    if (mangerPacGom)
    {
        if (pthread_mutex_lock(&mutexScore))
        {
            perror("error mutex lock on mutexScore");
            exit(1);
        }
        score++;
        MAJ_Score = true;
        if (pthread_mutex_unlock(&mutexScore))
        {
            perror("error mutex unlock on mutexScore");
            exit(1);
        }
        pthread_cond_signal(&condScore);
    }
    if (mangerSuperPacGom)
    {
        if (pthread_mutex_lock(&mutexScore))
        {
            perror("error mutex lock on mutexScore");
            exit(1);
        }
        score += 5;
        MAJ_Score = true;
        if (pthread_mutex_unlock(&mutexScore))
        {
            perror("error mutex unlock on mutexScore");
            exit(1);
        }
        pthread_cond_signal(&condScore);
    }
    if (mangerPacGom || mangerSuperPacGom)
    {
        if (pthread_mutex_lock(&mutexNbPacGom))
        {
            perror("error mutex lock on mutexNbPacGom");
            exit(1);
        }
        nbPacGom--;
        if (pthread_mutex_unlock(&mutexNbPacGom))
        {
            perror("error mutex unlock on mutexNbPacGom");
            exit(1);
        }
        pthread_cond_signal(&condNbPacGom);
    }

    //Decrementer le bon fantome
    if (pthread_mutex_lock(&mutexNbFantomes))
    {
        printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_lock on mutexNbFantomes", pthread_self());
        exit(1);
    }

    if (pFantome->couleur == ROUGE)
        nbRouge--;
    else if (pFantome->couleur == VERT)
        nbVert--;
    else if (pFantome->couleur == ORANGE)
        nbOrange--;
    else if (pFantome->couleur == MAUVE)
        nbMauve--;

    if (pthread_mutex_unlock(&mutexNbFantomes))
    {
        printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_unlock on mutexNbFantomes", pthread_self());
        exit(1);
    }
    pthread_cond_signal(&condNbFantomes);
    pthread_exit(NULL);
}

void *threadFantome(void *param)
{
    printf("[threadFantome][Debut]Tid: %d\n", pthread_self());

    int dirFantome = HAUT, min = 0, max = 0, newDir = 0, changeDir = 0, tuerPacman = 0, continuer = 0, PossibiliteDeNouvelleDir[4] = {0}, newDirOk = 0, bloquer = 0, nextC = 0, nextL = 0, delaiFantome;
    S_FANTOME *paramLocal;
    bool spawnOK = false;

    pthread_cleanup_push(TerminaisonFantome, NULL);

    //si la mémoire spécifique n'existe pas encore
    if ((paramLocal = (S_FANTOME *)pthread_getspecific(cle)) == NULL)
    {
        paramLocal = (S_FANTOME *)param;
        pthread_setspecific(cle, paramLocal);
    }

    MasquerTousLesSignaux();
    while (!spawnOK)
    {
        if (pthread_mutex_lock(&mutexTab))
        {
            perror("[threadFantome][Erreur] error mutex lock on mutexTab");
            exit(1);
        }
        if (tab[9][8] == VIDE && tab[8][8] == VIDE)
        {
            paramLocal->cache = VIDE;
            if (pthread_mutex_lock(&mutexMode))
            {
                perror("[threadFantome][Erreur] error mutex lock on mutexMode");
                exit(1);
            }
            if (mode == 1)
                MonDessineFantome(dirFantome, paramLocal);
            else
                MonDessineBlueFantome(dirFantome, paramLocal);

            spawnOK = true;
            if (pthread_mutex_unlock(&mutexMode))
            {
                perror("[threadFantome][Erreur] error mutex lock on mutexMode");
                exit(1);
            }
        }
        if (pthread_mutex_unlock(&mutexTab))
        {
            perror("[threadFantome][Erreur] error mutex lock on mutexTab");
            exit(1);
        }
    }
    DeMasquerSIGCHLD();

    while (1)
    {
        //printf("[threadFantomes: %d]Boucle\n",pthread_self());
        //system("clear");
        //AfficheTab();
        newDir = 0;
        tuerPacman = 0;
        continuer = 0;
        nextL = 0;
        nextC = 0;
        PossibiliteDeNouvelleDir[0] = 0;
        PossibiliteDeNouvelleDir[1] = 0;
        PossibiliteDeNouvelleDir[2] = 0;
        PossibiliteDeNouvelleDir[3] = 0;
        max = 0;
        changeDir = 0;
        newDirOk = 0;
        bloquer = 0;

        if (pthread_mutex_lock(&mutexDelai))
        {
            perror("[threadFantome][Erreur]pthread_mutex_lock on mutexDelai\n");
            exit(1);
        }
        delaiFantome = (delai / 3) * 5;
        if (pthread_mutex_unlock(&mutexDelai))
        {
            perror("[threadFantome][Erreur]pthread_mutex_unlock on mutexDelai\n");
            exit(1);
        }
        Attente(delaiFantome);

        DeMasquerSIGCHLD();

        MasquerTousLesSignaux();

        if (pthread_mutex_lock(&mutexTab))
        {
            perror("[threadFantomes][Erreur]pthread_mutex_lock on mutexTab\n");
            exit(1);
        }
        if (pthread_mutex_lock(&mutexMode))
        {
            perror("[threadFantomes][Erreur]pthread_mutex_lock on mutexMode\n");
            exit(1);
        }

        //Restitution du Cache
        MonEffacerFantome(paramLocal);

        //Mouvement
        switch (dirFantome)
        {
        case HAUT:
            nextL = paramLocal->L - 1;
            nextC = paramLocal->C;
            break;
        case BAS:
            nextL = paramLocal->L + 1;
            nextC = paramLocal->C;
            break;
        case GAUCHE:
            if ((paramLocal->C - 1) < 0 && tab[paramLocal->L][NB_COLONNE - 1] < 1)
            //cas spécifique où la case suivante est à l'autre bout du tab
            //si la case suivante n'est pas un MUR ou un autre FANTOME
            {
                nextL = paramLocal->L;
                nextC = NB_COLONNE - 1;
            }
            else
            {
                nextL = paramLocal->L;
                nextC = paramLocal->C - 1;
            }
            break;
        case DROITE:
            if ((paramLocal->C + 1) >= NB_COLONNE && tab[paramLocal->L][0] < 1)
            //cas spécifique où la case suivante est à l'autre bout du tab
            //si la case suivante n'est pas un MUR ou un autre FANTOME
            {
                nextL = paramLocal->L;
                nextC = 0;
            }
            else
            {
                nextL = paramLocal->L;
                nextC = paramLocal->C + 1;
            }
            break;
        default:
            printf("[threadFantomes: %d][Erreur]Switch(dirFantome): %d\n", pthread_self(), dirFantome);
            exit(1);
            break;
        }

        if (mode == 1 && tab[nextL][nextC] == PACMAN)
            tuerPacman++;
        else if (mode == 2 && tab[nextL][nextC] == PACMAN)
            changeDir++;
        else if (tab[nextL][nextC] < 1)
            continuer++;
        else
            changeDir++;

        if (tuerPacman)
        {
            paramLocal->L = nextL;
            paramLocal->C = nextC;

            MonEffacerCarre(nextL, nextC);

            pthread_cancel(tidPacMan);
        }

        if (continuer)
        {
            paramLocal->L = nextL;
            paramLocal->C = nextC;
        }

        if (changeDir)
        {
            //Recupere le nombre de cases vides
            if (tab[paramLocal->L - 1][paramLocal->C] < 1)
            {
                for (int i = 0; i < 4; i++)
                    if (PossibiliteDeNouvelleDir[i] == 0)
                    {
                        PossibiliteDeNouvelleDir[i] = HAUT;
                        i = 4;
                    }
            }
            if (tab[paramLocal->L + 1][paramLocal->C] < 1)
            {
                for (int i = 0; i < 4; i++)
                    if (PossibiliteDeNouvelleDir[i] == 0)
                    {
                        PossibiliteDeNouvelleDir[i] = BAS;
                        i = 4;
                    }
            }
            if (tab[paramLocal->L][paramLocal->C - 1] < 1)
            {
                for (int i = 0; i < 4; i++)
                    if (PossibiliteDeNouvelleDir[i] == 0)
                    {
                        PossibiliteDeNouvelleDir[i] = GAUCHE;
                        i = 4;
                    }
            }
            if (tab[paramLocal->L][paramLocal->C + 1] < 1)
            {
                for (int i = 0; i < 4; i++)
                    if (PossibiliteDeNouvelleDir[i] == 0)
                    {
                        PossibiliteDeNouvelleDir[i] = DROITE;
                        i = 4;
                    }
            }

            for (int i = 0; i < 4; i++)
            {
                if (PossibiliteDeNouvelleDir[i] != 0)
                {
                    max++;
                }
            }
            if (PossibiliteDeNouvelleDir[0] != 0)
            {
                max--;
            }

            // printf("[threadFantomes: %d]Min = %d Max = %d\n",pthread_self(),min,max);

            if (max == 0 && PossibiliteDeNouvelleDir[0] == 0)
            {
                // printf("[threadFantomes: %d]Bloquer\n",pthread_self());
                bloquer++;
            }

            while (!newDirOk && !bloquer)
            {
                // printf("[threadFantomes: %d]newDirOk\n",pthread_self());
                newDir = min + rand() % (max - min + 1);
                newDir = PossibiliteDeNouvelleDir[newDir];
                for (int i = 0; i < 4; i++)
                    if (PossibiliteDeNouvelleDir[i] == newDir)
                        newDirOk++;
            }

            if (!bloquer)
            {
                if (newDir == HAUT)
                    paramLocal->L--;
                else if (newDir == BAS)
                    paramLocal->L++;
                else if (newDir == GAUCHE)
                    paramLocal->C--;
                else if (newDir == DROITE)
                    paramLocal->C++;

                dirFantome = newDir;
            }
        }

        //Choper le cache
        paramLocal->cache = tab[paramLocal->L][paramLocal->C];

        //Afficher le fantome
        if (mode == 1)
        {
            //Dessin normal
            DessineFantome(paramLocal->L, paramLocal->C, paramLocal->couleur, dirFantome);
        }
        else if (mode == 2)
        {
            //Dessin bleu
            DessineFantomeComestible(paramLocal->L, paramLocal->C);
        }

        tab[paramLocal->L][paramLocal->C] = pthread_self();

        if (pthread_mutex_unlock(&mutexTab))
        {
            perror("[threadFantomes][Erreur]pthread_mutex_unlock on mutexTab\n");
            exit(1);
        }
        if (pthread_mutex_unlock(&mutexMode))
        {
            perror("[threadFantomes][Erreur]pthread_mutex_unlock on mutexMode\n");
            exit(1);
        }
    }
    pthread_cleanup_pop(0);
    //pthread_kill(tidCompteurFantomes, SIGQUIT);
    pthread_exit(NULL);
}

void *threadVies(void *param)
{
    printf("[threadVies: %d][Debut]\n", pthread_self());

    MasquerTousLesSignaux();

    DessineChiffre(18, 22, nbVies);

    while (nbVies > 0)
    {
        //Thread PacMan
        pthread_create(&tidPacMan, NULL, threadPacMan, NULL);
        //printf("[threadVies]pthread_create\n");
        pthread_join(tidPacMan, NULL);
        nbVies--;
        DessineChiffre(18, 22, nbVies);
    }

    //bloquer le tab pour figer les fantomes
    if (pthread_mutex_lock(&mutexTab))
    {
        perror("[threadVies][Erreur]pthread_mutex_lock on mutexTab\n");
        exit(1);
    }

    DessineGameOver(9, 4);

    pthread_exit(NULL);
}

void *threadTimeOut(void *param)
{
    printf("[threadTimeOut: %d][Debut]\n", pthread_self());

    sigset_t maskTimeout;
    sigfillset(&maskTimeout);
    sigdelset(&maskTimeout, SIGALRM);
    sigprocmask(SIG_SETMASK, &maskTimeout, NULL);

    int min = 8;
    int max = 15;
    int time = 0;

    time = min + rand() % (max - min + 1);

    if (param)
    {
        time += *(int *)param;
    }

    alarm(time);
    pause();
    pthread_exit(NULL);
}

void MonEffacerFantome(S_FANTOME *variableSpecifique)
{
    if (variableSpecifique->cache == VIDE)
    {
        MonEffacerCarre(variableSpecifique->L, variableSpecifique->C);
    }
    else if (variableSpecifique->cache == PACGOM)
    {
        MonDessinePacGom(variableSpecifique->L, variableSpecifique->C);
    }
    else if (variableSpecifique->cache == SUPERPACGOM)
    {
        MonDessineSuperPacGom(variableSpecifique->L, variableSpecifique->C);
    }
    else if (variableSpecifique->cache == BONUS)
    {
        MonDessineBonus(variableSpecifique->L, variableSpecifique->C);
    }
}

void MonDessineFantome(int dir, S_FANTOME *variableSpecifique)
{
    tab[variableSpecifique->L][variableSpecifique->C] = pthread_self();
    DessineFantome(variableSpecifique->L, variableSpecifique->C, variableSpecifique->couleur, dir);
}

void MonDessineBlueFantome(int dir, S_FANTOME *variableSpecifique)
{
    tab[variableSpecifique->L][variableSpecifique->C] = pthread_self();
    DessineFantomeComestible(variableSpecifique->L, variableSpecifique->C);
}

void MonDessineBonus(int l, int c)
{
    tab[l][c] = BONUS;
    DessineBonus(l, c);
}

void MonDessinePacMan(int l, int c, int dir)
{
    DessinePacMan(l, c, dir);
    tab[l][c] = PACMAN;
    L = l;
    C = c;
}

void MonDessinePacGom(int l, int c)
{
    DessinePacGom(l, c);
    tab[l][c] = PACGOM;
}

void MonDessineSuperPacGom(int l, int c)
{
    DessineSuperPacGom(l, c);
    tab[l][c] = SUPERPACGOM;
}

void MonEffacerCarre(int l, int c)
{
    EffaceCarre(l, c);
    tab[l][c] = VIDE;
}

void HandlerSIGCHLD(int)
{
    printf("[HandlerSIGCHLD] %d.%d\n", getpid(), pthread_self());
    pthread_exit(NULL);
}

void HandlerInt(int)
{
    printf("[HandlerInt] %d.%d\n", getpid(), pthread_self());

    if (pthread_mutex_lock(&mutexTab))
    {
        perror("[HandlerInt][Erreur]pthread_mutex_lock on mutexTab\n");
        exit(1);
    }
    if ((C - 1) < 0)
    {
        DIR = GAUCHE;
    }
    else if (tab[L][C - 1] != MUR)
    {
        DIR = GAUCHE;
    }
    if (pthread_mutex_unlock(&mutexTab))
    {
        perror("[HandlerInt][Erreur]pthread_mutex_unlock on mutexTab\n");
        exit(1);
    }
}
void HandlerHup(int)
{
    printf("[HandlerHup] %d.%d\n", getpid(), pthread_self());
    if (pthread_mutex_lock(&mutexTab))
    {
        perror("[HandlerHup][Erreur]pthread_mutex_lock on mutexTab\n");
        exit(1);
    }
    if ((C + 1) >= NB_COLONNE)
    {
        DIR = DROITE;
    }
    if (tab[L][C + 1] != MUR)
    {
        DIR = DROITE;
    }
    if (pthread_mutex_unlock(&mutexTab))
    {
        perror("[HandlerHup][Erreur]pthread_mutex_unlock on mutexTab\n");
        exit(1);
    }
}
void HandlerUsr1(int)
{
    printf("[HandlerUsr1] %d.%d\n", getpid(), pthread_self());
    if (pthread_mutex_lock(&mutexTab))
    {
        perror("[HandlerUsr1][Erreur]pthread_mutex_lock on mutexTab\n");
        exit(1);
    }
    if (tab[L - 1][C] != MUR)
    {
        DIR = HAUT;
    }
    if (pthread_mutex_unlock(&mutexTab))
    {
        perror("[HandlerUsr1][Erreur]pthread_mutex_unlock on mutexTab\n");
        exit(1);
    }
}
void HandlerUsr2(int)
{
    printf("[HandlerUsr2] %d.%d\n", getpid(), pthread_self());
    if (pthread_mutex_lock(&mutexTab))
    {
        perror("[HandlerUsr2][Erreur]pthread_mutex_lock on mutexTab\n");
        exit(1);
    }
    if (tab[L + 1][C] != MUR)
    {
        DIR = BAS;
    }
    if (pthread_mutex_unlock(&mutexTab))
    {
        perror("[HandlerUsr2][Erreur]pthread_mutex_unlock on mutexTab\n");
        exit(1);
    }
}

void HandlerAlarm(int)
{
    printf("[HandlerAlarm] %d.%d\n", getpid(), pthread_self());

    if (pthread_mutex_lock(&mutexMode))
    {
        printf("[threadTimeOut][Erreur]pthread_mutex_lock on mutexMode\n");
        exit(1);
    }
    mode = 1;
    if (pthread_mutex_unlock(&mutexMode))
    {
        printf("[threadTimeOut][Erreur]pthread_mutex_unlock on mutexMode\n");
        exit(1);
    }
    pthread_cond_signal(&condMode);

    pthread_exit(0);
}

void ArmerTousLesSignaux(void)
{
    printf("[ArmerTousLesSignaux] %d.%d\n", getpid(), pthread_self());
    //sigint
    struct sigaction A;
    A.sa_handler = HandlerInt;
    A.sa_flags = 0;
    sigemptyset(&A.sa_mask);
    if (sigaction(SIGINT, &A, NULL) == -1)
    {
        perror("[ArmerTousLesSignaux][sigaction(SIGINT, &A, NULL)]");
        exit(1);
    }
    //sighup
    struct sigaction B;
    B.sa_handler = HandlerHup;
    B.sa_flags = 0;
    sigemptyset(&B.sa_mask);
    if (sigaction(SIGHUP, &B, NULL) == -1)
    {
        perror("[ArmerTousLesSignaux][sigaction(SIGHUP, &B, NULL)]");
        exit(1);
    }
    //sigusr1
    struct sigaction C;
    C.sa_handler = HandlerUsr1;
    C.sa_flags = 0;
    sigemptyset(&C.sa_mask);
    if (sigaction(SIGUSR1, &C, NULL) == -1)
    {
        perror("[ArmerTousLesSignaux][sigaction(SIGUSR1, &C, NULL)]");
        exit(1);
    }
    //sigusr2
    struct sigaction D;
    D.sa_handler = HandlerUsr2;
    D.sa_flags = 0;
    sigemptyset(&D.sa_mask);
    if (sigaction(SIGUSR2, &D, NULL) == -1)
    {
        perror("[ArmerTousLesSignaux][sigaction(SIGUSR2, &D, NULL)]");
        exit(1);
    }
    //sigchld
    struct sigaction E;
    E.sa_handler = HandlerSIGCHLD;
    E.sa_flags = 0;
    sigemptyset(&E.sa_mask);
    if (sigaction(SIGCHLD, &E, NULL) == -1)
    {
        perror("[ArmerTousLesSignaux][sigaction(SIGCHLD, &E, NULL)]");
        exit(1);
    }
    //sigalrm
    struct sigaction F;
    F.sa_handler = HandlerAlarm;
    F.sa_flags = 0;
    sigemptyset(&F.sa_mask);
    if (sigaction(SIGALRM, &F, NULL) == -1)
    {
        perror("[ArmerTousLesSignaux][sigaction(SIGALRM, &F, NULL)]");
        exit(1);
    }
}

void DeMasquerSIGCHLD(void)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigdelset(&mask, SIGCHLD);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
    {
        perror("[DeMasquerSIGCHLD][sigprocmask(SIG_SETMASK, &mask, NULL)]");
        exit(1);
    }
}

void MasquerTousLesSignaux(void)
{
    //	printf("[MasquerTousLesSignaux] %d.%d\n", getpid(), pthread_self());
    //version 1
    // sigset_t mask;
    // sigemptyset(&mask);
    // sigaddset(&mask, SIGUSR1);
    // sigaddset(&mask, SIGUSR2);
    // sigaddset(&mask, SIGHUP);
    // sigaddset(&mask, SIGINT);
    // if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
    // {
    //     perror("[MasquerTousLesSignaux][sigprocmask(SIG_SETMASK, &mask, NULL)]");
    //     exit(1);
    // }

    //version 2
    sigset_t mask;
    sigemptyset(&mask);
    sigfillset(&mask);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
    {
        perror("[MasquerTousLesSignaux][sigprocmask(SIG_SETMASK, &mask, NULL)]");
        exit(1);
    }
}

void DeMasquerTousLesSignaux(void)
{
    //printf("[DeMasquerTousLesSignaux] %d.%d\n", getpid(), pthread_self());
    //version 1
    // sigset_t mask;
    // sigemptyset(&mask);
    // sigaddset(&mask, SIGUSR1);
    // sigaddset(&mask, SIGUSR2);
    // sigaddset(&mask, SIGHUP);
    // sigaddset(&mask, SIGINT);
    // if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    // {
    //     perror("[DeMasquerTousLesSignaux][sigprocmask(SIG_UNBLOCK, &mask, NULL)]");
    //     exit(1);
    // }

    //version 2
    sigset_t mask;
    sigemptyset(&mask);
    sigdelset(&mask, SIGINT);
    sigdelset(&mask, SIGHUP);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGUSR2);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
    {
        perror("[DeMasquerTousLesSignaux][sigprocmask(SIG_SETMASK, &mask, NULL)]");
        exit(1);
    }
}

void initCle()
{
    printf("[initCle] initialisation d'une cle\n");
    if (pthread_key_create(&cle, destructeur))
    {
        perror("[initCle] error: pthread_key_create");
        exit(1);
    }
}

void destructeur(void *param)
{
    printf("[destructeur]\n");
    free(param);
    //delete param;
}