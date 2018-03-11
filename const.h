//
// Created by malo on 20/02/2018.
//

#ifndef PROJET_UE_RESEAU_CONST_H
#define PROJET_UE_RESEAU_CONST_H

#define BUFSIZE 256
#define BUFSIZE_MIN 127
#define NB_MAX_PENDINGQUEUE 10

#define GHT_IDENTIFIER "ght"
#define GHT_SIZE_IDENTIFIER 3
#define GHT_ASKCLT "1"
#define GHT_CLTCONF "2"
#define GHT_NOCLT "3"
#define GHT_EXIT "5"
#define GHT_SIZE_CONST 1

#define AFF_IDENTIFIER "aff"
#define AFF_ASKRET "1"
#define AFF_ASKADD "0"

#define BRN_IDENTIFIER "brn"
#define BRN_EXIT "exit"
#define BRN_EXIT_NO_SET 0
#define BRN_EXIT_SET 1
#define BRN_CONN 1
#define BRN_NOTCONN 0

#define SUP_IDENTIFIER "sup"

#define MAX_AFF 10
#define MAX_GHT 10
#define MAX_USR 100
#define GEST_CONFCLT "4"
#define GEST_NOCLT -1
#define GEST_CMD_SIZE 1


#endif //PROJET_UE_RESEAU_CONST_H
