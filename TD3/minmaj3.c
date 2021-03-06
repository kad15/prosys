/* Affichage a l'ecran du contenu d'un fichier dont le nom est passe en parametre */
#define _REENTRANT
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>



/* Execution concurrente de 2 threads */
/* gcc -Wall -lpthread -o thread2 ex_thread2.c */

pthread_mutex_t _m = PTHREAD_MUTEX_INITIALIZER;
void *conversion(void* fd)
{
    fd=fd;

	int nb, i, ecart = 'a' - 'A';
	char buf[512];
        //
	do {
                pthread_mutex_lock(&_m);
                nb=read(0, buf, sizeof(buf));
		for (i=0;i<nb;i++)
		{
			if (buf[i] >='a' && buf[i]<='z')
			{
				buf[i]-=ecart;
			}
		}
		write(1,buf,nb);
                pthread_mutex_unlock(&_m);

  	} while (nb>0);
        //
 	return (0);
}

int main(int argc, char* argv[]){
 	int fd;
	pthread_t idThread1, idThread2; /* identifiant des threads */



// Si un nom de fichier est fourni en paramètre, on ouvre ce fichier
// et on lui associe le descripteur sortie standard

	if (argc==2)
	{
		if ((fd=open(argv[1], O_RDONLY))==-1)
		{
    			perror("Ouverture");
    			exit(1);
  		}
		close(0);
		dup(fd);
		close(fd); // le descripteur num 1 pointe sur le fichier donné en argument

	}


   /* creation thread 1*/
  if (pthread_create(&idThread1, NULL, conversion, NULL)!=0){
    printf("erreur creation thread1\n");
    exit(1);
  }

  /* creation thread 2*/
  if (pthread_create(&idThread2, NULL, conversion, NULL)!=0){
    printf("erreur creation thread2\n");
    exit(1);
  }



    /* attente de la terminaison des threads */
  pthread_join(idThread1, NULL);
  pthread_join(idThread2, NULL);
  return(0);
}





