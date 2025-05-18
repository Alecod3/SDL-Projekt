#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <windows.h>
#include "game.h"
#include "menu.h"
#include "network.h"
#include "sound.h"

bool skipMenu = false;
SDL_Texture *tex_player = NULL;
SDL_Texture *tex_mob = NULL;
SDL_Texture *tex_tiles = NULL;
SDL_Texture *tex_extralife = NULL;
SDL_Texture *tex_extraspeed = NULL;
SDL_Texture *tex_doubledamage = NULL;
SDL_Texture *tex_freezeenemies = NULL;

int main(int argc, char *argv[])
{
    // Initialize SDL2, SDL_image, and SDL_ttf
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        SDL_Log("Failed to initialize SDL_image: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() == -1)
    {
        SDL_Log("Failed to initialize SDL_ttf: %s", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create window and renderer
    SDL_Window *window = SDL_CreateWindow("Zombie Game",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load font
    TTF_Font *uiFont = TTF_OpenFont("shlop.ttf", 28);
    if (!uiFont)
    {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Initialize sound
    SoundSystem *audio = SoundSystem_create();
    if (!audio)
    {
        SDL_Log("Failed to initialize sound system");
        TTF_CloseFont(uiFont);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Run the game
    int result = run_game(renderer, window, uiFont, audio);

    // Clean up (skipped if restart_game in game.c exits the process)
    SDL_DestroyTexture(tex_extralife);
    SDL_DestroyTexture(tex_extraspeed);
    SDL_DestroyTexture(tex_doubledamage);
    SDL_DestroyTexture(tex_freezeenemies);
    SDL_DestroyTexture(tex_player);
    SDL_DestroyTexture(tex_mob);
    SDL_DestroyTexture(tex_tiles);
    SoundSystem_destroy(audio);
    TTF_CloseFont(uiFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return result;
}