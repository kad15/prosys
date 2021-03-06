/* BELDJILALI IENAC15_SITA  TD6 */
/* Conversion multi htread avec supervision */
/* gcc -Wall -W -lpthread -o minmaj6 minmaj6.c */

#define _REENTRANT
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
//#include <semaphore.h>

#define FALSE 0
#define TRUE 1
#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define N 8
#define TAILLE_BUF 512

int _fd2;
unsigned long _idThread=0;

typedef struct elt elt;
struct elt {
    int taille;
    char donnee[TAILLE_BUF];
};
int iEcr=0,iLect=0;
static elt _file[N];
int _nbOcc=0; // nombre de cases pleines ; si _nbOcc == N on doit mettre en attente l'écriture dans le _file




/* définition des conditions et du mutex */
pthread_cond_t _onPeutDeposer = PTHREAD_COND_INITIALIZER;
pthread_cond_t _onPeutPrelever = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _m_read_fic = PTHREAD_MUTEX_INITIALIZER;


/* Pour la suspension reprise */
int _suspendre = FALSE;
pthread_cond_t _c_suspendre = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _m_suspendre = PTHREAD_MUTEX_INITIALIZER;

/* Pour la terminaison */
int _terminer = FALSE;


void ecrire(elt e)  // on écrit dans le _file = file
{

    pthread_mutex_lock(&_mutex);
    while (_nbOcc == N) { // nombre de case occupées == N=8 => plus de case vide ; on stoppe l'écriture dans la file _file et  le mutex est libéré pour éviter l'interblicage
        pthread_cond_wait(&_onPeutDeposer, &_mutex);

    }
    // on laisse la main aux autres thread tant qu'on ne reçoit pas de signal "o_nbOcceutDeposer"
    _file[iEcr]=e;// on a "franchit le wait" donc le _file n'est pas saturé et on peut écrire et incrémenter

    // fprintf(stderr, "%s\n",e.donnee);

    iEcr = (iEcr + 1) % N;
    _nbOcc++;
    pthread_cond_signal(&_onPeutPrelever); // on a écrit une donnée donc on peut envoyer le signal o_nbOcceutPrelever qui réveille le thread prélever
    pthread_mutex_unlock(&_mutex);


}

elt lire(void)  // on lit le _file
{
    elt e;
    // synchro écriture/lecture dans la file
    pthread_mutex_lock(&_mutex);
    while(_nbOcc == 0) { // ya plus rien à lire => on met en attente la lecture et le mutex est libéré pour éviter l'interblicage
        pthread_cond_wait(&_onPeutPrelever, &_mutex);
    }
    e=_file[iLect];
    //_file[iLect].taille=0;
    //strcpy(_file[iLect].donnee,"NULL");
    iLect = (iLect + 1) % N;
    //fprintf(stderr, "%d\n",_nbOcc);
    _nbOcc--;  // il y a au moins une place libre. On envoie donc un signal pour autoriser à nouveau l'écriture dans la file
    pthread_cond_signal(&_onPeutDeposer);
    pthread_mutex_unlock(&_mutex);
    return e;
}



void toMajuscule(elt * e)  /* conversion en majuscule de la stucture e  dans la _file */
{
    int i, ecart = 'a' - 'A';
    for (i=0; i<e->taille; i++) { // conversion sur place en majuscule
        if (e->donnee[i] >='a' && e->donnee[i] <='z') {
            e->donnee[i] -=ecart;
        }
    }
}

/* producteur : ecrit dans la file _file circulaire de 0 à N-1 avant de recommencerf à partir de 0*/
/* le producteur ne doit pas écrire si la file _file est plein */
/* le producteur et le consommateur ne doivent pas travailler dans une même case*/
/* le producteur écrit dans des case vides ; ce qui se traduit par P(semCaseVide) et fournit des cases pleines => V(semCasePleines)*/
void *producteur(void* arg)  /* lit dans le fichier et ecrit dans la file _file circulaire*/
{

    elt e;
    int * ptr_fd = (int*)arg;
    do {
        pthread_mutex_lock(&_m_read_fic);
        e.taille=read(*ptr_fd, e.donnee, sizeof(e.donnee));


        if (e.taille==-1) {
            perror("erreur lecture fichier");
            pthread_exit(NULL);
        }
        ecrire(e);
        printf("Le thread producteur numéro %lu écrit dans le tampon i.e. la file.\n",pthread_self() );
        pthread_mutex_unlock(&_m_read_fic);
        sleep(1);

        //début terminaison
        if(_terminer) {
            printf("_terminer == TRUE ; on termine le thread producteur numéro %lu. \n",pthread_self());
            pthread_exit(NULL);
        }
        // fin terminaison


    } while(1);
}


/* consommateur lit dans la file circulaire de 0 à N-1 avant de recommencerf à partir de 0*/
/* le consommateur ne doit pas faire de lecture si la file _file est vide */
/* le consommateur lit des cases pleines et fournit des cases vides*/
void *consommateur(void* arg)  /* convertit en capital et écrit dans le fichier toto6 */
{
    arg=arg;
    elt e;
    while(1) {
        e = lire();
        toMajuscule(&e);
        write(_fd2,e.donnee, e.taille);
        printf("Le thread consommateur numéro %lu écrit dans le fichier toto6.\n",pthread_self() );
        sleep(1);

        //début suspension
        pthread_mutex_lock(&_m_suspendre);
        while(_suspendre) {
            printf("_suspendre == TRUE ; on suspend le thread consommateur numéro %lu.  libération du mutex\n",pthread_self());
            printf("Attention : les threads producteurs ne sont pas suspendus et continuent à écrire jusqu'à ce que le tampon soit saturé !");
            pthread_cond_wait(&_c_suspendre,&_m_suspendre);
        }
        pthread_mutex_unlock(&_m_suspendre);
        // fin suspension


        //début terminaison
        /* _nbOcc == 0 pour que le consommateur continue de lire le tampon et d'alimenter le fichier toto6 jusqu'à ce que le tampon soit vide*/
        if(_terminer && _nbOcc==0)pthread_exit(NULL);
        // fin terminaison
    }
}

void *superviseur(void* arg)  /* supervision des threads*/
{
    arg=arg;

    // variable choix :
    int choix=0;

    // boucle de traitement :
    do {
        printf(" \t********* Menu **********\n");
        printf("\tTaper votre choix puis sur ENTER \n");
        printf(" \t1 : suspension.\n");
        printf(" \t2 : reprise.\n");
        printf(" \t3 : terminaison.\n");
        printf("\n");
        scanf("%d",&choix);
        getchar();
        printf("\nTaper votre choix suivi de ENTER : ");
        switch(choix) {
        case 1 : // on suspend le thread consommateur qui convertit et écrit la chaine lue dans la file dans le fichier toto6.
            //nb : l'écriture dans le tampon ne s'arrête pas avant que ce dernier ne soit plein car on ne le suspend pas.
            pthread_mutex_lock(&_m_suspendre);
            _suspendre=TRUE;
            pthread_cond_signal(&_c_suspendre); // signal au consommateur
            pthread_mutex_unlock(&_m_suspendre);
            break;
        case 2 : // on reprend le traitement
            pthread_mutex_lock(&_m_suspendre);
            _suspendre=FALSE;
            pthread_cond_signal(&_c_suspendre); // signal au consommateur
            pthread_mutex_unlock(&_m_suspendre);

            break;
        case 3 : // on termine les threads producteurs qui lisent le fichier et écrit dans la file
            // on termine le thread consommateur
            _terminer=TRUE;
            printf("Le superviseur envoie un signal de terminaison et se termine");
            break;
        default :
            printf("\n choix invalide, recommencez....\n");
        }
    } while (choix != 3);



    pthread_exit(NULL);
}

int main()
{

    int fd;
    pthread_t idthreadProducteur1, idthreadProducteur2, idThreadConsommateur, idThreadSuperviseur; /* identifiant des threads */

    // Si un nom de fichier est fourni en paramètre, on ouvre ce fichier
    // et on lui associe le descripteur sortie standard
    if ((fd=open("fic.large", O_RDONLY))==-1) {
        perror("Oooops ");
        exit(1);
    }

    if ((_fd2=open("toto6", O_WRONLY|O_CREAT|O_TRUNC,0644))==-1) {
        perror("main toto6 : ");
        exit(1);
    }

    if (pthread_create(&idThreadSuperviseur, NULL, superviseur, NULL)!=0) {
        printf("erreur creation threadEcriture1\n");
        exit(1);
    }

    if (pthread_create(&idthreadProducteur1, NULL, producteur, &fd)!=0) {
        printf("erreur creation threadEcriture1\n");
        exit(1);
    }


    if (pthread_create(&idthreadProducteur2, NULL, producteur, &fd)!=0) {
        printf("erreur creation threadEcriture2\n");
        exit(1);
    }

    /* creation thread conversion*/
    if (pthread_create(&idThreadConsommateur, NULL, consommateur, NULL)!=0) {
        printf("erreur creation thread Conversion\n");
        exit(1);
    }



    /* attente de la terminaison des threads */
    pthread_join(idthreadProducteur1, NULL);
    pthread_join(idthreadProducteur2, NULL);
    pthread_join(idThreadConsommateur, NULL);
    pthread_join(idThreadSuperviseur, NULL);


    /* suppression drd vzr conditions */
    // mutex pour accès concurrent au fichier fic.large
    pthread_mutex_destroy(&_m_read_fic);

    // mutex et condition d'accès concurrent au tampon
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_onPeutDeposer);
    pthread_cond_destroy(&_onPeutPrelever);

    // mutex condition de contrôle suspension reprise
    pthread_mutex_destroy(&_m_suspendre);
    pthread_cond_destroy(&_c_suspendre);


    close(fd);
    close(_fd2);
    exit(0);
}
