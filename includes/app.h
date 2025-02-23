#ifndef _APP_H
#define _APP_H

#include<stdio.h>
#include<stdbool.h>
#include<SDL2/SDL.h>
#include<SDL2/SDL_ttf.h>

#define print(...) printf("[%s]: ", __func__); printf(__VA_ARGS__);

enum EditingMode {
    EM_FOV = 0
};

typedef struct app_t {
    bool running;
    int screen_width;
    int screen_height;
    SDL_Window* window;
    SDL_Renderer* renderer;

    TTF_Font* font;
    double fov;
    int cube_count;
    enum EditingMode em;
} app_t;

#endif // _APP_H