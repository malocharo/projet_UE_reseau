#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <memory.h>


#include "user.h"
#include "const.h"
#define DEBUG 08


int appel_client()
{
    char choix;
    printf("Voulez vous appeler un client ?[O/N]\n");
    fflush(stdin);
    fflush(stdout);

    scanf(" %c",&choix);
    printf("%c",choix);
    if(choix == 'O' || choix == 'o')
        return 1;
    return 0;
}

int main(int argc,char **argv) {
    char num_gui[BUFSIZE_MIN];
    size_t length_num_gui;
    int sock;
    uint16_t port;
    struct hostent *hote = NULL;
    struct sockaddr_in addr_serv;
    ssize_t nb_write;
    ssize_t nb_read;
    char buf_recv[BUFSIZE];
    int dmd_lib = 1;
    struct user usr;
    char c = '\0';

    int ind_clt;

    if(argc < 2)
    {
        printf("usage : %s port ip\n",argv[0]);
        exit(-1);
    }
    printf("Bonjour, votre numero :\n");
    scanf("%s",num_gui);

    if((sock = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        printf("Impossible de créer la socket\n");
        perror("socket");
        exit(-1);
    }

    if((hote = gethostbyname(argv[2]))==NULL)
    {
        printf("Impossible de trouver l'hote\n");
        perror("gethostbyname");
        exit(-1);
    }
    port = (uint16_t)atoi(argv[1]);
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(port);
    bcopy(hote->h_addr_list[0],&addr_serv.sin_addr,(size_t)hote->h_length);

    if(connect(sock,(struct sockaddr*)&addr_serv,sizeof(addr_serv))==-1)
    {
        printf("erreur lors de la demande de connexion avec le serveur\n");
        perror("connect");
        exit(-1);
    }
    // envoie de l identifiant "ght" pour identification aupres du serv gestion
    if((nb_write = write(sock,GHT_IDENTIFIER,strlen(GHT_IDENTIFIER)) != strlen(GHT_IDENTIFIER)))
    {
        printf("erreur à l'envoi de l'identifiant\n");
        perror("write");
        exit(1);
    }

    //Envoye son numéro
    length_num_gui = strlen(num_gui);
    if((nb_write = write(sock,&length_num_gui,sizeof(int))) != sizeof(int))
    {
        printf("erreur à l'envoi de la taille de l'identifiant\n");
        perror("write");
        exit(1);
    }
    if((nb_write = write(sock,num_gui,strlen(num_gui)) != strlen(num_gui)))
    {
        printf("erreur à l'envoi de l'identifiant\n");
        perror("write");
        exit(1);
    }
    printf("identifiant envoyé %s\n",num_gui);
    while(1)
    {
        fflush(stdin);
        do
        {
            printf("voulez vous un client ? [o/n]\n");
            c = (char)getchar();
        }while(c != 'o');

        nb_write = write(sock,GHT_ASKCLT,strlen(GHT_ASKCLT)); //"1" => demande de client , je suis libre
        if(nb_write != strlen(GHT_ASKCLT))
        {
            printf("erreur lors de l'envoie de la demande de client\n");
            if(nb_write == -1)
            {
                perror("write socket;");
            }
            exit(1); //TODO handle & retry
        }

        nb_read = read(sock,buf_recv,GHT_SIZE_CONST); // égal a 1 sinon pas de limite de message toussa
        if(nb_read<0)
        {
            printf("erreur lors de la reception\n");
            perror("recept");
            exit(1);
        }
        else if((nb_read == GHT_SIZE_CONST) && (strcmp(buf_recv,GHT_NOCLT) == 0)) //"3" => pas de client
        {
            printf("pas de client en attente\n");
            continue;
        }

        else if((nb_read == GHT_SIZE_CONST) && (strcmp(buf_recv,GHT_EXIT) == 0))//"3" => pas de client + exit    !On ferme donc le guichet!
        {
            printf("pas de client en attente et c'est la fin !\n");
            close(sock);
            return 0;
        }

        else if(strcmp(buf_recv,GEST_CONFCLT) == 0) // il y'a un client
        {
            if((nb_read = read(sock,&usr.id,sizeof(int)))<0)
            {
                printf("erreur reception donne client\n");
                perror("recp");
                exit(-1);
            }
            if((nb_read = read(sock,&ind_clt,sizeof(int)))<0)
            {
                printf("erreur reception donne client\n");
                perror("recp");
                exit(-1);
            }
            if((nb_read = read(sock,&usr.nom,BUFSIZE))<0)
            {
                printf("erreur reception donne client\n");
                perror("recp");
                exit(-1);
            }

            printf("client : %s %d\n",usr.nom,usr.id);
            printf("Ecrire 'ok' pour valider la présence du client\n");
            scanf("%s",buf_recv);
            while(strcmp(buf_recv,"ok") != 0)
            {
                printf("Ecrire 'ok' pour valider la présence du client\n");
                scanf("%s",buf_recv);
            }

            nb_write = write(sock,GHT_CLTCONF,strlen(GHT_CLTCONF)); //"2" => client recu, pas libre,afficher sur afficheur
            if(nb_write != 1)
            {
                printf("erreur lors de l'envoie de la demande de client\n");
                if(nb_write == -1)
                {
                    perror("write socket;");
                }
                exit(1); //TODO handle & retry
            }
            nb_write = write(sock,&ind_clt,sizeof(int)); //=> envoie indice client pour enlever des afficheurs
            if(nb_write != sizeof(int))
            {
                printf("erreur lors de l'envoie de la demande de client\n");
                perror("write socket;");
                exit(1); //TODO handle & retry
            }

        }

    }




}