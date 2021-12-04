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

#define Y_SIZE 128
#define X_SIZE 64
#define Y_FIELD_SIZE 8
#define X_FIELD_SIZE 12
#define BOX_HEIGHT 10
#define BOX_WIDTH 10
#define TIMER_UPDATE_FREQ 30
//#define GENERATE_BOX(x) { x->field[0][rand() % X_FIELD_SIZE]; }

typedef u_int8_t byte;

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
	byte state: 5;
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
    InputEvent 	input;
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

void game_state_destroy(GameState *game_state) {
	for (int y = 0; y < Y_FIELD_SIZE; ++y) {
		free(game_state->field[y]);
	}
	free(game_state->field);
	free(game_state);
}

static void generate_box(GameState const * game_state) {
	memset(game_state->field[0] + rand() % X_FIELD_SIZE, 0xff, sizeof(Box));
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
	int bottom_row = Y_FIELD_SIZE - 1;
	int flag = 0;
	for (int y = bottom_row; y > 0; --y) {
		for (int x = 0; x < X_FIELD_SIZE; ++x) {
			if (field[x][y].state == 0) {
				flag = 1;
				break ;
			}
		}
		if (flag == 1) {
			continue;
		}
		memset(field[bottom_row], 0, X_FIELD_SIZE);
		heap_swap(field + y, field + y - 1);
	}
}

static void heap_defense_render_callback(Canvas* const canvas, void* mutex) {
    static int i = 0;
    i++;
	int timeout = 25;
	const GameState *game_state = acquire_mutex((ValueMutex *)mutex, timeout);
	if (!game_state)
		return;

	canvas_clear(canvas);
	canvas_set_color(canvas, ColorBlack);
    if(i % 20 < 10) {
        canvas_draw_box(canvas, 0, 0, BOX_WIDTH, BOX_HEIGHT);
    }
	for (int y = 0; y < Y_FIELD_SIZE; ++y) {
		for (int x = 0; x < X_FIELD_SIZE; ++x) {
			if (game_state->field[y][x].state) {
				canvas_draw_box(canvas, x * BOX_HEIGHT, y * BOX_WIDTH, BOX_WIDTH, BOX_HEIGHT);
			}
		}
	}
	release_mutex((ValueMutex *)mutex, game_state);
}


static void heap_defense_input_callback(InputEvent* input_event, osMessageQueueId_t event_queue) {
	furi_assert(event_queue);

	GameEvent event;
    event.input = *input_event;
    event.type =EventKeyPress ;
	osMessageQueuePut(event_queue, &event, 0, osWaitForever);
}

static void heap_defense_timer_callback(osMessageQueueId_t event_queue) {
	furi_assert(event_queue);

	GameEvent event;
    event.type = EventGameTick;
//    event.input = nu
	osMessageQueuePut(event_queue, &event, 0, 0);
}


int32_t heap_defence_app(void* p){
    srand(DWT->CYCCNT);

    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(GameEvent), NULL);
    GameState *game = allocGameState();

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

    // Open GUI and register view_port
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    int errors = 0;
    GameEvent event;

    while (game->game_status != StatusGameExit && errors < 10) {
    	if (osMessageQueueGet(event_queue, &event, NULL, 100) != osOK) {
    		furi_log_print(FURI_LOG_ERROR, "queue_get_failed");
    		++errors;
    	}
    	GameState *game_state = (GameState *)acquire_mutex_block(&state_mutex);
    	errors = 0;
    	if (event.type == EventKeyPress) {
    		/// move player
    		switch (event.input.key) {
    			case InputKeyBack:
    				game->game_status = StatusGameExit;
    				break;
				default:
					break;
    		}
    	} else if (event.type == EventGameTick) {
    		/// apply logic
    		//move_person();
//    		drop_box(game_state);
    		//drop_person();
//    		clear_rows(game_state->field);
//    		generate_box(game_state);
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