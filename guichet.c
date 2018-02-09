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
#define DEBUG 08
#define BUFSIZE 512

int appel_client()
{
    char choix;
    printf("Voulez vous appeler un client ?[O/N]\n");
    scanf(" %c",&choix);
    if(choix == 'O' || choix == 'o')
        return 1;
    return 0;
}

int main(int argc,char **argv) {
    int num_gui;
    int sock;
    uint16_t port;
    struct hostent *hote = NULL;
    struct sockaddr_in addr_serv;
    ssize_t nb_write;
    ssize_t nb_read;
    char buf_recv[BUFSIZE];
    int dmd_lib = 1;
    struct user usr;
    char *guichet = "ght";
    int ind_clt;

    if(argc < 2)
    {
        printf("usage : %s port ip\n",argv[0]);
        exit(-1);
    }
    printf("Bonjour, votre numero :\n");
    scanf("%d",&num_gui);

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
    if((nb_write = write(sock,guichet,strlen(guichet)) != strlen(guichet)))
    {
        printf("erreur à l'envoi de l'identifiant\n");
        perror("write");
        exit(1);
    }

    while(appel_client())
    {
        nb_write = write(sock,"1",1); //"1" => demande de client , je suis libre
        if(nb_write != 1)
        {
            printf("erreur lors de l'envoie de la demande de client\n");
            if(nb_write == -1)
            {
                perror("write socket;");
            }
            exit(1); //TODO handle & retry
        }

        nb_read = read(sock,buf_recv,BUFSIZE);
        if(nb_read<0)
        {
            printf("erreur lors de la reception\n");
            if(nb_read==-1)
            {
                perror("recept");
            }
            exit(1);
        }
        else if((nb_read == 1) && (strcmp(buf_recv,"-1") != 0)) //"-1" => pas de client
        {
            printf("pas de client en attente\n");
            continue;
        }
        else // client
        {
            if((nb_read = read(sock,&usr,sizeof(usr)))<0)
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

            printf("client : %s %d\n",usr.nom,usr.id);

            nb_write = write(sock,"2",1); //"2" => client recu, pas libre,afficher sur afficheur
            if(nb_write != 1)
            {
                printf("erreur lors de l'envoie de la demande de client\n");
                if(nb_write == -1)
                {
                    perror("write socket;");
                }
                exit(1); //TODO handle & retry
            }
            nb_write = write(sock,&ind_clt,sizeof(int)); //"2" => client recu, pas libre,afficher sur afficheur
            if(nb_write != sizeof(int))
            {
                printf("erreur lors de l'envoie de la demande de client\n");
                perror("write socket;");
                exit(1); //TODO handle & retry
            }

        }

    }




}