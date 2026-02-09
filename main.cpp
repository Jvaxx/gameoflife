#include "main.h"
#include "logic.h"
#include "maths.h"
#include <SDL3/SDL_timer.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

static SDL_Window *game_window = std::nullptr_t();
static SDL_Renderer *game_renderer = std::nullptr_t();
static Screen_buffer *screen = new Screen_buffer{};
static World_view *game_world_view = new World_view{&screen->main_view};
static Menu_view *game_menu_view = new Menu_view{&screen->menu_view};
static Grid *game_grid = new Grid(1000, 1000);

// Events_button *bouton1 = new Events_button();
// Events_button *bouton2 = new Events_button();

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Init video en échec.\n";
        return SDL_APP_FAILURE;
    }
    if (!SDL_CreateWindowAndRenderer("Game of life",
                                     1440, 810,
                                     SDL_WINDOW_RESIZABLE,
                                     &game_window, &game_renderer)) {
        std::cerr << "Init renderer en échec.\n";
        return SDL_APP_FAILURE;
    }
    Game_state *state = new Game_state{};
    state->last_tick = SDL_GetPerformanceCounter();
    state->last_frame = SDL_GetPerformanceCounter();
    state->last_log = SDL_GetPerformanceCounter();
    state->last_input_proc = SDL_GetPerformanceCounter();
    *appstate = state;
    game_world_view->pix_per_m = 1.0;
    game_world_view->theta = 0.2;
    game_world_view->origin = {500, 500};

    // randomize_grid(*game_grid, 0.5f);

    // TODO: Boutons
    // game_menu_view->buttons.push_back(*bouton1);
    // game_menu_view->buttons.push_back(*bouton2);

    std::cout << "App initilisée.\n";
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    Game_state *state = static_cast<Game_state *>(appstate);
    uint64_t start_time{SDL_GetPerformanceCounter()};

    // Process input: max speed
    process_input(state, *game_world_view, *game_grid);

    // Update grid: target speed: state.mspt
    if (static_cast<float>((start_time - state->last_tick) * 1000) / SDL_GetPerformanceFrequency() > state->mspt) {
        uint64_t start_process_time{SDL_GetPerformanceCounter()};
        if (state->playing) {
            update_grid(*game_grid);
        }
        state->last_tick = SDL_GetPerformanceCounter();
        state->process_time += SDL_GetPerformanceCounter() - start_process_time;
        ++state->ticks_since_log;
    }

    // Render screen: target speed: 33 MSPRender
    if (static_cast<float>((start_time - state->last_frame) * 1000) / SDL_GetPerformanceFrequency() > 33) {

        // Render world view
        uint64_t start_render_time{SDL_GetPerformanceCounter()};
        screen->main_view.clear_pixel(0xFFFF00FF);
        state->clr_px_time += SDL_GetPerformanceCounter() - start_render_time;

        uint64_t start_fill_time{SDL_GetPerformanceCounter()};
        fill_grid2(game_world_view, game_grid, state);
        state->fill_tiles_time += SDL_GetPerformanceCounter() - start_fill_time;

        uint64_t start_grid_time{SDL_GetPerformanceCounter()};
        draw_grid(game_world_view, game_grid);
        screen->main_view.waiting_update = true;
        state->draw_grid_time += SDL_GetPerformanceCounter() - start_grid_time;

        // Render menu (pas vraiment implémenté):
        uint64_t start_render_menu_time{SDL_GetPerformanceCounter()};
        // if (screen->menu_view.active && screen->menu_view.waiting_update) {
        //     render_menu(game_menu_view);
        // }
        state->render_menu_time += SDL_GetPerformanceCounter() - start_render_menu_time;

        // Render screen
        uint64_t start_updt_buff{SDL_GetPerformanceCounter()};
        screen->main_view.upload_if_needed();
        // screen->menu_view.upload_if_needed();
        screen->render(game_renderer);
        state->buff_updt_time += SDL_GetPerformanceCounter() - start_updt_buff;

        state->last_frame = SDL_GetPerformanceCounter();
        state->frame_time += SDL_GetPerformanceCounter() - start_render_time;
        ++state->frames_since_log;
    }

    // Log stats: target speed: 2000 MSPLog
    if (static_cast<float>((start_time - state->last_log) * 1000) / SDL_GetPerformanceFrequency() > 2000) {
        print_logs(state);
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    Game_state *state = reinterpret_cast<Game_state *>(appstate);
    switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        std::string button{"?"};
        switch (event->button.button) {
        case SDL_BUTTON_LEFT:
            state->input.mouse_l.press = (event->button.down);
            state->input.mouse_l.x = event->button.x;
            state->input.mouse_l.y = event->button.y;
            break;
        case SDL_BUTTON_RIGHT:
            state->input.mouse_r.press = (event->button.down);
            state->input.mouse_r.x = event->button.x;
            state->input.mouse_r.y = event->button.y;
            break;
        case SDL_BUTTON_MIDDLE:
            break;
        case SDL_BUTTON_X1:
            break;
        case SDL_BUTTON_X2:
            break;
        default:
            break;
        }
        break;
    }

    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        switch (event->key.scancode) {
        case SDL_SCANCODE_A:
            state->input.mv_l.press = (event->key.down);
            break;
        case SDL_SCANCODE_S:
            state->input.mv_d.press = (event->key.down);
            break;
        case SDL_SCANCODE_W:
            state->input.mv_u.press = (event->key.down);
            break;
        case SDL_SCANCODE_D:
            state->input.mv_r.press = (event->key.down);
            break;
        case SDL_SCANCODE_H:
            state->input.rot_pos.press = (event->key.down);
            break;
        case SDL_SCANCODE_L:
            state->input.rot_neg.press = (event->key.down);
            break;
        case SDL_SCANCODE_J:
            state->input.wheel.scroll = (event->key.down);
            break;
        case SDL_SCANCODE_K:
            state->input.wheel.scroll = -(event->key.down);
            break;

        case SDL_SCANCODE_SPACE:
            state->input.pause.press = (event->key.down) ? 1 : state->input.pause.press;
            break;
        case SDL_SCANCODE_O:
            state->input.slow_down.press = (event->key.down) ? 1 : state->input.slow_down.press;
            break;
        case SDL_SCANCODE_P:
            state->input.speed_up.press = (event->key.down) ? 1 : state->input.speed_up.press;
            break;

        default:
            break;
        }
        break;
    }

    case SDL_EVENT_MOUSE_WHEEL:
        state->input.wheel.scroll = event->wheel.y;
        // std::cout << "x:" << event->wheel.y << ", int_x:" << event->wheel.integer_x << ", int_y" << event->wheel.integer_y << "\n";
        break;

    case SDL_EVENT_WINDOW_RESIZED:
        std::cout << "Resized: " << event->window.data1 << ", " << event->window.data2 << '\n';
        screen->resize(game_renderer, event->window.data1, event->window.data2);
        break;
    case SDL_EVENT_QUIT:
        std::cout << "On quitte avec succès.\n";
        return SDL_APP_SUCCESS;

    default:
        return SDL_APP_CONTINUE;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    std::cout << "On quitte.\n";
}
