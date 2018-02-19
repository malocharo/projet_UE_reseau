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

struct aff{
    struct user usr;
    char nb_guichet[BUFSIZE];
}tab_aff[100];

size_t size_tab_aff = 0;

int display()
{
    int i;
    system("clear");
    for(i = 0;i<size_tab_aff;i++)
    {
        printf("%s%d guichet=> N°%s\n",tab_aff[i].usr.nom,tab_aff[i].usr.id,tab_aff[i].nb_guichet);
    }
}

int erase(char *nb_gui)
{
    int i;
    for(i = 0;i<size_tab_aff;i++)
        if(strcmp(nb_gui,tab_aff[i].nb_guichet) == 0)
        {
            strcpy(tab_aff[i].nb_guichet,tab_aff[size_tab_aff].nb_guichet);
            tab_aff[i].usr = tab_aff[size_tab_aff].usr;
            size_tab_aff--;
            /*normalement il ne peut y avoir qu'une seule ligne par guichet,
             * sinon il faut sortir les deux fonctions suivantes du if*/
            display();
            return i;
        }
}

int main(int argc, char**argv)
{

    int sock;
    struct hostent* hote;
    int port;
    ssize_t nb_read;
    struct sockaddr_in addr_serv;
    char buf[BUFSIZE];
    ssize_t nb_write;
    char *affichage = "aff";

    if(argc < 2)
    {
        printf("usage : %s port ip\n",argv[0]);
        exit(-1);
    }


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
    bcopy(hote->h_addr,&addr_serv.sin_addr,(size_t)hote->h_length);

    if(connect(sock,(struct sockaddr*)&addr_serv,sizeof(addr_serv))==-1)
    {
        printf("erreur lors de la demande de connexion avec le serveur\n");
        perror("connect");
        exit(-1);
    }
    //envoie de l'identifant "aff" pour identification par le serv gestion
    if((nb_write = write(sock,affichage,strlen(affichage)) != strlen(affichage)))
    {
        printf("erreur à l'envoi de l'identifiant\n");
        perror("write");
        exit(1);
    }
    while(1)
    {
        if ((nb_read = read(sock, buf, BUFSIZE)) < 0) {
            printf("erreur lors de la reception du msg");
            perror("read");
            exit(1);
        }
        if (atoi(buf) == 0)//envoi d'une demande d'affichage
        {
            //reception de la struct usr
            if ((nb_read = read(sock, &tab_aff[size_tab_aff].usr, sizeof(struct user))) < 0) {
                printf("erreur lors de la reception du struct client");
                perror("read");
                exit(1);
            }
            //reception du numero de guichet
            if ((nb_read = read(sock, &tab_aff[size_tab_aff].nb_guichet, BUFSIZE)) < 0) {
                printf("erreur lors de la reception du struct client");
                perror("read");
                exit(1);
            }
            size_tab_aff++;
            display();

        }
        else if (atoi(buf) == 1) //envoi d'une demande de retrait
        {
            if ((nb_read = read(sock, buf, BUFSIZE)) < 0) //on attend le num du guichet
            {
                printf("erreur lors de la reception du msg");
                perror("read");
                exit(1);
            }
            erase(buf);
        }
    }


}