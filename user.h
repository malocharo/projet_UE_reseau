//
// Created by malo on 07/02/18.
//

#ifndef PROJET_RESEAU_USER_H
#define PROJET_RESEAU_USER_H

enum State{WTCONF, CONF};

struct user{
    char nom[256];
    int id;
    enum State state;
};
#endif //PROJET_RESEAU_USER_H
