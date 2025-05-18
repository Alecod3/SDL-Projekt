#ifndef MENU_H
#define MENU_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "network.h"

NetMode show_multiplayer_menu(SDL_Renderer *renderer, TTF_Font *font);
char *prompt_for_ip(SDL_Renderer *renderer, TTF_Font *font);
void wait_for_client(SDL_Renderer *renderer, TTF_Font *font);
int showMenu(SDL_Renderer *renderer, SDL_Window *window);
int showSettings(SDL_Renderer *renderer, SDL_Window *window);
int showGameOver(SDL_Renderer *renderer);

#endif