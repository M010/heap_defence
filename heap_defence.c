//
// Created by moh on 30.11.2021.
//

#include <stdlib.h>
#include <string.h>
//#include <stdlib.h>

#include "../flipperzero-firmware/core/furi.h"
#include "../flipperzero-firmware/applications/gui/gui.h"
#include "../flipperzero-firmware/applications/input/input.h"
#include "../flipperzero-firmware/lib/STM32CubeWB/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.h"
#include "../flipperzero-firmware/applications/input/input.h"
#include "../flipperzero-firmware/firmware/targets/f7/furi-hal/furi-hal-resources.h"
#include <stdlib.h>
#include <string.h>

#define Y_FIELD_SIZE 6
#define Y_LAST (Y_FIELD_SIZE - 1)
#define X_FIELD_SIZE 12
#define X_LAST (Y_FIELD_SIZE - 1)

#define BOX_HEIGHT 10
#define BOX_WIDTH 10
#define TIMER_UPDATE_FREQ 10
#define BOX_DROP_RATE 5
#define BOX_GENERATION_RATE 10

#define PERSON_HEIGHT (BOX_HEIGHT * 2) // TODO Каждый раз будет заново считать
#define PERSON_WIDTH BOX_WIDTH
#define PERSON_STEP_PIX 1
#define PERSON_RIGHT (person->screen_x + PERSON_WIDTH)
#define PERSON_LEFT (person->screen_x)
#define PERSON_BOTTOM (person->screen_y - PERSON_HEIGHT)
#define PERSON_TOP (person->screen_y)

typedef u_int8_t byte;

typedef enum {
    StatusWaitingForStart,
    StatusGameInProgress,
    StatusGameOver,
    StatusGameExit
} GameStatuses;

typedef enum {
    PersonStatusDead,
    PersonStatusWalk,
    PersonStatusJump,
    PersonStatusPush
} PersonStatuses;

typedef struct {
    byte is_going_left;
    byte screen_x;
    byte screen_y;
    PersonStatuses status;
} Person;

typedef struct {
    byte x;
    byte y;
} Pixel;

typedef struct {
    byte type;
    byte state;
    byte shift;
    byte offset;
} Box;

typedef struct {
    Box** field;
    Person* person;
    GameStatuses game_status;
    byte tick_count;
} GameState;

typedef enum { EventKeyPress, EventGameTick } EventType;

typedef struct {
    EventType type;
    InputEvent input;
} GameEvent;

/**
 *
 * #Construct / Destroy
 */
GameState* allocGameState() {
    GameState* game_state = furi_alloc(sizeof(GameState));
    game_state->person = furi_alloc(sizeof(Person));
    game_state->field = furi_alloc(Y_FIELD_SIZE * sizeof(Box*));
    for(int y = 0; y < Y_FIELD_SIZE; ++y) {
        game_state->field[y] = furi_alloc(X_FIELD_SIZE * sizeof(Box));
    }
    game_state->person->screen_x = 64;
    game_state->person->screen_y = 32;
    game_state->game_status = StatusWaitingForStart;
    return game_state;
}

void game_state_destroy(GameState* game_state) {
    for(int y = 0; y < Y_FIELD_SIZE; ++y) {
        free(game_state->field[y]);
    }
    free(game_state->person);
    free(game_state->field);
    free(game_state);
}

/**
 *
 * #Box logic
 */

static void generate_box(GameState const* game_state) {
    furi_assert(game_state);
    if(game_state->tick_count % BOX_GENERATION_RATE) {
        return;
    }

    int x_offset = rand() % X_FIELD_SIZE;
    game_state->field[0][x_offset].state = 1;
    game_state->field[0][x_offset].offset = BOX_HEIGHT;
}

static void heap_swap(Box* first, Box* second) {
    Box temp = *first;
    first->state = second->state;
    second->type = temp.type; //чтобы не потерять текстуру
    *first = *second;
    *second = temp;
}

static void drop_box(GameState* game_state) {
    furi_assert(game_state);

    for(int x = 0; x < X_FIELD_SIZE; x++) {
        byte *of = &(game_state->field[Y_LAST][x].offset);
        *of = *of ? (*of - 1) : *of;
    }

    for(int y = Y_LAST; y > 0; y--) {
        for(int x = 0; x < X_FIELD_SIZE; x++) {
            Box* cur_cell = game_state->field[y] + x;
            Box* upper_cell = game_state->field[y - 1] + x;

            byte* offset = &(upper_cell->offset);
            if(*offset != 0) (*offset)--;

            if(cur_cell->state == 0 && (upper_cell->state != 0 && *offset == 0)) {
                *offset = (y == Y_LAST) ? BOX_HEIGHT : 0;
                heap_swap(cur_cell, upper_cell); //TODO: Think about
            }
        }
    }
}

static void clear_rows(Box** field) {
    int bottom_row = Y_FIELD_SIZE - 1;
    for(int x = 0; x < X_FIELD_SIZE; ++x) {
        if(field[bottom_row][x].state == 0) {
            return;
        }
    }
    memset(field[bottom_row], 0, sizeof(Box) * X_FIELD_SIZE);
    clear_rows(field);
}

/**
 * #Person logic
 */

//static void move_person(GameState *game_state) {
//	Person *person = game_state->person;
//
//	byte new_X_position = person->screen_x + (person->is_going_left) * PERSON_STEP_PIX;
//	switch (person->status) {
//		case PersonStatusJump: {
//			byte new_X_position = person->screen_x + (person->is_going_left) * PERSON_STEP_PIX;
//			byte new_Y_position = person->screen_y + PERSON_STEP_PIX;
//			break;
//		}
//		default: {
//			byte new_X_position = person->screen_x + (person->is_going_left) * PERSON_STEP_PIX;
//			break;
//		}
//	}
//
//
//	//byte new_Y_position = person->screen_y ;
//
//}

/**
 *
 * #Callback
 *
 */

static void draw_box(Canvas* canvas, Box* box, int tick, int x, int y) {
    if(!box) return;
    if(!box || !(box->state)) return;
    canvas_draw_box(
        canvas, x * BOX_HEIGHT, y * BOX_WIDTH + BOX_WIDTH - box->offset, BOX_WIDTH, BOX_HEIGHT);
}

static void heap_defense_render_callback(Canvas* const canvas, void* mutex) {
    int timeout = 25;
    const GameState* game_state = acquire_mutex((ValueMutex*)mutex, timeout);
    if(!game_state) return;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    for(int y = 1; y < Y_FIELD_SIZE; ++y) {
        for(int x = 0; x < X_FIELD_SIZE; ++x) {
            draw_box(canvas, &(game_state->field[y][x]), game_state->tick_count, x, y);
        }
    }
    canvas_draw_frame(
        canvas,
        game_state->person->screen_x,
        game_state->person->screen_y,
        PERSON_WIDTH,
        PERSON_HEIGHT);
    release_mutex((ValueMutex*)mutex, game_state);
}

static void heap_defense_input_callback(InputEvent* input_event, osMessageQueueId_t event_queue) {
    furi_assert(event_queue);

    GameEvent event;
    event.input = *input_event;
    event.type = EventKeyPress;
    osMessageQueuePut(event_queue, &event, 0, osWaitForever);
}

static void heap_defense_timer_callback(osMessageQueueId_t event_queue) {
    furi_assert(event_queue);

    GameEvent event;
    event.type = EventGameTick;
    osMessageQueuePut(event_queue, &event, 0, 0);
}

int32_t heap_defence_app(void* p) {
    srand(DWT->CYCCNT);

    furi_log_print(FURI_LOG_ERROR, "asdfjsaklllkfjjaksdlfjkasdfjaskldjfkas\n");
    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(GameEvent), NULL);
    GameState* game = allocGameState();

    ValueMutex state_mutex;
    if(!init_mutex(&state_mutex, game, sizeof(GameState))) {
        free(game);
        return 1;
    }

    ViewPort* view_port = view_port_alloc();
    furi_assert(view_port);
    view_port_draw_callback_set(view_port, heap_defense_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, heap_defense_input_callback, event_queue);
    osTimerId_t timer =
        osTimerNew(heap_defense_timer_callback, osTimerPeriodic, event_queue, NULL);
    osTimerStart(timer, osKernelGetTickFreq() / TIMER_UPDATE_FREQ);

    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    GameEvent event;
    while(game->game_status != StatusGameExit) {
        if(osMessageQueueGet(event_queue, &event, NULL, 100) != osOK) {
            furi_log_print(FURI_LOG_ERROR, "queue_get_failed");
            continue;
        }
        GameState* game_state = (GameState*)acquire_mutex_block(&state_mutex);
        if(event.type == EventKeyPress) {
            /// move player
            switch(event.input.key) {
            case InputKeyBack:
                game_state->game_status = StatusGameExit;
                break;
            case InputKeyUp:
                game_state->person->screen_y -= PERSON_STEP_PIX; // TODO bug prone
                break;
            case InputKeyDown:
                game_state->person->screen_y += PERSON_STEP_PIX;
                break;
            case InputKeyLeft:
                game_state->person->is_going_left = -1;
                game_state->person->screen_x -= PERSON_STEP_PIX;
                break;
            case InputKeyRight:
                game_state->person->is_going_left = 1;
                game_state->person->screen_x += PERSON_STEP_PIX;
            default:
                break;
            }
        } else if(event.type == EventGameTick) {
            /// apply logic
            generate_box(game_state);

            drop_box(game_state);
            //drop_person();
            // из-за новой логики можно чистить в конце цикла
            clear_rows(game_state->field);
            game_state->tick_count++;
        }
        release_mutex(&state_mutex, game_state);
        view_port_update(view_port);
    }

    osTimerDelete(timer);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close("gui");
    view_port_free(view_port);
    osMessageQueueDelete(event_queue);
    delete_mutex(&state_mutex);
    game_state_destroy(game);

    return 0;
}