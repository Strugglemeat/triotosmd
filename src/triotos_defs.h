// DEFINITIONS --------------------------

#define BOARD_WIDTH 5
#define BOARD_HEIGHT 10

#define LANDING_TIMER_AMOUNT 120 //2 seconds
#define NATURAL_FALLING_TIMER_AMOUNT 120 //2 seconds
#define NATURAL_FALLING_LOWER_LIMIT 20

#define EMPTY 0

// function prototypes --------

//GENERAL
void game_loop();
void game_doDraw();
void sys_inputs(u16 buttons);
void game_initialize();
void game_input();
void game_gameOver();
void managePause();

void game_timers();

void game_spawnPiece();

void game_falling();
bool game_hasCollided(u8 direction);
void game_intoBoard();
void game_rotate(bool reverse);

void game_checkGravity();
void game_processGravity();

bool doesItReachBottom(u8 x, u8 y);

u8 GetRandomValue(u8 rangeStart, u8 rangeEnd);

void create_next(u8 whichPosition);
void shift_next();

//MATCHING
void game_matching();
void game_processMelter();

//DRAWING
void draw_piece(u8 x, u8 y, u8 blockColor);
void draw_board(u8 beginX, u8 beginY, u8 endX,  u8 endY);
void draw_matches();
void game_destroying();
void draw_next();
void draw_clearBoard();
void draw_score();

//FALLER
void manage_faller();

//DEBUG
void debug_insertboardData();
u8 countHowManyPieces();

//ENUMERATIONS ---------
enum gameStates{
	INITIALIZE,
	COUNTDOWN,
	PLAYING,
	PAUSED,
	GAMEOVER
};

enum boardStates
{
	SPAWNING,
	FALLING,
	LANDING,
	INTOBOARD,
	MATCHING,
	DESTROYING,
	GRAVITY,
	PROCESSGRAVITY
};

enum collisionDirections
{
	COLLIDE_DOWN,
	COLLIDE_LEFT,
	COLLIDE_RIGHT,
	COLLIDE_ROTATE_CW,
	COLLIDE_ROTATE_CCW,
	COLLIDE_SPAWN
};

enum drawPieceTypes
{
	NORMAL,
	DISAPPEAR,
	WALL=0xFF
};