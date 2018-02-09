cmake_minimum_required(VERSION 3.9)
project(projet_reseau C)

set(CMAKE_C_STANDARD 99)

add_executable(projet_reseau guichet.c borne.c user.h affichage.c gestion_salle.c)