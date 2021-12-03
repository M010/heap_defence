#pragma once

#include <vector>
#include "iostream"
#include <random>
#include <cassert>
#include <algorithm>
#include <ncurses.h>
#include <chrono>
#include <thread>
#include "Tile.h"

using std::string;

template <int Rows, int Cols>
class Frame
{
//	using Row = std::vector<char>;
	using Row = std::string;
	using Field = std::vector<Row>;
	using Position = std::pair<int, int>;
	Position person = { Cols/2, Rows-1};
	static const char person_frame = 'M';
	static const char person_head = 'o';

	const int rows_count;
	const int cols_size;
	int key = -1;
	Field field;
	std::mt19937 gen;

public:
	Frame() :
			rows_count(Rows), cols_size(Cols),
			field(Field(rows_count, Row(Cols, ' ')))
	{
		static_assert(Rows > 0, "Rows must be greater then 0");
		static_assert(Cols > 0, "Cols must be greater then 0");
	}

	void clean_rows(){
		static const Row full_row(Cols, '0');
		static const Row empty_row(Cols, ' ');

		field.erase(std::remove(field.begin(), field.end(), full_row), field.end());
		Field tmp{Rows - field.size(), empty_row};
		field.insert(field.begin(), tmp.begin(), tmp.end());
	}



	void generate_box() {
		int drop_line = gen() % cols_size;
		field[0][drop_line] = '0';
	}

	void drop_box(){
		for(int i_y = rows_count - 1; i_y > 0; i_y--)
		{
			for(int i_x = 0; i_x < cols_size; i_x++)
			{
				char& pixel = field[i_y][i_x];
				char& upper_pixel = field[i_y - 1][i_x];

				if(' ' == pixel && '0' == upper_pixel)
					std::swap(pixel, upper_pixel);
			}
		}
	}

	char& get_by_position(Position p)
	{
		return field[p.second][p.first];
	}

	Position sum_positions(const Position& lhs, const Position& rhs)
	{
		return {lhs.first + rhs.first, lhs.second + rhs.second};
	}

	void move_person()
	{
		Position diff{0, 0};
		key = getch();
		bool can_move = true;

		if (key == 100)
		{
			diff.first++;
		} else if (key == 97)
		{
			diff.first--;
//		} else if (key == 115)
//		{
//			diff.second++;
		} else if (key == 119)
		{
			can_move = on_ground();
			diff.second--;
		}
		else
			return;

		if(!can_move)
			return;

		Position new_position = sum_positions(person, diff);

		if (new_position.first < 0 || new_position.first == cols_size)
			return;
		if (new_position.second < 0 || new_position.second == rows_count)
			return;

		if (get_by_position(new_position) != '0')
		{
			person = new_position;
		} else if (is_moveble(new_position, diff))
		{
			char& move_to = get_by_position(sum_positions(new_position, diff));
			std::swap(get_by_position(new_position), move_to);
			person = new_position;
		}
	}

	bool is_moveble(const Position& position, const Position& diff)
	{
		if((position.second != 0 &&
		  '0' == get_by_position({position.first, position.second - 1}))
		  || position.first == 0
		  || position.first == cols_size - 1)
		  return false;
		else if(get_by_position(sum_positions(position, diff)) == '0')
			return false;
		return true;
	}

	bool on_ground()
	{
		bool in_last_row = (person.second == rows_count - 1);
		if (in_last_row)
			return true;

		auto after_drop = sum_positions(person, {0, 1});
		bool is_flying = (get_by_position(after_drop) == ' ');
		return !is_flying;
	}


	void drop_person()
	{
		bool in_last_row = (person.second == rows_count - 1);
		if(in_last_row)
			return;

		auto after_drop = sum_positions(person, {0, 1});
		bool is_flying = (get_by_position(after_drop) == ' ');

		if(is_flying)
			person = after_drop;

	}

	void step()
	{
		using namespace std::chrono;

		auto start = steady_clock::now();
		static int i;
		i++;
		move_person();
		if(!(i % 3))
		{
			drop_box();
			drop_person();
		}
		if(!(i % 13))
			generate_box();
		clean_rows();
		auto end = steady_clock::now();
		milliseconds m = duration_cast<milliseconds>(start-end);
		std::this_thread::sleep_for(milliseconds{150} - m);
	}

	void print(){
		move(0,0);
		printw("%s\n" , string(field[0].size(), 'T').c_str());
		for(uint i_y = 0; i_y < rows_count; i_y++)
		{
			for(int i_x = 0; i_x < cols_size; i_x++) {
				if(person == std::pair<int, int>(i_x, i_y))
					addch(person_frame);
				else if (person.second != 0 &&
				std::make_pair(person.first, person.second - 1) == std::pair<int, int>(i_x, i_y))
					addch(person_head);
				else
					addch(field[i_y][i_x]);
			}
			printw("|\n");
		}
		printw("%s\n" , string(field[0].size(), 'T').c_str());
		printw("x:%d y:%d\n", person.first, person.second);
	}
};
