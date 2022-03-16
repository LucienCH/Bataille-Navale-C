#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> /* fcntl */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h> /* errno */
#include <string.h>

#include <bateau.h>
#include <mer.h>

/* 
 *  Programme principal 
 */

// Verrouillage/Déverrouillage du fichier mer complet (1 verrouiller / 0 déverrouiller)
  
int verouillerMer(int fd_mer,int etat){
  struct flock verr_mer;

  verr_mer.l_whence = SEEK_SET;
  verr_mer.l_start = 0;
  verr_mer.l_len = 0;

  if(etat == 1){
    verr_mer.l_type = F_WRLCK;
  }else{
    verr_mer.l_type = F_UNLCK;
  }

  if((fcntl(fd_mer, F_SETLKW, &verr_mer)) == -1){
    printf("Erreur lors de la pose/ retrait du verrou dans verouillerMer \n");
    return -1;
  }
  return 1;
}


int verrouillerBouclier(int fd_mer, int etat, bateau_t * bateau){

  struct flock verr_bouc;
  
  verr_bouc.l_whence = 0;
  verr_bouc.l_len = MER_TAILLE_CASE;

  coords_t * bateauCellTab = bateau_corps_get(bateau);
	coord_t * bateauCell = bateauCellTab->coords;

  printf("NB de coordonées : %d \n\n ", bateauCellTab->nb);

  if(etat == 1){
    verr_bouc.l_type = F_WRLCK;
    
  }else{
    verr_bouc.l_type = F_UNLCK;
  }
  
  for(int i = 0; i < bateauCellTab->nb; i++){
    verr_bouc.l_start = bateauCell[i].pos;
    if(fcntl(fd_mer, F_SETLK, &verr_bouc) == -1 ){
      printf("Erreur lors de la pose/ retrait du verrou dans verouillerBouclier \n");
      return -1;
    }
  }
  return 1;
}

int verrouillerVoisins(int fd_mer, int etat, coords_t * liste_voisins){
  
  coord_t coordCur;
  case_t caseMer;
  
  struct flock verr_vois;

  verr_vois.l_whence = 0;
  verr_vois.l_len = MER_TAILLE_CASE;

  if(etat == 1){
    verr_vois.l_type = F_WRLCK;
    
  }else{
    verr_vois.l_type = F_UNLCK;
  }
  
  for(int i = 0; i < coords_nb_get(liste_voisins); i++ ){
    
    mer_case_lire(fd_mer, liste_voisins->coords[i], &caseMer);

    if(caseMer == MER_CASE_LIBRE){
      verr_vois.l_start = liste_voisins->coords[i].pos;
      
      if(fcntl(fd_mer, F_SETLKW, &verr_vois) == -1 ){
        printf("Erreur lors de la pose/ retrait du verrou dans verouillerVoisin \n");
        return -1;
      }
    
    }

  }

  return 1;
}

int main( int nb_arg , char * tab_arg[] )
{
  int no_err = 0 ;
  char fich_mer[128] ;
  case_t marque = MER_CASE_LIBRE ;
  char nomprog[128] ;
  float energie = 0.0 ;

  int fd_mer;
  int nbBateaux = 0;
  int numRand;


  bateau_t * bateau = BATEAU_NULL ; 
  coords_t * liste_voisins = NULL ;
  booleen_t deplacement = FAUX;
  coord_t cible ;
  booleen_t acquisition ;
  booleen_t coule = FAUX;

  /*----------*/

  strcpy( nomprog , tab_arg[0] ) ;

  if( nb_arg != 4 )
    {
      fprintf( stderr , "Usage : %s <fichier mer> <marque> <energie>\n", 
	       nomprog );
      exit(-1);
    }

  if( strlen(tab_arg[2]) !=1 ) 
    {
      fprintf( stderr , "%s : erreur marque <%s> incorrecte \n",
	       nomprog , tab_arg[2] );
      exit(-1) ;
    }


  strcpy( fich_mer , tab_arg[1] );
  marque = tab_arg[2][0];
  sscanf( tab_arg[3] , "%f" , &energie);
  
  /* Initialisation de la generation des nombres pseudo-aleatoires */
  srandom((unsigned int)getpid());

  printf( "\n\n%s : ----- Debut du bateau %c (%d) -----\n\n ", nomprog , marque , getpid() );


  /*---------------------------------------------
  ------------- Ouverture fichier -------------
  ---------------------------------------------*/

  if( (fd_mer = open(fich_mer, O_CREAT|O_RDWR, 0666)) == -1){
    printf("Erreur sur le descripteur de fichier \n");
    return -1;
  }


  /*---------------------------------------------
  -------------- Création du bateau ------------
  ---------------------------------------------*/
  
  if( ( bateau = bateau_new( NULL, marque, getpid() ) ) == BATEAU_NULL ){
    printf("Sortie erreur\n") ; 
    exit(-1);
  }

    // Test affichage d'un bateau 
  printf("\nAffichage structure bateau: ");
  bateau_printf( bateau );
  printf("\n\n");

    // Placement du bateau sur dans le fichier mer
  if(( no_err = mer_bateau_initialiser( fd_mer , bateau)) ){
	  fprintf( stderr, "%s : erreur %d dans mer_bateau_initialiser\n", nomprog , no_err );
    if( no_err == ERREUR ){
	    fprintf( stderr, "\t(mer_bateau_initialiser n'a pas pu placer le bateau \"%c\" dans la mer)\n" ,bateau_marque_get(bateau) );
	  }
	  exit(no_err) ;
	}

    // On enregistre le nombre de bateaux dans la mer 
  mer_nb_bateaux_lire(fd_mer, &nbBateaux);
	mer_nb_bateaux_ecrire(fd_mer, ++nbBateaux);

  
    // On temporise un peu

// il faut poser un verrou non bloquant pour le bouclier setlk en ecriture
  marque = bateau_marque_get(bateau);

  printf("Pose du bouclier sur le bateau !!!! \n");

  if( (verrouillerBouclier(fd_mer, 1, bateau)) != 1 ){
    printf("Erreur dans la pose du bouclier avant le while \n");
    exit(0);
  }else{
    printf("Le bouclier est posé avant le while \n");
  }


  /*---------------------------------------------
  ---------- Boucle principale du jeu -----------
  ---------------------------------------------*/


  while (1){

    if(energie < BATEAU_SEUIL_BOUCLIER){
      
      if( (verrouillerBouclier(fd_mer, 0, bateau)) != 1){
        printf("Erreur dans la levee du bouclier dans le while \n");
        exit(0);
      }else{
        printf("Le verrou sur le bouclier est enlevé !!! \n");
      }
      
    }else{
      
      if( (verrouillerBouclier(fd_mer, 1, bateau)) != 1){
        printf("Erreur dans la pose du bouclier dans le while \n");
        exit(0);
      }else{
        printf("Le verrou sur le bouclier est posé !!! \n");
      }
      
    }

    if((no_err = mer_bateau_est_touche(fd_mer, bateau, &coule)))
			// Destruction d'un bateau si on a un probleme lors de l'acces a ses donnees
			{
				printf("Erreur dans la vérification de l'attaque sur un bateau\n");
				exit(no_err);
			}
    
    if(coule && energie < BATEAU_SEUIL_BOUCLIER){
      printf("C'est la mort ! \n");
      break;
    }
    
    printf("---------- Recherche de cases voisines \n -------");

    if((no_err = mer_voisins_rechercher( fd_mer, bateau, &liste_voisins )))
    {
      fprintf( stderr, "%s : erreur %d dans mer_voisins_rechercher \n", nomprog , no_err );
      exit(no_err) ;
    }

    printf( "------ Liste des voisins : \n");
    coords_printf( liste_voisins );
    printf("\n\n");

      
      // on verrouille les voisins 
    if ( (verrouillerVoisins(fd_mer, 1, liste_voisins)) != 1 ){
      printf("Erreur pose verrou de la liste des voisins dans while \n");
      exit(1);
    }else{
      printf("Verrou voisin posé ! \n");
    }

    // ------- Déplacement dans la mer --------

    if( (verrouillerBouclier(fd_mer, 0, bateau)) != 1 ){
      printf("Erreur dans la levee du bouclier dans le while \n");
     exit(0);
    }else{
      printf("Le bouclier est levé pourle déplacement !!!!!!!! \n");
    }

      // On se deplace dans la mer
    if( (no_err = mer_bateau_deplacer( fd_mer, bateau, liste_voisins, &deplacement)) ){
      fprintf( stderr, "%s : erreur %d dans mer_bateau_deplacer\n", nomprog , no_err );
      exit(no_err) ;
    }

    energie -= 0.25*energie;
    printf("Energie : %f \n", energie);

    //on déverrouille les voisins
    if ( (verrouillerVoisins(fd_mer, 0, liste_voisins)) != 1 ){
      printf("Erreur dans le verrou de la liste des voisins dans while \n");
      exit(1);
    }
    printf("Verrou voisin levé ! \n");
    
    

      // On cherche une cible
    if( (no_err = mer_bateau_cible_acquerir( fd_mer, bateau, &acquisition, &cible )) ){
      fprintf( stderr, "%s : erreur %d dans mer_bateau_cible_acquerir\n", nomprog , no_err );
      exit(no_err) ;
    }

	  if( acquisition ) {
      printf("Acquisition d'une cible par le bateau \n");
      bateau_printf( bateau );
      printf( "\n-->La cible choisie est la case ");
      coord_printf( cible );
      printf( "\n\n");

      // on tire sur la cible 
	    if( (no_err = mer_bateau_cible_tirer( fd_mer, cible)) ) {
        fprintf( stderr, "%s : erreur %d dans mer_bateau_cible_tirer\n", nomprog , no_err );
        exit(no_err) ;
      }

	  }else {
      printf("Pas d'acquisition de cible pour le bateau \n");
      bateau_printf( bateau );
      printf( "\n");
	  }
    
    numRand = rand() % 3;
    sleep(numRand);

    


    mer_nb_bateaux_lire(fd_mer, &nbBateaux);

  }

  mer_nb_bateaux_lire(fd_mer, &nbBateaux);
  
  if(nbBateaux == 1){
    printf("Le bateau %d à gagné ! \n", marque);
  }

  //destruction du bateau
  printf("\n - Bateau %c : Destruction -\n", marque);
  mer_nb_bateaux_ecrire(fd_mer, --nbBateaux);
  mer_bateau_couler(fd_mer, bateau);
  bateau_destroy(&bateau);


  close(fd_mer);

  printf( "\n\n%s : ----- Fin du navire %c (%d) -----\n\n ", 
	  nomprog , marque , getpid() );

  exit(0);
}
