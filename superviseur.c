//
// Created by malo on 08/03/18.
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


#include "const.h"

int main(int argc, char **argv)
{
    int sock;
    char buf[BUFSIZE];

    struct hostent *host = NULL;
    struct sockaddr_in gest_addr;
    int gest_addr_size = sizeof(gest_addr);

    if(argc != 3)
    {
        printf("usage : %s port udp adresse\n",argv[0]);
        exit(-1);
    }

    sock = socket(AF_INET,SOCK_DGRAM,0);
    if(sock == -1)
    {
        perror("socket");
        exit(-1);
    }

    host = gethostbyname(argv[2]);
    if(host == NULL)
    {
        printf("host inconnu %s",argv[2]);
        perror("gethostbyname");
        exit(-1);
    }

    gest_addr.sin_family = AF_INET;
    gest_addr.sin_port = htons((uint16_t)atoi(argv[1]));
    bcopy(host->h_addr,&gest_addr.sin_addr,(unsigned int)host->h_length);
    while(1)
    {
        do
        {
            printf("Votre message aux guichets :\n");
            scanf("%s", buf);
        } while (strlen(buf) == 0);

        if(sendto(sock,buf,strlen(buf),0,(struct sockaddr*)&gest_addr,gest_addr_size) != strlen(buf))
        {
            printf("erreur lors de l'envoi du message %s\n",buf);
            perror("sendto");
            exit(-1);
        }
        bzero(buf,BUFSIZE);
    }


}