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
#include <sys/select.h>

#include "user.h"

#define BUFSIZE 256
#define MAX_AFF 10
#define MAX_GHT 10
#define MAX_USR 100

int isInQueue(struct user *usr,int brn_inf,int brn_sup)
{
    int i;
    if(brn_inf>brn_sup)
    {
        for(i = brn_inf;i<MAX_USR;i++)
            if(strlen(usr[i].nom) != 0)
                return i;
        for(i=0;i<brn_sup;i++)
            if(strlen(usr[i].nom) != 0)
                return i;
        return -1;
    }
    else
    {
        for(i=brn_inf;i<brn_sup;i++)
            if(strlen(usr[i].nom)!=0)
                return i;
        return -1;
    }

}
int main(int argc,char**argv)
{
    int sock_acceuil,sock_service;
    struct sockaddr_in addr_loc;
    struct sockaddr_in addr_clt;
    struct hostent* hote;
    int port;
    char buf[BUFSIZE];
    int i,j;
    int retval;
    ssize_t nb_read;
    ssize_t nb_write;
    int clt;
    int size_addr_clt = sizeof(addr_clt);

    struct addr_cmp{
        struct sockaddr_in addr_cmp;
        int sock;
    };

    struct addr_cmp sock_aff[MAX_AFF];
    int nb_aff = 0;

    struct addr_cmp sock_ght[MAX_GHT];
    int nb_ght = 0;

    struct user usr_tab[MAX_USR];
    for(i = 0;i<MAX_USR;i++)
        bzero(usr_tab[i].nom,BUFSIZE);

    int nb_usr = 0;
    int usr_brn_inf = 0;
    int usr_brn_sup = 0;

    struct addr_cmp sock_brn;
    sock_brn.sock = 0;
    fd_set rfds;

    if(argc != 2)
    {
        printf("usage : %s port\n",argv[0]);
        exit(1);
    }
    if((sock_acceuil = socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        perror("socket");
        exit(1);
    }

    port = (unsigned int)atoi(argv[1]);

    addr_loc.sin_family = AF_INET;
    addr_loc.sin_port = htons((uint16_t)port);
    addr_loc.sin_addr.s_addr = htonl(INADDR_ANY);

    if((bind(sock_acceuil,(struct sockaddr*)&addr_loc,sizeof(addr_loc))) == -1)
    {
        printf("erreur lors de l'attachement de la socket decoute\n");
        perror("bind");
        exit(1);
    }

    if(listen(sock_acceuil,10)==-1)
    {
        printf("erreur lors de la mise en place du listen\n");
        perror("listen");
        exit(-1);
    }

    if((sock_service = accept(sock_acceuil,(struct sockaddr*)&addr_clt,(socklen_t*)&size_addr_clt) == -1))
    {
        printf("erreur lors de l'accept\n");
        perror("accept");
        exit(-1);
    }



    while(1)
    {
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO,&rfds); //clavier
        FD_SET(sock_acceuil,&rfds);
        if(sock_brn.sock)
            FD_SET(sock_brn.sock,&rfds); //socket de borne unique
        for(i=0;i<nb_aff;i++)
            FD_SET(sock_aff[i].sock,&rfds); //socket des afficheurs
        for(i=0;i<nb_ght;i++)
            FD_SET(sock_ght[i].sock,&rfds); //socket des guichets

        retval = select(FD_SETSIZE,&rfds,NULL,NULL,NULL);

        //erreur de select
        if(retval<0)
        {
            printf("erreur lors de l'utilisation du select\n");
            perror("select");
            exit(-1);
        }

        // touche clavier "i", on affiche les afficheurs et les guichets ainsi que leurs adresses
        // touche clavier "u", on afficher des infos sur la liste d'attente
        // touche clavier "q", on quitte l'application
        if(FD_ISSET(STDIN_FILENO,&rfds))
        {
            if((nb_read = read(STDIN_FILENO,buf,BUFSIZE)) < 0)
            {
                printf("erreur lors de la saisie clavier\n");
                perror("read");
                exit(-1); //TODO handle & retry
            }

            if(strcmp(buf,"i") == 0)
            {
                printf("Types     adresse       port\n");
                for(i = 0;i<nb_aff;i++)
                    printf("afficheur  %s   %d\n",inet_ntoa(sock_aff[i].addr_cmp.sin_addr),sock_aff[i].addr_cmp.sin_port);
                for(i = 0;i<nb_ght;i++)
                    printf("guichet    %s   %d\n",inet_ntoa(sock_ght[i].addr_cmp.sin_addr),sock_ght[i].addr_cmp.sin_port);
                continue;
            }
            if(strcmp(buf,"q")==0)
            {
                exit(0); // ouais bon faut faire mieux sans doute
            }
        }

        if(FD_ISSET(sock_acceuil,&rfds))// nouvelle connexion gestion d erreur type envoie conf ?
        {
            sock_service = accept(sock_acceuil,(struct sockaddr*)&addr_clt,(socklen_t *)&size_addr_clt);
            if(sock_service == -1)
            {
                printf("erreur lors de l'accept\n");
                perror("accept");
                exit(-1);
            }

            if((nb_read = read(sock_service,buf,BUFSIZE))<0)
            {
                printf("erreur lors du read\n");
                perror("read");
                exit(-1);
            }

            if(strcmp(buf,"brn")==0)
            {
                sock_brn.sock = sock_service;
                sock_brn.addr_cmp = addr_clt;
            }
            else if(strcmp(buf,"ght")==0)
            {
                if(nb_ght <= MAX_GHT -1)  //[0-9]
                {
                    sock_ght[nb_ght].sock = sock_service;
                    sock_ght[nb_ght].addr_cmp = addr_clt;
                    nb_ght++;
                }
            }
            else if(strcmp(buf,"aff")==0)
            {
               if(nb_aff<=MAX_AFF-1)
               {
                   sock_aff[nb_aff].sock = sock_service;
                   sock_aff[nb_aff].addr_cmp = addr_clt;
                   nb_aff++;
               }
            }
        }
        if(FD_ISSET(sock_brn.sock,&rfds))
        {
            if((nb_read = read(sock_brn.sock,&usr_tab[usr_brn_sup],sizeof(struct user)))<0)
            {
                printf("error reception message\n");
                perror("read");
                exit(-1);
            }
            usr_brn_sup++;
            nb_usr++;
            if(usr_brn_sup == MAX_USR-1)
                usr_brn_sup = 0;
        }
        for(i = 0;i<nb_aff;i++)
            if(FD_ISSET(sock_aff[i].sock,&rfds))
            {

            }

        for(i=0;i<nb_ght;i++)
            if(FD_ISSET(sock_ght[i].sock,&rfds))
            {
                if((nb_read = read(sock_ght[i].sock,buf,BUFSIZE)) < 1)
                {
                    if(strcmp(buf,"1")==0)//demande d'un client
                    {
                        clt = isInQueue(usr_tab,usr_brn_inf,usr_brn_sup);
                        if(clt == -1)//pas de client
                        {
                            nb_write = write(sock_ght[i].sock,"-1",1);
                            if(nb_write!=1)
                            {
                                printf("erreur writ\n");
                                perror("write");
                                exit(-1);
                            }
                        }
                        else //il y a un client
                        {
                            nb_write = write(sock_ght[i].sock,"1",1); //conf on envoie client
                            if(nb_write!=1)
                            {
                                printf("erreur writ\n");
                                perror("write");
                                exit(-1);
                            }

                           nb_write = write(sock_ght[i].sock,&usr_tab[clt],sizeof(struct user));
                           if(nb_write != sizeof(struct user))
                           {
                               printf("erreur lors de l'envoi des donnes clients\n");
                               perror("write");
                               exit(-1);
                           }
                           nb_write = write(sock_ght[i].sock,&clt,sizeof(int));
                           if(nb_write != sizeof(int))
                           {
                               printf("erreur lors de l'envoi de l'indice du client\n");
                               perror("write");
                               exit(-1);
                           }

                        }
                    }
                    else if(strcmp(buf,"2") == 0)//confirmation reception client
                    {
                        nb_read = read(sock_ght[i].sock,&clt,sizeof(int));
                        if(nb_read != sizeof(int))
                        {
                            printf("erreur lors de la reception de l'indice client\n");
                            perror("read");
                            exit(-1);
                        }
                        bzero(usr_tab[clt].nom,BUFSIZE);
                        nb_usr--;
                        usr_brn_inf++;
                        if(usr_brn_inf == MAX_USR-1)
                            usr_brn_inf = 0;

                        //TODO IMPORTANT faut enlever le guichet et le client de l'affichage
                        for(j=0;j<nb_aff;j++)
                        {
                            nb_write = write(sock_aff[j].sock,&i,sizeof(int)); // on envoie i, peut être on devrais envoyer le num du guichet entrée par l'user
                            if(nb_write != sizeof(int))
                            {
                                perror("erreur lors de l'envoi de l'indice guichet");
                                exit(-1);
                            }
                        }
                    }
                }
            }

        break; //a suppr avant de compil
    }


}