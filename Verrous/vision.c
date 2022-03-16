#include <stdio.h>
#include <commun.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <mer.h>


int verouillerMer(int fd_mer,int etat){
  struct flock verr_mer;

  verr_mer.l_whence = SEEK_SET;
  verr_mer.l_start = 0;
  verr_mer.l_len = 0;

  if(etat){
    verr_mer.l_type = F_WRLCK;
  }else{
    verr_mer.l_type = F_UNLCK;
  }

  if((fcntl(fd_mer, F_SETLKW, &verr_mer)) == -1 ){
    printf("Erreur lors de la pose/ retrait du verrou dans verouillerMer \n");
    return -1;
  }
  return 1;
}

/*--------------------* 
 * Main demon 
 *--------------------*/
int main( int nb_arg , char * tab_arg[] ){
     char fich_mer[128] ;
     char nomprog[256] ;

     /*----------*/

     if( nb_arg != 2 )
     {
          fprintf( stderr , "Usage : %s <fichier mer>\n", 
               tab_arg[0] );
          exit(-1);
     }

     strcpy( nomprog , tab_arg[0] );
     strcpy( fich_mer , tab_arg[1] );


     printf("\n%s : ----- Debut de l'affichage de la mer ----- \n", nomprog );

     /***********/
     /* A FAIRE */
     /***********/

     int fd_mer;

     if((fd_mer = open(fich_mer, O_CREAT|O_RDONLY, 0666))== -1){
          printf("Erreur sur le fichier mer ... \n");
          return -1;
     }


     struct stat stat_mer_base;
     struct stat stat_mer_new;

     if(stat(fich_mer,&stat_mer_base) == -1 || stat(fich_mer,&stat_mer_new) == -1 ){
          printf("Erreur dans la fonction stat... \n");
          return -1;
     }

     mer_afficher(fd_mer);

     
     while (1){
          if(difftime(stat_mer_base.st_mtime, stat_mer_new.st_mtime)){
               if(stat(fich_mer,&stat_mer_base) == -1 || stat(fich_mer,&stat_mer_new) == -1 ){
                    printf("Erreur dans la fonction stat... \n");
                    exit(0);
               }
               
               mer_afficher(fd_mer);
          }
          
          stat(fich_mer,&stat_mer_new);

     }
     
     close(fd_mer);
     printf("\n%s : --- Arret de l'affichage de la mer ---\n", nomprog );

     exit(0);
}