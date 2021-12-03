//
// Created by moh on 30.11.2021.
//
#include <string.h>
#define _MLX_

#ifdef _MLX_
#include "minilibx/mlx.h"
#endif

#define Y_SIZE 128
#define X_SIZE 64


#define Y_FIELD_SIZE 8
#define X_FIELD_SIZE 12

#define BOX_SIZE 42
#define WHITE 0xFFFFFFFF
#define BLACK 0x0

typedef struct Person{
	int x;
	int y;
	//TODO another fields
} Person_t;

typedef struct Box{
	int line;
	int y;
	//TODO another fields
} Box;


typedef struct Game{
	//+1 for '\0'
	char  field[Y_FIELD_SIZE][X_FIELD_SIZE + 1];
	Person_t person;
	//TODO::Dropping boxes!?
} Game_t;

Game_t g_game;

void *window;
void *mlx;

static char empty_str[X_FIELD_SIZE + 1] = "            ";

#ifdef _MLX_

#endif



void init()
{
	mlx = mlx_init();
	window = mlx_new_window(mlx,Y_SIZE ,X_SIZE, "HI");
//	mlx_clear_window(mlx, window);

	///Clear field array
	for(int row = 0; row < Y_FIELD_SIZE; row++)
		strcpy(g_game.field[row], empty_str);
}

void draw_box(int x, int y, int color)
{
	for(int i_x = 0; i_x < BOX_SIZE; i_x++)
	{
		for (int i_y = 0; i_y < BOX_SIZE; i_y++)
		{
			mlx_pixel_put(mlx, window, i_x + x, i_y + y, color);
		}
	}
}

//void draw_field_box(int x, int y)
//{
//	mlx_pixel_put()
//}

int main()
{
	init();
	draw_box(0, 0, WHITE);
	mlx_do_sync(mlx);
	while (1);
}