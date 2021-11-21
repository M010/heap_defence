#pragma once

#include <vector>
#include "iostream"
#include <random>
#include <cassert>
#include <algorithm>
#include "Tile.h"

template <uint Rows, uint Cols>
class Frame
{
//	using Row = std::vector<char>;
	using Row = std::string;
	using Field = std::vector<Row>;


	const int rows_count;
	const int cols_size;
	Field field;
	std::mt19937 gen;

public:
	Frame() :
			rows_count(Rows), cols_size(Cols),
			field(Field(rows_count, Row(Cols, ' ')))
	{
		static_assert(Rows , "Rows must be greater then 0");
		static_assert(Cols , "Cols must be greater then 0");
	}

	void clean_rows(){
		static const Row full_row(Cols, '0');
		static const Row empty_row(Cols, ' ');

		field.erase(std::remove(field.begin(), field.end(), full_row), field.end());
		Field tmp{Rows - field.size(), empty_row};
		field.insert(field.begin(), tmp.begin(), tmp.end());
	}

	void generate_box()
	{
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
				{
					std::swap(pixel, upper_pixel);
				}
			}
		}
	}

	void step()
	{
		static int i;
		i++;
		if(!(i % 4))
			drop_box();
		if(!(i % 13))
			generate_box();

		clean_rows();
	}

	void print(){
		using std::cout;

		cout << std::string(field[0].size(), '_') << '\n';
		for(uint i_y = 0; i_y < rows_count; i_y++)
		{
			for(int i_x = 0; i_x < cols_size; i_x++) {
				cout << field[i_y][i_x];
			}
			cout << "|\n";
		}
		cout << std::string(field[0].size(), 'T') << '\n';
		cout << std::flush;
	}
};
