#include <genesis.h>
#include "gfx.h"

//variables
extern u8 whichScreen;
extern const u16 SCREEN_WIDTH;
extern const u16 SCREEN_HEIGHT;
extern const bool debug_on;
extern char debug_string[20];
#define DEBUG_STR_X 24

//*********function prototypes
//general
void startup();
void shutDown();
void generalInputs();

//render and textures
void myBeginRender();
void myEndRender();
void renderStartup();
void renderShutdown();
void loadTextures(u8 whichScreen);

//enumerations
enum screens{
	STARTUP,
	MENU,
	GAME,
	OPTION,
	EXIT
};