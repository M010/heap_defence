#include <iostream>
#include <thread>
#include <chrono>
#include <ncurses.h>
#include "Frame.h"

using namespace std;

void clear_screen()
{
	using chrono::milliseconds;

	static const milliseconds timespan(100); // or whatever
	std::this_thread::sleep_for(timespan);
	cout << "\033[2J\033[1;1H";
}

int main()
{
	Frame<10, 15> frame;
	initscr();
	noecho();
	while (1)
	{
		frame.step();
		frame.print();
		char ch = getch();
		putchar(ch);
		frame.get_ch(ch);
		refresh();
	}
	endwin();
	return 0;
}
