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
#include "const.h"


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
        printf("%s%d guichet=> N° %s\n",tab_aff[i].usr.nom,tab_aff[i].usr.id,tab_aff[i].nb_guichet);
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
    size_t len_name_clt;


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
    if((nb_write = write(sock,AFF_IDENTIFIER,strlen(AFF_IDENTIFIER)) != strlen(AFF_IDENTIFIER)))
    {
        printf("erreur à l'envoi de l'identifiant\n");
        perror("write");
        exit(1);
    }
    while(1)
    {   bzero(buf,BUFSIZE);
        if ((nb_read = read(sock, buf, GHT_SIZE_CONST)) != GHT_SIZE_CONST) {
            printf("erreur lors de la reception du msg\n");
            perror("read");
            exit(1);
        }
        printf("receive by afficheur : %s\n",buf);
        if (strcmp(buf,AFF_ASKADD) == 0)//envoi d'une demande d'affichage
        {
            //reception de l id
            if ((nb_read = read(sock, &tab_aff[size_tab_aff].usr.id, sizeof(int))) != sizeof(int)) {
                printf("erreur lors de la reception de l id %d\n",tab_aff[size_tab_aff].usr.id);
                perror("read");
                exit(1);
            }
            //reception taille du nom
            if ((nb_read = read(sock,&len_name_clt, sizeof(int))) != sizeof(int)) {
                printf("erreur lors de la reception de la longueur du nom\n");
                perror("read");
                exit(1);
            }
            //reception du nom du client
            if ((nb_read = read(sock,&tab_aff[size_tab_aff].usr.nom,len_name_clt)) != len_name_clt) {
                printf("erreur lors de la reception du nom client recv %ld %s oct au lieu %ld\n",nb_read,tab_aff[size_tab_aff].usr.nom,len_name_clt);
                perror("read");
                exit(1);
            }
            //reception du numero de guichet
            if ((nb_read = read(sock, &tab_aff[size_tab_aff].nb_guichet, BUFSIZE)) < 0) {
                printf("erreur lors de la reception du numero guichet\n");
                perror("read");
                exit(1);
            }
            size_tab_aff++;
            display();

        }
        else if (strcmp(buf,AFF_ASKRET) == 0) // "1" => envoi d'une demande de retrait
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