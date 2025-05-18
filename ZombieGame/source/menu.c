#include "menu.h"
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1200

static void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *msg, int x, int y)
{
    SDL_Color color = {255, 255, 255, 255}; // White text
    SDL_Surface *surf = TTF_RenderText_Blended(font, msg, color);
    if (!surf)
    {
        SDL_Log("TTF_RenderText_Blended failed: %s", TTF_GetError());
        return;
    }
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_FreeSurface(surf);
    if (!tex)
    {
        SDL_Log("SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
        return;
    }
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}

NetMode show_multiplayer_menu(SDL_Renderer *renderer, TTF_Font *font)
{
    SDL_Event event;
    bool inMenu = true;
    int selected = 0;
    SDL_Rect buttons[2];
    const char *labels[2] = {"Host Game", "Join Game"};
    TTF_Font *optionFont = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font *titleFont = TTF_OpenFont("simbiot.ttf", 64);
    if (!optionFont || !titleFont)
    {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        if (optionFont)
            TTF_CloseFont(optionFont);
        if (titleFont)
            TTF_CloseFont(titleFont);
        return MODE_HOST; // Default to host on error
    }
    for (int i = 0; i < 2; i++)
    {
        buttons[i].x = SCREEN_WIDTH / 2 - 100;
        buttons[i].y = 250 + i * 80;
        buttons[i].w = 200;
        buttons[i].h = 50;
    }
    while (inMenu)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                TTF_CloseFont(optionFont);
                TTF_CloseFont(titleFont);
                exit(0);
            }
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_UP)
                    selected = (selected + 1) % 2;
                if (event.key.keysym.sym == SDLK_DOWN)
                    selected = (selected + 1) % 2;
                if (event.key.keysym.sym == SDLK_RETURN)
                {
                    TTF_CloseFont(optionFont);
                    TTF_CloseFont(titleFont);
                    return (selected == 0) ? MODE_HOST : MODE_JOIN;
                }
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                int mx = event.motion.x, my = event.motion.y;
                for (int i = 0; i < 2; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                        selected = i;
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mx = event.button.x, my = event.button.y;
                for (int i = 0; i < 2; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                    {
                        TTF_CloseFont(optionFont);
                        TTF_CloseFont(titleFont);
                        return (i == 0) ? MODE_HOST : MODE_JOIN;
                    }
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (titleFont)
        {
            SDL_Color red = {200, 0, 0};
            SDL_Surface *titleSurface = TTF_RenderText_Blended(titleFont, "Multiplayer", red);
            SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
            SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 60, titleSurface->w, titleSurface->h};
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
            SDL_FreeSurface(titleSurface);
            SDL_DestroyTexture(titleTexture);
        }
        for (int i = 0; i < 2; i++)
        {
            SDL_SetRenderDrawColor(renderer, (i == selected) ? 180 : 90, 0, 0, 255);
            SDL_RenderFillRect(renderer, &buttons[i]);
            SDL_Color color = {255, 255, 255};
            SDL_Surface *surface = TTF_RenderText_Blended(optionFont, labels[i], color);
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {
                buttons[i].x + (buttons[i].w - surface->w) / 2,
                buttons[i].y + (buttons[i].h - surface->h) / 2,
                surface->w,
                surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    TTF_CloseFont(optionFont);
    TTF_CloseFont(titleFont);
    return MODE_HOST; // Fallback
}

char *prompt_for_ip(SDL_Renderer *renderer, TTF_Font *font)
{
    SDL_StartTextInput();
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    char *ip = calloc(256, 1);
    size_t len = 0;
    SDL_Event e;
    SDL_Rect inputRect = {100, 150, 300, 32};
    while (1)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                exit(0);
            if (e.type == SDL_TEXTINPUT)
            {
                if (len + strlen(e.text.text) < 255)
                {
                    strcat(ip, e.text.text);
                    len = strlen(ip);
                }
            }
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_BACKSPACE && len > 0)
                {
                    ip[--len] = '\0';
                }
                else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
                {
                    SDL_StopTextInput();
                    return ip;
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
        render_text(renderer, font, "Enter server IP:", 100, 110);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &inputRect);
        render_text(renderer, font, ip, inputRect.x + 5, inputRect.y + 5);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

void wait_for_client(SDL_Renderer *renderer, TTF_Font *font)
{
    SDL_Event e;
    int x, y;
    while (1)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                exit(0);
        }
        if (network_receive(&x, &y))
            break;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        render_text(renderer, font, "Waiting for client...", 100, 100);
        SDL_RenderPresent(renderer);
        SDL_Delay(100);
    }
}

int showMenu(SDL_Renderer *renderer, SDL_Window *window)
{
    SDL_Event event;
    bool inMenu = true;
    int selected = 0;
    SDL_Rect buttons[2];
    const char *labels[2] = {"Multiplayer", "Exit Game"};
    TTF_Font *font = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font *titleFont = TTF_OpenFont("simbiot.ttf", 64);
    for (int i = 0; i < 2; i++)
    {
        buttons[i].x = SCREEN_WIDTH / 2 - 100;
        buttons[i].y = 250 + i * 80;
        buttons[i].w = 200;
        buttons[i].h = 50;
    }
    while (inMenu)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return -1;
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_UP)
                    selected = (selected + 1) % 2;
                if (event.key.keysym.sym == SDLK_DOWN)
                    selected = (selected + 1) % 2;
                if (event.key.keysym.sym == SDLK_RETURN)
                {
                    TTF_CloseFont(font);
                    TTF_CloseFont(titleFont);
                    return (selected == 0) ? 1 : -1;
                }
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                int mx = event.motion.x, my = event.motion.y;
                for (int i = 0; i < 2; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                        selected = i;
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mx = event.button.x, my = event.button.y;
                for (int i = 0; i < 2; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                    {
                        TTF_CloseFont(font);
                        TTF_CloseFont(titleFont);
                        return (i == 0) ? 1 : -1;
                    }
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (titleFont)
        {
            SDL_Color red = {200, 0, 0};
            SDL_Surface *titleSurface = TTF_RenderText_Blended(titleFont, "Zombie Game", red);
            SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
            SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 60, titleSurface->w, titleSurface->h};
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
            SDL_FreeSurface(titleSurface);
            SDL_DestroyTexture(titleTexture);
        }
        for (int i = 0; i < 2; i++)
        {
            SDL_SetRenderDrawColor(renderer, (i == selected) ? 180 : 90, 0, 0, 255);
            SDL_RenderFillRect(renderer, &buttons[i]);
            SDL_Color color = {255, 255, 255};
            SDL_Surface *surface = TTF_RenderText_Blended(font, labels[i], color);
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {
                buttons[i].x + (buttons[i].w - surface->w) / 2,
                buttons[i].y + (buttons[i].h - surface->h) / 2,
                surface->w,
                surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    TTF_CloseFont(font);
    TTF_CloseFont(titleFont);
    return -1;
}

int showSettings(SDL_Renderer *renderer, SDL_Window *window)
{
    // Empty, as Settings option is removed
    return 0;
}

int showGameOver(SDL_Renderer *renderer)
{
    SDL_Event event;
    bool inGameOver = true;
    int selected = 0;
    SDL_Rect buttons[2];
    const char *labels[2] = {"Play Again", "Exit Game"};
    TTF_Font *optionFont = TTF_OpenFont("shlop.ttf", 28);
    TTF_Font *titleFont = TTF_OpenFont("simbiot.ttf", 64);
    for (int i = 0; i < 2; i++)
    {
        buttons[i].x = SCREEN_WIDTH / 2 - 100;
        buttons[i].y = 250 + i * 80;
        buttons[i].w = 200;
        buttons[i].h = 50;
    }
    while (inGameOver)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return -1;
            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_UP)
                    selected = (selected + 1) % 2;
                if (event.key.keysym.sym == SDLK_DOWN)
                    selected = (selected + 1) % 2;
                if (event.key.keysym.sym == SDLK_RETURN)
                {
                    TTF_CloseFont(optionFont);
                    TTF_CloseFont(titleFont);
                    return (selected == 0) ? 0 : -1;
                }
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                int mx = event.motion.x, my = event.motion.y;
                for (int i = 0; i < 2; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                        selected = i;
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mx = event.button.x, my = event.button.y;
                for (int i = 0; i < 2; i++)
                {
                    if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].w &&
                        my >= buttons[i].y && my <= buttons[i].y + buttons[i].h)
                    {
                        TTF_CloseFont(optionFont);
                        TTF_CloseFont(titleFont);
                        return (i == 0) ? 0 : -1;
                    }
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (titleFont)
        {
            SDL_Color red = {200, 0, 0};
            SDL_Surface *titleSurface = TTF_RenderText_Blended(titleFont, "GAME OVER", red);
            SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
            SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 60, titleSurface->w, titleSurface->h};
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
            SDL_FreeSurface(titleSurface);
            SDL_DestroyTexture(titleTexture);
        }
        for (int i = 0; i < 2; i++)
        {
            SDL_SetRenderDrawColor(renderer, (i == selected) ? 180 : 90, 0, 0, 255);
            SDL_RenderFillRect(renderer, &buttons[i]);
            SDL_Color color = {255, 255, 255};
            SDL_Surface *surface = TTF_RenderText_Blended(optionFont, labels[i], color);
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {
                buttons[i].x + (buttons[i].w - surface->w) / 2,
                buttons[i].y + (buttons[i].h - surface->h) / 2,
                surface->w,
                surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    TTF_CloseFont(optionFont);
    TTF_CloseFont(titleFont);
    return -1;
}