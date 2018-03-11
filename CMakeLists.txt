cmake_minimum_required(VERSION 3.9)
project(projet_reseau C)

set(CMAKE_C_STANDARD 99)

add_executable(gestion_salle user.h gestion_salle.c const.h)
add_executable(guichet user.h guichet.c const.h)
add_executable(affichage user.h affichage.c const.h)
add_executable(borne user.h borne.c const.h)
add_executable(superviseur superviseur.c)