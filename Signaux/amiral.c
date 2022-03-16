/* 
 * Programme pour processus navire-amiral :
 */

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


#include <mer.h>
#include <bateaux.h>


// -- nombre de bateau dans la flotte maximum --
#define NB_BATEAUX 100


/* 
 * VARIABLES GLOBALES (utilisees dans les handlers)
*/

char Nom_Prog[256] ;
int fd_mer;
int nbBateaux;
int nbBateauxMax;
bateau_t * flotte[NB_BATEAUX];
char marque= 'A';
int debut = 1;

/*
 * FONCTIONS LOCALES 
*/


// -- rechercher une place libre dans la flotte --
int placeFlotte()
{
  for(int i = 0; i < NB_BATEAUX; i++)
  {
    if (flotte[i] == NULL)
    {
      return i;
    }
  }
  return -1;
}


// -- on parcours tous les bateaux de la flotte --
int  bateauPresent(int pid_bateau)
{
  
  for(int i = 0 ; i < NB_BATEAUX; i++)
  {
    if (flotte[i] != NULL && flotte[i]->pid == pid_bateau)
    {
      return i;
    }
  }
  return -1;

}

// -- pose un bateau dans la mer avec toutes les vérifications nécéssaires --
booleen_t initBateau(int fd_mer, int pid_bateau, bateau_t * bateau)
{
  int pos;
  
  if((pos = placeFlotte()) == -1 ){
    perror("Erreure dans initBateau... Plus de place dans flotte \n");
    return FAUX;
  }

  // -- on vérifie si il y a de la place dans le fichier mer --
  if(nbBateaux < nbBateauxMax)
  {
    if( ( bateau = bateau_new( NULL, marque, pid_bateau ) ) == BATEAU_NULL )
    {
      perror("Erreur dans la création du bateau ... \n");
      return FAUX;
    }

    // printf("Affichage de la structure du bateau : \n");
    // bateau_printf( bateau );

    if((mer_bateau_initialiser( fd_mer , bateau)) == -1 )
    {
      bateau_destroy(&bateau);
      kill(pid_bateau, SIGKILL);
      return FAUX;
    }
    else
    {
      flotte[pos] = bateau;
      marque += 1;
      mer_nb_bateaux_ecrire(fd_mer, ++nbBateaux);
      return VRAI;
    }
    // printf("nb Bateau %d \n", nbBateaux);
  }
  return FAUX;

// fin initBateau
}


// -- actions qu'un bateau doit effectuer à la récéption d'un signal --
int tourBateau(int fd_mer, bateau_t * bateau, int pid_bateau)
{ 
  
  // -- Booléen --
  booleen_t estTouche;
  booleen_t reussi;
  
  // -- Coordonnées -- 
  coord_t cible;
  coords_t * liste_voisins = NULL;

  mer_bateau_est_touche(fd_mer, bateau, &estTouche);

  if(estTouche == VRAI)
  {
    printf("Le bateau %c coule ... \n", bateau->marque);
    mer_bateau_couler(fd_mer, bateau);
    bateau_destroy(&bateau);
    mer_nb_bateaux_ecrire(fd_mer, --nbBateaux);
    bateau = NULL;
    kill(pid_bateau, SIGQUIT);
  }
  else
  {
    if((mer_voisins_rechercher(fd_mer, bateau, &liste_voisins)) != CORRECT)
    {
      perror("Erreur dans la recherche des voisins... \n");
      return -1;
    }

    // -- envoie signal perte d'énergie --
    kill(pid_bateau, SIGUSR2);

    // -- recherche d'une cible avant le tire --
    mer_bateau_cible_acquerir(fd_mer, bateau, &reussi, &cible);  
    
    if(reussi == VRAI)
    {
      printf("Je tire sur la cible ");
      mer_bateau_cible_tirer(fd_mer, cible);
    }
    return 1;
  }
  return 0;
}


/*
* HANDLER
*/

// -- handler du signal SIGUSR1 envoyé par un processus bateau
void hand_usr1(int numSig, siginfo_t * siginfo_bat, void * context)
{
  int pid_sig_bateau = siginfo_bat->si_pid;
  int position;  
  
  bateau_t * bateau = NULL;


  
  // printf("J'ai recu un signal %d du pid : %d \n ",numSig, pid_sig_bateau);
  
  mer_afficher(fd_mer);

  // -- on vérifie la présence du bateau dans la flotte --
  if ( (position = bateauPresent(pid_sig_bateau)) != -1 )
  {
    bateau = flotte[position];
  }
  
  // -- Si le Bateau n'existe pas on le crée --
  if(bateau == NULL && pid_sig_bateau != 0)
  {
    // printf("Pas de bateau dans la mer \n");
    printf("création du bateau ... \n");
    
    if(initBateau(fd_mer, pid_sig_bateau, bateau) == 0)
    {
      perror("Erreur dans initBateau() ... \n");
      exit(-1);
    }
  }
  
  // -- Si le bateau existe --
  else if(bateau != NULL && pid_sig_bateau != 0)
  {
    // sleep(2)
    // printf("Tour d'un bateau présent dans la flotte ... \n");
    
    
    // -- Test de victoire --
    if(nbBateaux == 1 && debut != 1)
    { 
      printf("Le bateau %c à gagné ! \n", bateau->marque);
      kill(pid_sig_bateau, SIGQUIT);
      exit(0);
    }
    
    if(debut && nbBateaux)
    {
      debut = 0;
    }
    else
    {
      usleep(800);
      if(tourBateau(fd_mer, bateau, pid_sig_bateau) < 0)
      {
        perror("Erreur dans le tour du bateau \n");
        exit(0);
      }
    }
  
  // -- fin ElseIF -- 
  }

// -- Fin du handler --
}

/*
 * Programme Principal
 */
int
main( int nb_arg , char * tab_arg[] )
{
  char fich_mer[128] ;
  int nbCol = 0;
  int nbLig = 0;


  /*----------*/

  /* 
  * Capture des parametres 
  */

  if( nb_arg != 2 ) 
  {
    fprintf( stderr , "Usage : %s <fichier mer> \n", 
    tab_arg[0] );
    exit(-1);
  }

  strcpy( Nom_Prog , tab_arg[0] );
  strcpy( fich_mer , tab_arg[1] );
  
  /*
  * Affichage pid bateau amiral 
  */
  
  printf(" PID bateau amiral = %d\n" , getpid() ) ;

  /* Initialisation de la generation des nombres pseudo-aleatoires */
  srandom((unsigned int)getpid());

  struct sigaction act_bat;

  act_bat.sa_flags = SA_SIGINFO;
  sigemptyset(&act_bat.sa_mask);
  
  // On masque SIGUSR1 pour ne pas recevoir de signaux concurrents
  sigaddset(&act_bat.sa_mask, SIGUSR1); 
  act_bat.sa_sigaction = hand_usr1;

  struct sigaction act_energie;

  act_energie.sa_flags = SA_SIGINFO;
  sigemptyset(&act_energie.sa_mask);
  
  // On masque SIGUSR2 pour ne pas recevoir de signaux concurrents
  sigaddset(&act_energie.sa_mask, SIGUSR2);



  sigaction(SIGUSR1, &act_bat, NULL);
  sigaction(SIGUSR2, &act_energie, NULL);


  // -- Ouverture du fichier mer --
  if((fd_mer = open(fich_mer, O_RDWR, 0666)) == -1)
  {
    perror("Erreur dans la lecture du fichier mer amiral.c \n");
    exit(-1);
  }

  // reset flotte
  for(int i = 0; i < NB_BATEAUX; i++){
    flotte[i] = NULL;
  }

  mer_dim_lire(fd_mer, &nbLig, &nbCol);
  nbBateauxMax = (nbCol * nbLig) / 4;

  printf("Nombre de Bateaux maximum : %d \n", nbBateauxMax);


  while ((1));
  

  printf("\n\n\t----- Fin du jeu -----\n\n");
  
  exit(0);
}

