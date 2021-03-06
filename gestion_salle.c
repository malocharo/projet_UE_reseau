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
#include <wait.h>

#include "user.h"
#include "const.h"

//structure permet de stocker les infos pour chaque composant connecté

struct addr_cmp{
    struct sockaddr_in addr_cmp;
    struct sockaddr_in addr_cmp_udp;
    int sock; //socket tcp
    char num[BUFSIZE];// utilise par les guichets pour transmettre leurs numéros jusqu'a l'affichage

};
//permet de trouver un user en attente
int isInQueue(struct user *usr,int brn_inf,int brn_sup)
{
    int i;
    if(brn_inf>brn_sup)
    {
        for(i = brn_inf;i<MAX_USR;i++)
            if(strlen(usr[i].nom) != 0 && usr[i].state == CONF)
                return i;
        for(i=0;i<brn_sup;i++)
            if(strlen(usr[i].nom) != 0 && usr[i].state == CONF)
                return i;
        return -1;
    }
    else
    {
        for(i=brn_inf;i<brn_sup;i++)
            if(strlen(usr[i].nom)!=0 && usr[i].state == CONF)
                return i;
        return -1;
    }

}
//fonction générique permet d'enlever un composant lors de sa deconnection
void remove_socket(int index,struct addr_cmp *addr,int* len_struct)
{
    if(*len_struct<=0)
        return;
    addr[index].sock = -1;
    addr[index] = addr[*len_struct-1];
    *len_struct = *len_struct - 1;
}

int main(int argc,char**argv) {
    int sock_acceuil, sock_service, sock_udp, sock_udp_send;
    struct sockaddr_in addr_loc;
    struct sockaddr_in addr_loc_udp;
    int sockaddr_in_size = sizeof(struct sockaddr_in);
    struct sockaddr_in addr_clt;
    struct hostent *hote;
    int port;
    char buf[BUFSIZE];
    int i, j;
    int retval;
    ssize_t nb_read;
    ssize_t nb_write;
    size_t length_num_gui;
    size_t length_name_clt = 0;
    int clt;
    int size_addr_clt = sizeof(addr_clt);
    int uid = 0;
    int brn_flag_exit = BRN_EXIT_NO_SET;//Si la borne à exit ou non
    int sock_opt = 1; //option de la socket d'acceuil
    int brn_state = BRN_NOTCONN; //permet de connaitre l'état de connection de la borne
    pid_t pid;
    int status;

    struct addr_cmp sock_aff[MAX_AFF]; //tableau de structure permettant de stocker tous les composants de type afficheur
    int nb_aff = 0;

    struct addr_cmp sock_ght[MAX_GHT]; //tableau de structure permettant de stocker tous les composants de type guichet
    int nb_ght = 0;

    struct user usr_tab[MAX_USR]; //tableau de structure permettant de stocker les utilisateurs
    for (i = 0; i < MAX_USR; i++) {
        bzero(usr_tab[i].nom, BUFSIZE); // on initialiser les noms a zero car on les utilises pour comparer ensuite
        usr_tab[i].state = CONF;
    }
    int nb_usr = 0;
    int usr_brn_inf = 0;
    int usr_brn_sup = 0;

    struct addr_cmp sock_brn; // structure pour stocker la borne
    sock_brn.sock = 0;

    struct addr_cmp sock_sup; // strucutre pour stocker la connection superviseur

    fd_set rfds;

    if (argc != 3) {
        printf("usage : %s port TCP port UPD\n", argv[0]);
        exit(1);
    }
    if ((sock_acceuil = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    //permet de reutiliser la socket immédiatement après la fermeture de l'application
    if (setsockopt(sock_acceuil, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt)) != 0) {
        printf("1 : erreur lors de la configuration de la socket d'acceuil\n");
        perror("setsockopt");
        exit(-1);
    }

    port = (unsigned int) atoi(argv[1]);

    addr_loc.sin_family = AF_INET;
    addr_loc.sin_port = htons((uint16_t) port);
    addr_loc.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((bind(sock_acceuil, (struct sockaddr *) &addr_loc, sizeof(addr_loc))) == -1) {
        printf("2 : erreur lors de l'attachement de la socket decoute\n");
        perror("bind");
        exit(1);
    }

    if (listen(sock_acceuil, NB_MAX_PENDINGQUEUE) == -1) {
        printf("3 : erreur lors de la mise en place du listen\n");
        perror("listen");
        exit(-1);
    }

    if ((sock_udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("erreur lors de la creation de la socket UDP\n");
        perror("socket");
        exit(-1);
    }

    addr_loc_udp.sin_family = AF_INET;
    addr_loc_udp.sin_port = htons((uint16_t) atoi(argv[2]));
    addr_loc_udp.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_udp, (struct sockaddr *) &addr_loc_udp, sizeof(addr_loc_udp)) == -1) {
        printf("erreur lors du bind de la socket udp\n");
        perror("bind");
        exit(-1);
    }


    while (1) {
        system("clear");
        printf("touche clavier i, affiche les afficheurs et les guichets ainsi que leurs adresses\n");
        printf("touche clavier l, afficher des infos sur la liste d'attente\n");
        printf("touche clavier q, on quitte l'application\n");
        fflush(stdin);
        fflush(stdout);
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds); //clavier
        FD_SET(sock_acceuil, &rfds);
        if (brn_state == BRN_CONN) // sinon boucle sans fin sur la socket close
            FD_SET(sock_brn.sock, &rfds); //socket de borne unique

        for (i = 0; i < nb_aff; i++)
            FD_SET(sock_aff[i].sock, &rfds); //socket des afficheurs
        for (i = 0; i < nb_ght; i++)
            FD_SET(sock_ght[i].sock, &rfds); //socket des guichets
        FD_SET(sock_udp, &rfds);


        retval = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);

        //erreur de select
        if (retval < 0) {
            printf("5 : erreur lors de l'utilisation du select\n");
            perror("select");
            exit(-1);
        }

        // touche clavier "i", on affiche les afficheurs et les guichets ainsi que leurs adresses
        // touche clavier "l", on afficher des infos sur la liste d'attente
        // touche clavier "q", on quitte l'application

        if (FD_ISSET(sock_udp, &rfds)) //connexion udp
        {

            pid = fork();
            if (pid == -1) {
                printf("erreur lors du fork\n");
                perror("fork");
                exit(-1);
            } else if (!pid) { //on est dans le fils
                bzero(buf, BUFSIZE);
                // on recois le message depuis le superviseur et on le renvoie immediatement
                if ((nb_read = recvfrom(sock_udp, buf, BUFSIZE, 0, (struct sockaddr *) &sock_sup.addr_cmp,
                                        (socklen_t * ) & sockaddr_in_size)) < 0) {
                    perror("recvfrom");
                    exit(errno);
                }

                if ((sock_udp_send = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
                    printf("erreur lors de la creation de la socket UDP\n");
                    perror("socket");
                    exit(-1);
                }
                //envoie du message a tous les guichets
                for (i = 0; i < nb_ght; i++) {
                    if ((nb_write = sendto(sock_udp_send, buf, strlen(buf), 0,
                                           (const struct sockaddr *) &sock_ght[i].addr_cmp_udp,
                                           (unsigned int) sockaddr_in_size)) != strlen(buf)) {
                        printf("erreur lors de l'envoie du message %s au guichet %d\n", buf, i);
                        perror("sendto");
                        exit(-1);
                    }
                }
                close(sock_udp_send);
                close(sock_udp);
                exit(0);

            } else { // on est dans le pere
                waitpid(-1, &status, 0);
                if (!WIFEXITED(status)) {
                    printf("Erreur le fils ce termine anormalement status = %d\n", WEXITSTATUS(status));
                }


            }
            if (FD_ISSET(STDIN_FILENO, &rfds)) { //interaction clavier
                bzero(buf, BUFSIZE);
                buf[0] = (char) getchar();

                if (buf[0] == 'i') {
                    printf("Types     adresse       port    opt\n");

                    if (brn_state == BRN_CONN) {
                        printf("borne  %s   %d\n", inet_ntoa(sock_brn.addr_cmp.sin_addr), sock_brn.addr_cmp.sin_port);
                    }
                    for (i = 0; i < nb_aff; i++)
                        printf("afficheur  %s   %d\n", inet_ntoa(sock_aff[i].addr_cmp.sin_addr),
                               sock_aff[i].addr_cmp.sin_port);
                    for (i = 0; i < nb_ght; i++)
                        printf("guichet    %s   %d    %s\n", inet_ntoa(sock_ght[i].addr_cmp.sin_addr),
                               sock_ght[i].addr_cmp.sin_port, sock_ght[i].num);
                } else if (buf[0] == 'q') {
                    exit(0);
                } else if (buf[0] == 'l') {
                    printf("Il y'a %d personnes en attente\n", nb_usr);
                    for (i = 0; i < NB_MAX_PENDINGQUEUE; i++) {
                        if (strlen(usr_tab[i].nom) != 0)
                            printf("%s%d\n", usr_tab[i].nom, usr_tab[i].id);
                    }
                }
                continue;
            }

            if (FD_ISSET(sock_acceuil, &rfds))// nouvelle connexion
            {
                sock_service = accept(sock_acceuil, (struct sockaddr *) &addr_clt, (socklen_t * ) & size_addr_clt);
                printf("nouvelle connexion\n");
                if (sock_service == -1) {
                    printf("13 : erreur lors de l'accept\n");
                    perror("accept");
                    exit(-1);
                }
                bzero(buf, BUFSIZE);
                // recuperation d'un identifiant composant qui permet de définir l'action à effectuer
                if ((nb_read = read(sock_service, buf, GHT_SIZE_IDENTIFIER)) < 0) {
                    printf("14 : erreur lors du read\n");
                    perror("read");
                    exit(-1);
                }

                //si le composant est une borne
                if (strcmp(buf, BRN_IDENTIFIER) == 0) {
                    if ((nb_write = write(sock_service, &uid, sizeof(int)) != sizeof(int))) {
                        printf("16 ; erreur à l'envoi de l'identifiant\n");
                        perror("write");
                        exit(1);
                    }

                    sock_brn.sock = sock_service;
                    sock_brn.addr_cmp = addr_clt;
                    brn_state = BRN_CONN;
                    printf("nouvelle borne ajouté uid = %d\n", uid);
                //si le composant est un guichet
                } else if (strcmp(buf, GHT_IDENTIFIER) == 0) {
                    if ((nb_read = read(sock_service, &length_num_gui, sizeof(int))) != sizeof(int)) {
                        printf("18 : erreur lors de la reception de la taille du guichet\n");
                        perror("read");
                        exit(-1);
                    }
                    bzero(buf, BUFSIZE);
                    if ((nb_read = read(sock_service, buf, length_num_gui)) != length_num_gui) {
                        printf("19 : erreur lors de la reception du numero de guichet\n");
                        perror("read");
                        exit(-1);
                    }

                    if (nb_ght <= MAX_GHT - 1)  //[0-9]
                    {
                        sock_ght[nb_ght].sock = sock_service;
                        sock_ght[nb_ght].addr_cmp = addr_clt;
                        sock_ght[nb_ght].addr_cmp_udp = addr_clt;
                        sock_ght[nb_ght].addr_cmp_udp.sin_port = htons(
                                (uint16_t) atoi(argv[2]) + (uint16_t) atoi(buf));
                        strcpy(sock_ght[i].num, buf); //numero du guichet
                        nb_ght++;
                        printf("nouveau guichet ajouté\n");
                    }
                //si composant afficheur
                } else if (strcmp(buf, AFF_IDENTIFIER) == 0) {
                    if (nb_aff <= MAX_AFF - 1) {
                        sock_aff[nb_aff].sock = sock_service;
                        sock_aff[nb_aff].addr_cmp = addr_clt;
                        nb_aff++;
                        printf("nouvelle afficheur ajouté\n");
                    }
                }

            }

            //Gestion de la borne
            if (FD_ISSET(sock_brn.sock, &rfds)) {

                //receive id,name
                if ((nb_read = read(sock_brn.sock, &usr_tab[usr_brn_sup].id, sizeof(int))) < 0) {
                    printf("22 : error reception message\n");
                    perror("read");
                    exit(-1);
                }

                if ((nb_read = read(sock_brn.sock, &usr_tab[usr_brn_sup].nom, BUFSIZE)) < 0) {
                    printf("23 : error reception message\n");
                    perror("read");
                    exit(-1);
                }
                printf("recu : %s %d depuis la borne\n", usr_tab[usr_brn_sup].nom, usr_tab[usr_brn_sup].id);


                //Si c'est la fin de la journée (On coupe proprement)
                if (strcmp(usr_tab[usr_brn_sup].nom, BRN_EXIT) == 0) {
                    brn_flag_exit = BRN_EXIT_SET;
                }

                if (nb_read == 0)// socket fermé par la borne
                {
                    printf("deconnexion de la borne d'acceuil\n");
                    brn_state = BRN_NOTCONN;

                    //exit(-1);
                    /*Si on ferme la borne d'accueil on regarde si on a toujours de client*/
                } else {
                    usr_brn_sup++;
                    nb_usr++;
                    uid++;
                    if (usr_brn_sup == MAX_USR - 1)
                        usr_brn_sup = 0;
                }
            }

            //Gestion des afficheurs
            for (i = 0; i < nb_aff; i++)
                if (FD_ISSET(sock_aff[i].sock, &rfds)) {
                    if ((nb_read = read(sock_aff[i].sock, buf, BUFSIZE)) < 0) {
                        printf("26 : erreur lors de la reception d'un message de l'afficheur\n");
                        perror("read");
                        exit(-1);
                    }
                    if (nb_read == 0) //deconexion de l'afficheur
                    {
                        printf("deconnexion de l'afficheur %s\n", inet_ntoa(sock_aff[i].addr_cmp.sin_addr));
                        remove_socket(i, sock_aff, &nb_aff);
                    }
                }

            //Gestion des guichet
            for (i = 0; i < nb_ght; i++)
                if (FD_ISSET(sock_ght[i].sock, &rfds)) {
                    bzero(buf, BUFSIZE);
                    if ((nb_read = read(sock_ght[i].sock, buf, GHT_SIZE_CONST)) >= 0)// erreur
                    {
                        //printf("recu : %s par guichet %d\n",buf,i);
                        if (nb_read == 0)// connexion fermée par le guichet
                        {
                            printf("deconnection du guichet %s\n", inet_ntoa(sock_ght[i].addr_cmp.sin_addr));
                            remove_socket(i, sock_ght, &nb_ght);
                        }
                        if (strcmp(buf, GHT_ASKCLT) == 0)//demande d'un client
                        {
                            clt = isInQueue(usr_tab, usr_brn_inf, usr_brn_sup);//on regarde si un client est en attente
                            if (clt == GEST_NOCLT && brn_flag_exit == BRN_EXIT_NO_SET)//pas de client
                            {
                                nb_write = write(sock_ght[i].sock, GHT_NOCLT, strlen(GHT_NOCLT));
                                if (nb_write != strlen(GHT_NOCLT)) {
                                    printf("30 : erreur write\n");
                                    perror("write");
                                    exit(-1);
                                }

                            } else if (clt == GEST_NOCLT &&
                                       brn_flag_exit == BRN_EXIT_SET) {//pas de client + exit reçu de la borne

                                nb_write = write(sock_ght[i].sock, GHT_EXIT, strlen(GHT_EXIT));
                                if (nb_write != strlen(GHT_EXIT)) {
                                    printf("31 : erreur writ\n");
                                    perror("write");
                                    exit(-1);
                                }
                            } else //il y a un client
                            {
                                for (j = 0; j < nb_aff; j++) // on affiche le client ainsi que son guichet attribué
                                {
                                    if ((nb_write = write(sock_aff[j].sock, AFF_ASKADD, strlen(AFF_ASKADD))) !=
                                        strlen(AFF_ASKADD)) {
                                        printf("32 : erreur lors de l'envoie du code de demande d'affichage a l'afficheur %d\n",
                                               i);
                                        perror("write");
                                        exit(-1);
                                    }
                                    if ((nb_write = write(sock_aff[j].sock, &usr_tab[clt].id, sizeof(int))) !=
                                        sizeof(int)) {
                                        printf("33 : erreur lors de l'envoie de l'id du client %d a l'afficheur %d\n",
                                               clt, i);
                                        perror("write");
                                        exit(-1);
                                    }
                                    length_name_clt = (strlen(usr_tab[clt].nom));
                                    if ((nb_write = write(sock_aff[j].sock, &length_name_clt, sizeof(int))) !=
                                        sizeof(int)) {
                                        printf("34 : erreur lors de l'envoie de la taille du nom du client %d a l'afficheur %d\n",
                                               clt, i);
                                        perror("write");
                                        exit(-1);
                                    }
                                    if ((nb_write = write(sock_aff[j].sock, usr_tab[clt].nom, length_name_clt)) !=
                                        length_name_clt) {
                                        printf("35 : erreur lors de l'envoie de la taille du nom du client %d a l'afficheur %d\n",
                                               clt, i);
                                        perror("write");
                                        exit(-1);
                                    }
                                    if ((nb_write = write(sock_aff[j].sock, sock_ght[i].num,
                                                          strlen(sock_ght[i].num))) != strlen(sock_ght[i].num)) {
                                        printf("36 : erreur lors de l'envoie de la taille du nom du client %d a l'afficheur %d\n",
                                               clt, i);
                                        perror("write");
                                        exit(-1);
                                    }
                                }

                                nb_write = write(sock_ght[i].sock, GEST_CONFCLT,
                                                 strlen(GEST_CONFCLT)); //conf on envoie client
                                usr_tab[clt].state = WTCONF;
                                if (nb_write != strlen(GEST_CONFCLT)) {
                                    printf("37 : erreur write\n");
                                    perror("write");
                                    exit(-1);
                                }

                                nb_write = write(sock_ght[i].sock, &usr_tab[clt].id, sizeof(int));
                                if (nb_write != sizeof(int)) {
                                    printf("38 : erreur lors de l'envoi des donnes clients\n");
                                    perror("write");
                                    exit(-1);
                                }
                                nb_write = write(sock_ght[i].sock, &clt, sizeof(int));
                                if (nb_write != sizeof(int)) {
                                    printf("39 : erreur lors de l'envoi de l'indice du client\n");
                                    perror("write");
                                    exit(-1);
                                }
                                nb_write = write(sock_ght[i].sock, &usr_tab[clt].nom, strlen(usr_tab[clt].nom));
                                if (nb_write != strlen(usr_tab[clt].nom)) {
                                    printf("40 : erreur lors de l'envoi de l'indice du client\n");
                                    perror("write");
                                    exit(-1);
                                }
                            }
                        } else if (strcmp(buf, GHT_CLTCONF) == 0)//"2" =>confirmation reception client
                        {
                            nb_read = read(sock_ght[i].sock, &clt, sizeof(int));
                            if (nb_read != sizeof(int)) {
                                printf("41 : erreur lors de la reception de l'indice client\n");
                                perror("read");
                                exit(-1);
                            }
                            usr_tab[clt].state = CONF;
                            bzero(usr_tab[clt].nom, BUFSIZE);
                            nb_usr--;
                            usr_brn_inf++;
                            if (usr_brn_inf == MAX_USR - 1)
                                usr_brn_inf = 0;


                            for (j = 0; j < nb_aff; j++) {
                                if ((nb_write = write(sock_aff[j].sock, AFF_ASKRET, strlen(AFF_ASKRET))) !=
                                    strlen(AFF_ASKRET)) {
                                    printf("42 : erreur lors de l envoie de la demande de suppression\n");
                                    perror("write");
                                    exit(-1);
                                }

                                nb_write = write(sock_aff[j].sock, sock_ght[i].num,
                                                 strlen(sock_ght[i].num)); // on envoie le num
                                if (nb_write != strlen(sock_ght[i].num)) {
                                    perror("43 : erreur lors de l'envoi de l'indice guichet\n");
                                    exit(-1);
                                }
                            }
                        }
                    } else {
                        printf("44 : erreur lors de la reception d'un message venant du guicher %d\n", i);
                        perror("read");
                        exit(-1);
                    }

                }

        }


    }
}