/* my 1st proto-game in C yay! C99 btw
 *
 *      Danilo Masic apr-sep 2021, oct 2024
 *      depends: libsdl2
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#define HEIGHT      720
#define WIDTH       960
#define VEL         0.33f   // w-s step
#define A_VEL       0.2618f // a-d step (in rad)
#define SCR_W       0.5f    // virtual screen width in the map
#define SCR_D       0.5f    // ....... ...... height.. ... ...
#define MAP_X_SIZE  10      // map width  (for the x's)
#define MAP_Y_SIZE  10      // map height (for the y's)
#define GREY_SC_LEN 0xff    // for greyscale quantization
#define CYLIS_NUM   12

#define RAND_FL(min, max)   (min + (rand() / (float) RAND_MAX) * (max - min))

/* global vars */
static uint8_t map[MAP_Y_SIZE][MAP_X_SIZE] =
{
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 1, 0, 0, 0, 1, 1, 0, 1},
  {1, 0, 1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 1, 0, 0, 0, 1, 0, 0, 1},
  {1, 0, 1, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 1, 0, 0, 0, 0, 1, 0, 1},
  {1, 0, 1, 1, 0, 0, 1, 1, 0, 1},
  {1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
  {1, 1, 1, 1, 0, 0, 1, 1, 1, 1}
};

Uint32 *pix_img = NULL;
float cylis[CYLIS_NUM][2];


void raycast(float, float, const float *);
bool edge_detected(float *, float, float);
bool cyli_detected(float, float);
void update_cylis(void);
Uint32 render_shade(float);


int main(void)
{
    bool close = false;
    int music_chn;
    float x_pos = 4.5, y_pos = 11, angle = -3.141592 / 2;
    float u_vect[2]; /* unit vector, changed by angle */
    u_vect[0] = cos(angle);
    u_vect[1] = sin(angle);
    SDL_Event event;

    /* seed random */
    srand((unsigned) time(NULL));

    /* create cylinders outside blocks */
    for (int i = 0; i < CYLIS_NUM; ++i) {
        do {
            cylis[i][0] = RAND_FL(0, MAP_X_SIZE);
            cylis[i][1] = RAND_FL(0, MAP_Y_SIZE);
        } while (map[(int) cylis[i][0]][(int) cylis[i][1]] == 1);
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "error initializing SDL: %s\n", SDL_GetError());
        exit(1);
    }

                    /* rate, format (CTN), chn, buff-size */
    if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 2<<10) != 0) {
        fprintf(stderr, "error opening audio mixer: %s\n", Mix_GetError());
        exit(1);
    }

    Mix_Chunk *music = NULL;
    music = Mix_LoadWAV("./sounds/music.wav");
    if (music == NULL) {
        fprintf(stderr, "Error: can't load WAV file: ./sounds/music.wav\n");
        fprintf(stderr, "%s\n", Mix_GetError());
        exit(1);
    }

    Mix_Chunk *move_sound = NULL;
    move_sound = Mix_LoadWAV("./sounds/move.wav");
    if (move_sound == NULL) {
        fprintf(stderr, "Error: can't load WAV file: ./sounds/move.wav\n");
        fprintf(stderr, "%s\n", Mix_GetError());
        exit(1);
    }

    music_chn = Mix_PlayChannel(-1, music, -1);
    if (music_chn == -1) {
        printf("Unable to play WAV file: %s\n", Mix_GetError());
    }

    SDL_Window *win = SDL_CreateWindow("GAME",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       WIDTH, HEIGHT, 0);

    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, 0);
    /* r_flags instead of 0 */

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);

    pix_img = (Uint32 *) calloc(HEIGHT * WIDTH, sizeof(Uint32));
    if (pix_img == NULL) {
        fprintf(stderr, "could not allocate\n");
        exit(1);
    }

    raycast(x_pos, y_pos, u_vect); /* initial screen */

    while (!close) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    close = true;
                    break;
                case SDL_KEYDOWN:
                    Mix_PlayChannel(-1, move_sound, 0);
                    switch (event.key.keysym.sym) {
                        case SDLK_w:
                            x_pos += VEL * u_vect[0];
                            y_pos += VEL * u_vect[1];
                            //raycast(x_pos, y_pos, u_vect);
                            break;
                        case SDLK_s:
                            x_pos -= VEL * u_vect[0];
                            y_pos -= VEL * u_vect[1];
                            //raycast(x_pos, y_pos, u_vect);
                            break;
                        case SDLK_a:
                            angle += A_VEL;
                            u_vect[0] = cos(angle);
                            u_vect[1] = sin(angle);
                            //raycast(x_pos, y_pos, u_vect);
                            break;
                        case SDLK_d:
                            angle -= A_VEL;
                            u_vect[0] = cos(angle);
                            u_vect[1] = sin(angle);
                            //raycast(x_pos, y_pos, u_vect);
                            break;
                        case SDLK_q:
                            close = true;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
        SDL_UpdateTexture(texture, NULL, pix_img, WIDTH * sizeof(Uint32));

        update_cylis();
        raycast(x_pos, y_pos, u_vect);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(100);
    }

    Mix_HaltChannel(-1); /* stop all channels */
    Mix_FreeChunk(music);
    Mix_FreeChunk(move_sound);
    Mix_CloseAudio();

    free(pix_img);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

void raycast(float x_pos, float y_pos, const float *u_vect)
{
    float render_vector[2], scaled_r_v[2];
    /* the vector from the viewer to the pixel on the virtual screen */

    for (int pix=0; pix<WIDTH; ++pix) {
        float lambda = 1;   /* for scaling render_vector */
        /* first we set the render vector with on the virtual screen,
           it is perpendicular to the u_vect,
           and the dummy scales it to the size of the screen,
           with the center of the screen onto the viewer */
        float dummy = SCR_W * (0.5f - pix / (float) WIDTH);
        render_vector[0] = - u_vect[1] * dummy;
        render_vector[1] =   u_vect[0] * dummy;
        /* now we translate it to the SCR_D distance,
           in the direction of u_vect */
        render_vector[0] += u_vect[0] * SCR_D;
        render_vector[1] += u_vect[1] * SCR_D;
        /* now we have the vector that goes from the viewer
           to the pixel of the screen on the map */

        while (lambda < 2 * MAP_X_SIZE / SCR_D) {
            scaled_r_v[0] = lambda * render_vector[0];
            scaled_r_v[1] = lambda * render_vector[1];

            if (edge_detected(scaled_r_v, x_pos, y_pos) ||
                cyli_detected(scaled_r_v[0] + x_pos, scaled_r_v[1] + y_pos)) {
                /* distance = ||scaled_r_v|| */
                float distance =  hypot(scaled_r_v[0], scaled_r_v[1]);

                /* greyscale luminosity of the wall by distance */
                float shade = sqrt(SCR_D / (distance));

                /* height of the wall to be drawn by distance */
                float height_scale = 1 / (distance);

                /* actual pixel height of height_scale */
                int start_y = (int)(HEIGHT * 0.5 * (1 - 1.4 *
                                               SCR_D * height_scale));
                int stop_y  = (int)(HEIGHT * 0.5 * (1 + 1.4 *
                                               SCR_D * height_scale));

                /* keep start_y & stop_y inside the screen range */
                if (start_y < 0)
                    start_y = 0;
                if (stop_y > HEIGHT)
                    stop_y = HEIGHT;

                /* draw the vertical line */
                for (int y=0; y<HEIGHT; ++y) {
                    if (start_y <= y && y < stop_y)
                        pix_img[(HEIGHT-1-y)*WIDTH + pix] = render_shade(shade);
                    else
                        pix_img[(HEIGHT-1-y)*WIDTH + pix] = 0;
                }
                break;
            }
            lambda += 0.05;
        }
        if (lambda >= 2 * MAP_X_SIZE / SCR_D) {
            for (int y=0; y<HEIGHT; ++y)
                pix_img[y*WIDTH + pix] = 0;
        }
    }
}

bool edge_detected(float *v, float x0, float y0)
{
    int x = round(v[0] + x0);
    int y = round(v[1] + y0);
    if (0 <= x && x < MAP_X_SIZE && 0 <= y && y < MAP_Y_SIZE)
        return (bool) map[y][x];
    return false;
}

bool cyli_detected(float x, float y)
{
    for (int i = 0; i < CYLIS_NUM; ++i) {
        if (hypot(cylis[i][0] - x, cylis[i][1] - y) < 0.2)
            return true;
    }
    return false;
}

Uint32 render_shade(float scale)
{
    Uint32 int_sc = 0xff & (Uint32)(scale*GREY_SC_LEN);
    return (int_sc<<16) | (int_sc<<8) | int_sc;
}

void update_cylis(void)
{
    for (int i = 0; i < CYLIS_NUM; ++i) {
        cylis[i][0] += RAND_FL(-1E-1, 1E-1);
        cylis[i][1] += RAND_FL(-1E-1, 1E-1);
    }
}
