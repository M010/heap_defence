//
// Created by moh on 30.11.2021.
//
#include "../flipperzero-firmware/core/furi.h"
#include "../flipperzero-firmware/applications/gui/gui.h"
#include "../flipperzero-firmware/applications/input/input.h"
#include "../flipperzero-firmware/lib/STM32CubeWB/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.h"
#include <stdlib.h>
#include <string.h>
//#include <stdlib.h>

#ifdef _MLX_
//#include "minilibx/mlx.h"

void *window;
void *mlx;

static char empty_str[X_FIELD_SIZE];
//    memset(empty_str(sizeof(char)));
#endif

#define Y_SIZE 128
#define X_SIZE 64
#define Y_FIELD_SIZE 8
#define X_FIELD_SIZE 12
#define BOX_HEIGHT 10
#define BOX_WIDTH 10
#define TIMER_UPDATE_FREQ 4
//#define GENERATE_BOX(x) { x->field[0][rand() % X_FIELD_SIZE]; }

typedef u_int8_t byte;

typedef enum {
    ColorBlackKostyl = 0,
    ColorWhiteKosyyl = 0xFFFFFF,
} Colors;

typedef enum {
	StatusWaitingForStart,
	StatusGameInProgress,
	StatusGameOver,
	StatusGameExit
} GameStatuses;

typedef struct {
	int x;
	int y;
} Person;

typedef struct {
	byte x;
	byte y;
} Pixel;

typedef union {
	byte type: 1;
	byte texture: 2;
	byte state: 4;
} Box;

typedef struct {
	Box				**field;
	GameStatuses 	game_status;
	Person			person;
} GameState;

typedef enum {
    EventKeyPress,
    EventGameTick
} EventType;

typedef struct {
    EventType	type;
    InputType 	keycode;
} GameEvent;

GameState *allocGameState() {
    GameState *game_state = furi_alloc(sizeof(GameState));
    game_state->game_status = StatusWaitingForStart;
	game_state->field = furi_alloc(sizeof(char *) * Y_FIELD_SIZE);
	for (int x = 0; x < X_FIELD_SIZE; ++x) {
		furi_alloc(sizeof(char) * X_FIELD_SIZE);
	}
    return game_state;
}

static void heap_defense_timer_callback(osMessageQueueId_t event_queue) {
	furi_assert(event_queue);

	GameEvent event = {EventGameTick, 0};
	osMessageQueuePut(event_queue, &event, 0, 0);
}

static Pixel convert_box_position(byte x, byte y) {
	return (Pixel){.x = x * 10, y = y * 10};
}

static void heap_defense_render_callback(Canvas* const canvas, void* game) {
	int timeout = 25;
	const GameState *game_state = acquire_mutex((ValueMutex *)game, timeout);
	if (!game_state)
		return;

	for (int y = 0; y < Y_FIELD_SIZE; ++y) {
		for (int x = 0; x < X_FIELD_SIZE; ++x) {
			Pixel top_left = convert_box_position(x, y);
			if (game_state->field[y][x].state) {
				canvas_draw_box(canvas, top_left.x, top_left.y, BOX_WIDTH, BOX_HEIGHT);
			}
		}
	}
	release_mutex((ValueMutex *)game, game_state);
}

static void heap_defense_input_callback(InputEvent* input_event, osMessageQueueId_t event_queue) {
	furi_assert(event_queue);

	GameEvent event = {EventKeyPress, input_event->key};
	osMessageQueuePut(event_queue, &event, 0, osWaitForever);
}

static void generate_box(GameState const * game_state) {
	game_state->field[0][rand() % X_FIELD_SIZE].state = 1;
}

static void heap_swap(Box **first, Box **second) {
	Box *temp = *first;
	*first = *second;
	*second = temp;
}

static void drop_box(GameState *game_state) {
	for (int y = 0; y < Y_FIELD_SIZE; ++y) {
		for (int x = 0; x < X_FIELD_SIZE; ++x) {
			Box *current_cell = game_state->field[y] + x;
			Box *lower_cell = game_state->field[y + 1] + x;
			if (current_cell->state && lower_cell->state == 0) {
				heap_swap(&current_cell, &lower_cell);
			}
		}
	}
}

static void clear_rows(Box **field) {
	if (!memchr(field[Y_FIELD_SIZE - 1], 0, X_FIELD_SIZE))
		return;
	for (int y = Y_FIELD_SIZE - 1; y > 0; --y) {
		heap_swap(field + y, field + y - 1);
	}
}

int main() {
    srand(DWT->CYCCNT);

    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(GameEvent), NULL);
    GameState *game = allocGameState();

    ValueMutex state_mutex;
    if(!init_mutex(&state_mutex, game, sizeof(GameState))) {
        free(game);
        return 1;
    }

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, heap_defense_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, heap_defense_input_callback, event_queue);
	osTimerId_t timer =
			osTimerNew(heap_defense_timer_callback, osTimerPeriodic, event_queue, NULL);
	osTimerStart(timer, osKernelGetTickFreq() / TIMER_UPDATE_FREQ);

    // Open GUI and register view_port
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    GameEvent event;
    int errors = 0;
    while (game->game_status != StatusGameExit) {
    	if (osMessageQueueGet(event_queue, &event, NULL, 100) != osOK) {
			furi_log_print(FURI_LOG_ERROR, "queue_get_failed");
			++errors;
    	}
    	errors = 0;
    	if (event.type == EventKeyPress) {
    		/// move player
    	}
    	if (event.type == EventGameTick) {
    		/// apply logic
    		//move_person();
    		drop_box(game);
    		//drop_person();
    		clear_rows(game->field);
    		generate_box(game);
    	}

	}
	return 0;
}
