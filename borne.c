//
// Created by malo on 07/02/18.
//

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

#define BUFSIZE 256

int main(int argc,char **argv)
{
    int sock;
    struct sockaddr_in addr_serv;
    struct hostent *hote = NULL;
    int port;
    int uid = 0;
    int bool = 1;
    ssize_t nb_write;
    char *borne = "brn";


    struct user usr;
    if(argc<2)
    {
        printf("usage %s port ip\n",argv[0]);
        exit(0);
    }

    if((sock = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        printf("erreur creation de la socket \n");
        perror("socket");
        exit(1);
    }

    if((hote = gethostbyname(argv[2])) == NULL)
    {
        printf("erreur gethostbyname\n");
        perror("gethostbyname");
        exit(1);
    }
    port = (unsigned int)atoi(argv[1]);

    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port   = htons((uint16_t)port);
    bcopy(hote->h_addr,&addr_serv.sin_addr,(size_t)hote->h_length);

    if(connect(sock,(struct sockaddr*)&addr_serv,sizeof(addr_serv)) == -1)
    {
        printf("erreur lors de la connexion à la salle d'attente\n");
        perror("connect");
        exit(-1);
    }
    // envoie de "brn" pour identifier la connexion comme borne pour la gestion
    if((nb_write = write(sock,borne,strlen(borne)) != strlen(borne)))
    {
        printf("erreur à l'envoi de l'identifiant\n");
        perror("write");
        exit(1);
    }

    while(bool)
    {
        printf("Votre nom svp :\n");
        scanf("%s",usr.nom);
        if(strcmp(usr.nom,"exit") == 0)// si le nom est "exit" on quitte le borne
            break;
        usr.id = uid;
        uid++;

        if((nb_write = write(sock,&usr,sizeof(usr))) == -1)
        {
            printf("erreur envoie des donnees clients\n");
            exit(1);//TODO handle & retry

        }
        printf("donnees envoyees\n");
    }
    close(sock);

}