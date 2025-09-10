#include <genesis.h>
#include "gfx.h"

extern char debug_string[20];
#define DEBUG_STR_X 24

//enumerations
enum screens{
	STARTUP,
	MENU,
	GAME,
	OPTION,
	EXIT
};

//prototypes
void menu_loop();
void game_loop();
void option_loop();
