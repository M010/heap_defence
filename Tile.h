#pragma once
#include <cstdlib>
#include <string>
#include <vector>

class Tile
{
	uint x, y;
public:
	Tile(uint x, uint y);
	std::vector<std::string> shape = { "000", ""};
};



/*
 *      ***
 *       *
 *
 *       ***
 *         *
 *
 *       **
 *       **
 *
 *       *****
 *
 *       ***
 *       *
 *
 *       **
 *        **
 *
 *       **
 *      **
 *
 */