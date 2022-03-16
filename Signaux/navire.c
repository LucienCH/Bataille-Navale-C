/* 
 * Programme pour processus navire :
*/

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mer.h>
#include <bateaux.h>

/* 
 * VARIABLES GLOBALES (utilisees dans les handlers)
*/

int Energie ; 

/*
 * Handlers 
*/



// -- Handler pour SIGUSR2 fait perdre de l'énergie au bateau
void hand_usr2(int numSig)
{
  // printf("Je pers de l'énergie... \n");
  
  Energie -= BATEAU_MAX_ENERGIE / 4;
  // printf("Niveau énergie = %d ", Energie);
}


// -- Handler pour SIGQUIT affiche un message et stop le processus bateau 
void hand_quit(int numSig)
{
  printf("Fin du bateau %d ... \n", getpid());
  exit(0);
}

/*
 * Programme Principal 
*/

int
main( int nb_arg , char * tab_arg[] )
{
  char nomprog[128] ;
  pid_t pid_amiral ;
  pid_t pid_bateau = getpid()  ;

  /*----------*/

  /* 
   * Capture des parametres 
  */

  if( nb_arg != 2 )
    {
      fprintf( stderr , "Usage : %s <pid amiral>\n", 
	       tab_arg[0] );
      exit(-1);
    }

  /* Nom du programme */
  strcpy( nomprog , tab_arg[0] );
  sscanf( tab_arg[1] , "%d" , &pid_amiral ) ; 

  /* Initialisation de la generation des nombres pseudo-aleatoires */
  srandom((unsigned int)pid_bateau);


  /* Affectation du niveau d'energie */
  Energie = random()%BATEAU_MAX_ENERGIE ;
  
  printf( "\n\n--- Debut bateau [%d]---\n\n" , pid_bateau );

  /*
   * Deroulement du jeu 
  */

  printf("Tentative d'envoie de signal \n");

  struct sigaction act_bat;

  act_bat.sa_handler = hand_usr2;
  sigemptyset(&act_bat.sa_mask);
  sigaddset(&act_bat.sa_mask, SIGUSR2);

  sigaction(SIGUSR2, &act_bat,NULL);

  struct sigaction act_quit;
  
  sigemptyset(&act_quit.sa_mask);
  sigaddset(&act_quit.sa_mask, SIGQUIT);
  act_quit.sa_handler = hand_quit;

  sigaction(SIGQUIT, &act_quit, NULL);

  while(1)
  {
    kill(pid_amiral, SIGUSR1);
    sleep(2);
  }

  printf( "\n\n--- Arret bateau (%d) ---\n\n" , pid_bateau );

  exit(0);
}

