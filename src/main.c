#include<stdio.h>
#include<stdbool.h>
#include<assert.h>
#include<malloc.h>
#include<SDL2/SDL.h>

#include<app.h>

app_t* app;

void handle_keypress(SDL_Event event) {
    bool pressed = (event.type == SDL_KEYDOWN) ? true : false;
    if (!pressed) return;

    switch (event.key.keysym.sym) {
        case SDLK_PLUS:
        case SDLK_MINUS:
        case SDLK_KP_PLUS:
        case SDLK_KP_MINUS:
            bool adding = (event.key.keysym.sym == SDLK_PLUS) || (event.key.keysym.sym == SDLK_KP_PLUS);

            switch (app->em) {
                case EM_FOV:
                    app->fov += (adding ? 0.5 : -0.5);
                    break;
                default:
                    break;
            }

            break;

        default:
            break;
    }
}

void game_handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: 
                print("Received SDL_QUIT signal.\n");
                app->running = false;
                break;
            
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                handle_keypress(event);
                break;

            default:
                break;
        }
    }
}
void game_update(double dt) {
}

void render_text(char* text, SDL_Color fg, SDL_Color bg, int x, int y, int w, int h) {
    SDL_Surface* surf = TTF_RenderText(
        app->font,
        text,
        fg,
        bg
    );
    SDL_Texture* tex = SDL_CreateTextureFromSurface(app->renderer, surf);
    SDL_RenderCopy(
        app->renderer, 
        tex, 
        NULL, 
        &(SDL_Rect){
            .x = x,
            .y = y,
            .w = w,
            .h = h
        }
    );
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

typedef struct v3 {
    double x;
    double y;
    double z;
} v3;

typedef struct cube {
    v3 ftl;
    v3 ftr;
    v3 fbl;
    v3 fbr;

    v3 btl;
    v3 btr;
    v3 bbl;
    v3 bbr;
} cube;

cube cubes[2];

void connect_lines(v3 a, v3 b) {
    double fov = app->fov;

    double apx = a.x * fov / (fov + a.z);
    double apy = a.y * fov / (fov + a.z);
    
    double bpx = b.x * fov / (fov + b.z);
    double bpy = b.y * fov / (fov + b.z);

    int x1 = (int)apx;
    int y1 = (int)apy;
    int x2 = (int)bpx;
    int y2 = (int)bpy;

    SDL_RenderDrawLine(
        app->renderer,
        x1,
        y1,
        x2,
        y2
    );
}

char* editing_texts[2] = {
    "Editing FOV.",
    "Editing rotation angle."
};

void render_infos() {
    char to_render[100];
    SDL_Color pink = (SDL_Color){.r = 255, .g = 200, .b = 200, .a = 255};
    SDL_Color black = (SDL_Color){.r = 0, .g = 0, .b = 0, .a = 255};
    int text_row = 0;

    #define ri_text() \
        render_text( \
            to_render, \
            black, pink, \
            app->screen_width - SDL_strlen(to_render) * 10, (text_row++) * 30, \
            SDL_strlen(to_render) * 10, 30)

    sprintf(to_render, "FOV: %i", (int)app->fov);
    ri_text();
    
    for (int i = 0; i < app->cube_count; i++) {
        sprintf(to_render, "Cube %i           ", i);
        ri_text();

        cube cub = cubes[i];

        sprintf(to_render, "  - x: %i w: %i", (int)cub.ftl.x, (int)(cub.ftr.x - cub.ftl.x));
        ri_text();
        sprintf(to_render, "  - y: %i h: %i", (int)cub.ftl.y, (int)(cub.ftl.y - cub.btl.y));
        ri_text();
        sprintf(to_render, "  - z: %i d: %i", (int)cub.ftl.z, (int)(cub.btl.z - cub.ftl.z));
        ri_text();
    }
    sprintf(to_render, "%s (+/-)", editing_texts[app->em]);
    ri_text();
}

void game_render() {
    SDL_SetRenderDrawColor(app->renderer, 255, 200, 200, 255);
    SDL_RenderClear(app->renderer);

    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    for (int i = 0; i < app->cube_count; i++) {
        cube cub = cubes[i];

        // "Front" cube
        SDL_SetRenderDrawColor(app->renderer, 255, 0, 0, 255);
        connect_lines(cub.ftl, cub.ftr); // Top horizontal
        connect_lines(cub.ftr, cub.fbr); // Right vertical
        connect_lines(cub.fbr, cub.fbl); // Bottom horizontal
        connect_lines(cub.fbl, cub.ftl); // Left vertical

        // "Back" cube
        SDL_SetRenderDrawColor(app->renderer, 0, 255, 0, 255);
        connect_lines(cub.btl, cub.btr); // Top horizontal
        connect_lines(cub.btr, cub.bbr); // Right vertical
        connect_lines(cub.bbr, cub.bbl); // Bottom horizontal
        connect_lines(cub.bbl, cub.btl); // Left vertical       

        // Connections between both cubes
        SDL_SetRenderDrawColor(app->renderer, 0, 0, 255, 255);
        connect_lines(cub.ftl, cub.btl); // Top left
        connect_lines(cub.ftr, cub.btr); // Top left
        connect_lines(cub.fbl, cub.bbl); // Top left
        connect_lines(cub.fbr, cub.bbr); // Top left
    }

    render_infos();

    SDL_RenderPresent(app->renderer);
}

void create_cube(
    int i, 
    double x, double y, double z, 
    double width, double height, double depth
) {
    cube* cub = &cubes[i];
    // Front
    cub->ftl = (v3){.x = x        , .y = y         , .z = z        };
    cub->ftr = (v3){.x = x + width, .y = y         , .z = z        };
    cub->fbl = (v3){.x = x        , .y = y + height, .z = z        };
    cub->fbr = (v3){.x = x + width, .y = y + height, .z = z        };
    // Back
    cub->btl = (v3){.x = x        , .y = y         , .z = z + depth};
    cub->btr = (v3){.x = x + width, .y = y         , .z = z + depth};
    cub->bbl = (v3){.x = x        , .y = y + height, .z = z + depth};
    cub->bbr = (v3){.x = x + width, .y = y + height, .z = z + depth};
}

int main(int argc, char** argv) {
    print("Starting!\n");

    app = malloc(sizeof(app_t));

    app->running = true;
    app->screen_width = 800;
    app->screen_height = 600;

    app->fov = 120.0;
    app->cube_count = sizeof(cubes) / sizeof(cube);
    app->em = EM_FOV;

    create_cube(
        0,
        300.0, 200.0, 0.0,
        100.0, 100.0, 50.0
    );

    create_cube(
        1,
        500.0, 200.0, 0.0,
        100.0, 100.0, 50.0
    );

    print("Initialized cubes.\n");

    assert(SDL_Init(SDL_INIT_EVERYTHING) == 0);
    app->window = SDL_CreateWindow(
        "Rotating Cube", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        app->screen_width, app->screen_height, 
        0
    );
    assert(app->window != NULL);
    app->renderer = SDL_CreateRenderer(app->window, -1, 0);
    assert(app->renderer != NULL);
    assert(SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND) == 0);
    print("Initialized application window and renderer.\n");
    assert(TTF_Init() == 0);
    app->font = TTF_OpenFont("./OpenSans-Regular.ttf", 24);
    assert(app->font != NULL);


    double last_tick = (double)SDL_GetTicks();
    double current_tick = (double)SDL_GetTicks();
    double delta_accum = 0.0;
    double frametime = 1.0 / 50.0;

    print("Entering the mainloop.\n");
    while (app->running) {
        current_tick = (double)SDL_GetTicks();
        delta_accum += (current_tick - last_tick) / 1000.0;
        last_tick = current_tick;
        
        game_handle_events();
        while (delta_accum >= frametime) {
            game_update(frametime);
            delta_accum -= frametime;
        }
        game_render();
    }

    return 0;
}