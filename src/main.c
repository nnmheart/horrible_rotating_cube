#include<stdio.h>
#include<stdbool.h>
#include<assert.h>
#include<malloc.h>
#include<math.h>
#include<SDL2/SDL.h>

#include<app.h>

app_t* app;

const double RAD_TO_DEG = 180 / 3.1415;

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

v3 v_add(v3 a, v3 b) {
    return (v3){
        .x = a.x + b.x,
        .y = a.y + b.y,
        .z = a.z + b.z
    };
}

v3 v_min(v3 a, v3 b) {
    return (v3){
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z
    };
}

// Rotations
// NOTE: This is all unoptimized, but I don't care, I simply want to code.
// Consider precomputing sines and cosines beforehand then multiplying respective coordinations.
// https://en.wikipedia.org/wiki/Rotation_matrix
v3 v_rotate_x(v3 a, double angle) {
    // [1 0          0          ][x]   [x]
    // [0 cos(theta) -sin(theta)][y] = [y * cos(theta) - z * sin(theta)]
    // [0 sin(theta) cos(theta) ][z] = [y * sin(theta) + z * cos(theta)]

    double s = sin(angle);
    double c = cos(angle);

    return (v3){
        .x = a.x,
        .y = a.y * c - a.z * s,
        .z = a.y * s + a.z * c
    };
}

v3 v_rotate_y(v3 a, double angle) {
    // [cos(theta) 0 -sin(theta)][x]   [x * cos(theta) - z * sin(theta)]
    // [0          1 0          ][y] = [y]
    // [sin(theta) 0 cos(theta) ][z]   [x * sin(theta) + z * cos(theta)]

    double s = sin(angle);
    double c = cos(angle);

    return (v3){
        .x = a.x * c - a.z * s,
        .y = a.y,
        .z = a.x * s + a.z * c
    };
}

v3 v_rotate_z(v3 a, double angle) {
    // [cos(theta) -sin(theta) 0][x]   [x * cos(theta) - y * sin(theta)]
    // [sin(theta) cos(theta)  0][y] = [x * sin(theta) + y * cos(theta)]
    // [0          0           1][z]   [z]

    double s = sin(angle);
    double c = cos(angle);

    return (v3){
        .x = a.x * c - a.y * s,
        .y = a.x * s + a.y * c,
        .z = a.z
    };
}

v3 v_rotate(v3 a, double rot_x, double rot_y, double rot_z) {
    return v_rotate_z(
        v_rotate_y(
            v_rotate_x(a, rot_x),
            rot_y
        ),
        rot_z
    );
}

typedef struct cube {
    v3 ftl;
    v3 ftr;
    v3 fbl;
    v3 fbr;

    v3 btl;
    v3 btr;
    v3 bbl;
    v3 bbr;

    v3 center;
    double x_rot;
    double y_rot;
    double z_rot;

    bool auto_rot;
    double auto_x_rot;
    double auto_y_rot;
    double auto_z_rot;
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

char* editing_texts[6] = {
    "Editing FOV.",
    "Editing rotation X",
    "Editing rotation Y",
    "Editing rotation Z",
    "Editing selected cube",
    "Toggle autorotation"
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
    sprintf(to_render, "Current cube: %i\n", app->current_cube);
    ri_text();
    sprintf(to_render, "%s (+/-)", editing_texts[app->em]);
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
        sprintf(to_render, "   - rx: %i", (int)(cub.x_rot * RAD_TO_DEG));
        ri_text();
        sprintf(to_render, "   - ry: %i", (int)(cub.y_rot * RAD_TO_DEG));
        ri_text();
        sprintf(to_render, "   - rz: %i", (int)(cub.z_rot * RAD_TO_DEG));
        ri_text();
        
    }
}

void render_cube(cube cub) {
    // TODO: Depending on how cube is aligned, we shouldn't be using z.
    // Example: rotating the cube by Y axis moves the Z axis away
    // Hence rendering something else.


    #define do_things(v) v_add(v_rotate(v_min(v, cub.center), cub.x_rot, cub.y_rot, cub.z_rot), cub.center);

    v3 ftl = do_things(cub.ftl);
    v3 ftr = do_things(cub.ftr);
    v3 fbl = do_things(cub.fbl);
    v3 fbr = do_things(cub.fbr);
    v3 btl = do_things(cub.btl);
    v3 btr = do_things(cub.btr);
    v3 bbl = do_things(cub.bbl);
    v3 bbr = do_things(cub.bbr);

    // "Front" cube
    SDL_SetRenderDrawColor(app->renderer, 255, 0, 0, 255);
    connect_lines(ftl, ftr); // Top horizontal
    connect_lines(ftr, fbr); // Right vertical
    connect_lines(fbr, fbl); // Bottom horizontal
    connect_lines(fbl, ftl); // Left vertical

    // "Back" cube
    SDL_SetRenderDrawColor(app->renderer, 0, 255, 0, 255);
    connect_lines(btl, btr); // Top horizontal
    connect_lines(btr, bbr); // Right vertical
    connect_lines(bbr, bbl); // Bottom horizontal
    connect_lines(bbl, btl); // Left vertical       

    // Connections between both cubes
    SDL_SetRenderDrawColor(app->renderer, 0, 0, 255, 255);
    connect_lines(ftl, btl); // Top left
    connect_lines(ftr, btr); // Top left
    connect_lines(fbl, bbl); // Top left
    connect_lines(fbr, bbr); // Top left
}

void game_render() {
    SDL_SetRenderDrawColor(app->renderer, 255, 200, 200, 255);
    SDL_RenderClear(app->renderer);

    SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
    for (int i = 0; i < app->cube_count; i++) {
        cube cub = cubes[i];
        render_cube(cub);        
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

    cub->center = (v3){
        .x = x + (width / 2),
        .y = y + (height / 2),
        .z = z + (depth / 2)
    };

    cub->auto_rot = false;
    cub->x_rot = cub->auto_x_rot = 0;
    cub->y_rot = cub->auto_y_rot = 0;
    cub->z_rot = cub->auto_z_rot = 0;
}


void handle_keypress(SDL_Event event) {
    bool pressed = (event.type == SDL_KEYDOWN) ? true : false;
    if (!pressed) return;

    switch (event.key.keysym.sym) {
        case SDLK_f:
            app->em++;
            if (app->em > EM_AUTOROT) {
                app->em = EM_FOV;
            }
            break;

        case SDLK_PLUS:
        case SDLK_MINUS:
        case SDLK_KP_PLUS:
        case SDLK_KP_MINUS:
            bool adding = (event.key.keysym.sym == SDLK_PLUS) || (event.key.keysym.sym == SDLK_KP_PLUS);

            switch (app->em) {
                case EM_FOV:
                    app->fov += (adding ? 0.5 : -0.5);
                    break;
                case EM_ROTX:
                    (&cubes[app->current_cube])->x_rot += (adding ? 0.01 : -0.01);
                    break;
                case EM_ROTY:
                    (&cubes[app->current_cube])->y_rot += (adding ? 0.01 : -0.01);
                    break;
                case EM_ROTZ:
                    (&cubes[app->current_cube])->z_rot += (adding ? 0.01 : -0.01);
                    break;
                case EM_CUBE:
                    app->current_cube += (adding ? 1 : -1);
                    if (app->current_cube < 0) {
                        app->current_cube = app->cube_count - 1;
                    }
                    if (app->current_cube >= app->cube_count) {
                        app->current_cube = 0;
                    }
                case EM_AUTOROT:
                    cube* cub = &cubes[app->current_cube];
                    cub->auto_rot = !cub->auto_rot;
                    if (cub->auto_rot) {
                        cub->auto_x_rot = cub->x_rot;
                        cub->auto_y_rot = cub->y_rot;
                        cub->auto_z_rot = cub->z_rot;
                    }
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
    for (int i = 0; i < app->cube_count; i++) {
        cube* cub = &cubes[i];
        if (cub->auto_rot) {
            cub->x_rot += cub->auto_x_rot;
            cub->y_rot += cub->auto_y_rot;
            cub->z_rot += cub->auto_z_rot;

            while (cub->x_rot > 6.28) cub->x_rot -= 6.28;
            while (cub->y_rot > 6.28) cub->y_rot -= 6.28;
            while (cub->z_rot > 6.28) cub->z_rot -= 6.28;
        }
    }
}

int main(int argc, char** argv) {
    print("Starting!\n");

    app = malloc(sizeof(app_t));

    app->running = true;
    app->screen_width = 800;
    app->screen_height = 600;

    app->fov = 120.0;
    app->cube_count = sizeof(cubes) / sizeof(cube);
    app->current_cube = 0;
    app->em = EM_FOV;

    create_cube(
        0,
        150.0, 200.0, 0.0,
        100.0, 100.0, 50.0
    );

    create_cube(
        1,
        350.0, 200.0, 0.0,
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