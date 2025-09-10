#include "defs.h"
#include "triotos_defs.h"

//VARIABLES
u8 gameState=INITIALIZE;
u8 boardState;

u8 fallerX,fallerY;
u8 faller[4][4];//1 index
Sprite* fallerSprite[3];

u8 board[BOARD_WIDTH+2][BOARD_HEIGHT+2];
bool matches[BOARD_WIDTH+2][BOARD_HEIGHT+2];
bool gravity[BOARD_WIDTH+2][BOARD_HEIGHT+2];

u8 destroyingTimer;

u8 pauseTimer=0;
bool isPaused=false;

u8 matchChainCounter;
u8 spawnCounter;

//INPUTS
bool INPUT_UP;
bool INPUT_DOWN;
bool INPUT_LEFT;
bool INPUT_RIGHT;
bool INPUT_A;
bool INPUT_B;
bool INPUT_C;
bool spawn_allowedToDown;
bool INPUT_START;
bool hasReleased;

//MOVEMENT
u8 landingTimer;
u8 naturalFallingTimer;

//DAS
u8 DAS_DOWN_restriction_counter;
u8 DAS_LEFT_restriction_counter;
u8 DAS_RIGHT_restriction_counter;

u8 DAS_down_adder;
u8 DAS_left_adder;
u8 DAS_right_adder;

u8 heldTimeLeft,heldTimeRight,heldTimeDown;

#define DAS_DOWN_RESTRICT_AMOUNT 8
#define DAS_LATERAL_RESTRICT_AMOUNT 8
#define DAS_ADDER_MAX 6

//NEXT QUEUE
#define NEXT_QUEUE_AMT 3//but we only draw 2
u8 nextfaller[NEXT_QUEUE_AMT+1][4][4];
bool flag_needDrawNext;
#define NEXT_QUEUE_X 10 //in tiles

//SCORE
s16 score;
#define SCORE_ADD_CLEAR_PER_PIECE 100
char score_string[10];

void game_loop()
{
	if(gameState==INITIALIZE)game_initialize();

	sys_inputs(JOY_readJoypad(JOY_1));
	managePause();

	if(gameState==PLAYING && isPaused==false)
	{
		game_timers();

		if(boardState==FALLING || boardState==LANDING)game_input();

		if(boardState==SPAWNING)game_spawnPiece();
		else if(boardState==FALLING || boardState==LANDING)game_falling();
		else if(boardState==INTOBOARD)game_intoBoard();
		else if(boardState==GRAVITY)game_checkGravity();
		else if(boardState==MATCHING)game_matching();
		else if(boardState==DESTROYING)game_destroying();
		else if(boardState==PROCESSGRAVITY)game_processGravity();
	}

	if(boardState==FALLING || boardState==LANDING)manage_faller();
	
	sprintf(debug_string,"FPS - %ld", SYS_getFPS());
	sprintf(score_string,"%d",score);

//VDP BEGIN
	SYS_doVBlankProcess();
	SPR_update();
	game_doDraw();

	VDP_drawText(debug_string,DEBUG_STR_X,26);
}

void game_initialize()
{
	setRandomSeed(GET_HVCOUNTER);

	for(u8 j=0;j<BOARD_HEIGHT;j++)//y
	{
		for(u8 i=1;i<BOARD_WIDTH;i++)//x
		{
			board[i][j]=0;
			matches[i][j]=false;
			gravity[i][j]=false;
		}
	}
/*
[0][1][2][3][4][5][6]
0=wall
board_width+1=wall
*/
	for(u8 j=1;j<=BOARD_HEIGHT+1;j++)//sides
	{
		board[0][j]=WALL;
		board[BOARD_WIDTH+1][j]=WALL;
	}

	for(u8 i=1;i<=BOARD_WIDTH;i++)//bottom
	{
		board[i][BOARD_HEIGHT+1]=WALL;
	}
/*10+2
[0]
[1]
[2]
[3]
[4]
[5]
[6]
[7]
[8]
[9]
[10]
[11] wall
*/	
	for(u8 x=0;x<=BOARD_WIDTH+1;x++)//boundary which is only for debug
	{
		for(u8 y=1;y<=BOARD_HEIGHT+1;y++)
		{
			u8 tileType=0;
			if(board[x][y]==WALL)tileType=5;
			VDP_fillTileMapRect(BG_B, 
				TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,tileType), 
				x<<1, 
				y<<1, 
				2, 2);
		}
	}

	spawnCounter=0;

	for(int i=1;i<NEXT_QUEUE_AMT;i++)create_next(i);
	flag_needDrawNext=true;

	for(int i=0;i<3;i++)fallerSprite[i] = SPR_addSprite(&sprite_gameTiles,-16,-16,TILE_ATTR(PAL0, FALSE, FALSE, FALSE));

	//spawnTypeCounter=0;

	score=0;

	gameState=PLAYING;
	boardState=SPAWNING;
}

void game_doDraw()
{
	if(flag_needDrawNext==true)
	{
		draw_next();//only needs to be drawn when we spawn!
		flag_needDrawNext=false;
	}

	if(boardState==SPAWNING || boardState==GRAVITY)
	{
		draw_clearBoard();
		draw_board(1,0,BOARD_WIDTH,BOARD_HEIGHT+1);
	}
	/*
	else if(boardState==INTOBOARD)
	{
		draw_board((fallerX-1)<<1,(fallerY-1)<<1,(fallerX+3)<<1,(fallerY+3)<<1);
	}
	else if(boardState==DESTROYING)
	{
		draw_clearBoard();
		draw_board(2,0,BOARD_WIDTH-1,BOARD_HEIGHT-1);
		draw_matches();
	}
	*/

	//draw_score();
}

void draw_piece(u8 x, u8 y, u8 blockColor)
{
	VDP_fillTileMapRectInc(BG_A, 
		TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,((blockColor-1)*2)+1+0xF),
		x<<1,
		y<<1,
		2, 1);

	VDP_fillTileMapRectInc(BG_A, 
		TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,((blockColor-1)*2)+1+0xF+0x8),
		x<<1,
		(y<<1)+1,
		2, 1);
}

void draw_board(u8 beginX, u8 beginY, u8 endX,  u8 endY)
{
	if(beginX==0)beginX=1;//we can never draw in [0][j] - that's wall

	for(u8 y=beginY;y<endY;y++)
	{
		for(u8 x=beginX;x<=endX;x++)
		{
			if(board[x][y]!=EMPTY)
			{
				draw_piece(x, y, board[x][y]);
			}
		}
	}
}

void game_spawnPiece()
{
	//printf("spawn piece (%d pieces)\n",countHowManyPieces()+3);

	#define SPAWN_X 3

	fallerX=SPAWN_X;
	fallerY=0;

	//printf("fallerX at spawn is %d\n",fallerX);

	for(u8 i=1;i<=3;i++)
	{
		for(u8 j=1;j<=3;j++)
		{
			faller[i][j]=nextfaller[1][i][j];
		}
	}

	shift_next();

	create_next(NEXT_QUEUE_AMT-1);
	flag_needDrawNext=true;
/*
	if(game_hasCollided(COLLIDE_SPAWN)==true)
	{
		game_gameOver();
		//return;
	}
*/
	spawn_allowedToDown=false;//can't hold DOWN button to continuously pull down through spawns

	naturalFallingTimer=NATURAL_FALLING_TIMER_AMOUNT-spawnCounter;
	if(naturalFallingTimer<NATURAL_FALLING_LOWER_LIMIT)naturalFallingTimer=NATURAL_FALLING_LOWER_LIMIT;

	matchChainCounter=0;

	spawnCounter++;

	boardState=FALLING;
}

void game_falling()
{
	//printf("falling. boardState:%d\n",boardState);

	if(boardState==FALLING && game_hasCollided(COLLIDE_DOWN)==true)
	{
		boardState=LANDING;
		landingTimer=LANDING_TIMER_AMOUNT;
	}
	else if(boardState==FALLING && game_hasCollided(COLLIDE_DOWN)==false && naturalFallingTimer==0)
	{
		fallerY++;
		naturalFallingTimer=NATURAL_FALLING_TIMER_AMOUNT;
	}

	if(boardState==LANDING && game_hasCollided(COLLIDE_DOWN)==false)//landingTimer==0
	{
		boardState=FALLING;
	}
	else if(landingTimer==0 && boardState==LANDING && game_hasCollided(COLLIDE_DOWN)==true)
	{
		//printf("landingTimer zero and boardState is landing. intoBoard\n");
		boardState=INTOBOARD;
	}
}

void game_input()
{

if(DAS_DOWN_restriction_counter>0)DAS_DOWN_restriction_counter--;

//vertical
	if(INPUT_DOWN)
	{
		if(boardState==FALLING)
		{
			if(spawn_allowedToDown==true)
			{
				if(DAS_DOWN_restriction_counter==0)
				{
					if(game_hasCollided(COLLIDE_DOWN)==false)
					{
						DAS_DOWN_restriction_counter=DAS_DOWN_RESTRICT_AMOUNT-DAS_down_adder;

						DAS_down_adder++;
						if(DAS_down_adder>DAS_ADDER_MAX)DAS_down_adder=DAS_ADDER_MAX;

						heldTimeDown++;

						fallerY++;
						naturalFallingTimer=NATURAL_FALLING_TIMER_AMOUNT;//reset natural falling timer
					}
				}
			}
		}
		else if(boardState==LANDING)
		{
			//printf("pressed DOWN when boardstate was landing. intoBoard\n");

			if(spawn_allowedToDown==true)
			{
				if(heldTimeDown==0)
				{
					if(game_hasCollided(COLLIDE_DOWN)==true)
					{
						boardState=INTOBOARD;
					}
				}				
			}	
		}
	}
	else if(INPUT_DOWN==false)
	{
		heldTimeDown=0;
		spawn_allowedToDown=true;
		DAS_down_adder=0;
		DAS_DOWN_restriction_counter=0;
	}

	if(INPUT_UP)
	{
		//hard drop
	}

//horizontal

	if(DAS_LEFT_restriction_counter>0)DAS_LEFT_restriction_counter--;
	if(DAS_RIGHT_restriction_counter>0)DAS_RIGHT_restriction_counter--;

	if(INPUT_LEFT==true)
	{
		if(DAS_LEFT_restriction_counter==0)
		{
			if(game_hasCollided(COLLIDE_LEFT)==false)
			{
				DAS_LEFT_restriction_counter=DAS_LATERAL_RESTRICT_AMOUNT-DAS_left_adder;

				DAS_left_adder++;
				if(DAS_left_adder>DAS_ADDER_MAX)DAS_left_adder=DAS_ADDER_MAX;

				heldTimeLeft++;

				fallerX--;

				//printf("moved left to X:%d Y:%d\n",fallerX,fallerY);
			}			
		}
	}
	else if(INPUT_LEFT==false)
	{
		heldTimeLeft=0;
		DAS_left_adder=0;
	}

	if(INPUT_RIGHT)
	{
		if(DAS_RIGHT_restriction_counter==0)
		{
			if(game_hasCollided(COLLIDE_RIGHT)==false)
			{
				DAS_RIGHT_restriction_counter=DAS_LATERAL_RESTRICT_AMOUNT-DAS_right_adder;

				DAS_right_adder++;
				if(DAS_right_adder>DAS_ADDER_MAX)DAS_right_adder=DAS_ADDER_MAX;

				heldTimeRight++;

				fallerX++;

				//printf("moved right to X:%d Y:%d\n",fallerX,fallerY);
			}			
		}
	}
	else if(INPUT_RIGHT==false)
	{
		heldTimeRight=0;
		DAS_right_adder=0;
	}

//actions
	if(hasReleased==false)
	{
		if(INPUT_A==false && INPUT_B==false && INPUT_C==false)hasReleased=true;
	}
	else if(hasReleased==true)
	{
		if(INPUT_A || INPUT_C)
		{
			//if(game_hasCollided(COLLIDE_ROTATE_CCW)==false)
			//{
				//printf("pressed A to rotate CCW\n");

				game_rotate(true);
				hasReleased=false;
			//}
		}

		if(INPUT_B)
		{
			//if(game_hasCollided(COLLIDE_ROTATE_CW)==false)
			//{
				//printf("pressed B to rotate CW\n");

				game_rotate(false);
				hasReleased=false;
			//}
		}	
	}
}

void game_timers()
{
	if(landingTimer>0)landingTimer--;
	if(naturalFallingTimer>0)naturalFallingTimer--;
	if(destroyingTimer>0)destroyingTimer--;
}

void sys_inputs(u16 buttons)
{
	if (buttons & BUTTON_DOWN)INPUT_DOWN=true;
	else INPUT_DOWN=false;

	if (buttons & BUTTON_UP)INPUT_UP=true;
	else INPUT_UP=false;

	if (buttons & BUTTON_LEFT)INPUT_LEFT=true;
	else INPUT_LEFT=false;

	if (buttons & BUTTON_RIGHT)INPUT_RIGHT=true;
	else INPUT_RIGHT=false;

	if (buttons & BUTTON_A)INPUT_A=true;
	else INPUT_A=false;

	if (buttons & BUTTON_B)INPUT_B=true;
	else INPUT_B=false;

	if (buttons & BUTTON_C)INPUT_C=true;
	else INPUT_C=false;

	if (buttons & BUTTON_START)INPUT_START=true;
	else INPUT_START=false;
}

bool game_hasCollided(u8 direction)
{
	if(direction==COLLIDE_DOWN)
	{
		for(u8 collideY=3;collideY>0;collideY--)
		{
			for(u8 collideX=1;collideX<=3;collideX++)
			{
				if(faller[collideX][collideY]!=EMPTY && board[collideX+fallerX-2][collideY+fallerY-1]!=EMPTY)
				{
					//VDP_drawText("collis down", 4, 26);
					return true;
				}
			}
		}

		return false;
	}
	else if(direction==COLLIDE_LEFT)
	{
		for(u8 collideX=1;collideX<=3;collideX++)
		{
			for(u8 collideY=3;collideY>=1;collideY--)
			{
				if(faller[collideX][collideY]!=EMPTY && board[collideX+fallerX-3][collideY+fallerY]!=EMPTY)
				{
					//printf("[LEFT] collis at X:%d\n",collideX+fallerX);
					return true;					
				}
			}
		}

		return false;
	}
	else if(direction==COLLIDE_RIGHT)
	{
		for(u8 collideX=3;collideX>=1;collideX--)
		{
			for(u8 collideY=3;collideY>=1;collideY--)
			{
				if(faller[collideX][collideY]!=EMPTY && board[collideX+fallerX-1][collideY+fallerY]!=EMPTY)
				{
					//printf("[RIGHT] collis at X:%d\n",collideX+fallerX);
					return true;					
				}
			}
		}

		return false;
	}

	if(direction==COLLIDE_ROTATE_CW)
	{
		//printf("attempting rotate CW\n");

		if(faller[1][1]!=EMPTY && board[fallerX+3][fallerY+1]!=EMPTY)return true; //1,1 - 3,1
		if(faller[1][2]!=EMPTY && board[fallerX+2][fallerY+1]!=EMPTY)return true; //1,2 - 2,1
		if(faller[1][3]!=EMPTY && board[fallerX+1][fallerY+1]!=EMPTY)return true; //1,3 - 1,1
		if(faller[2][1]!=EMPTY && board[fallerX+3][fallerY+2]!=EMPTY)return true; //2,1 - 3,2
		//2,2 - 2,2
		if(faller[2][3]!=EMPTY && board[fallerX+1][fallerY+2]!=EMPTY)return true; //2,3 - 1,2
		if(faller[3][1]!=EMPTY && board[fallerX+3][fallerY+3]!=EMPTY)return true; //3,1 - 3,3
		if(faller[3][2]!=EMPTY && board[fallerX+2][fallerY+3]!=EMPTY)return true; //3,2 - 2,3
		if(faller[3][3]!=EMPTY && board[fallerX+1][fallerY+3]!=EMPTY)return true; //3,3 - 1,3

		return false;
	}
	else if(direction==COLLIDE_ROTATE_CCW)
	{
		//printf("attempting rotate CCW\n");

		if(faller[1][1]!=EMPTY && board[fallerX+1][fallerY+3]!=EMPTY)return true;
		if(faller[2][1]!=EMPTY && board[fallerX+1][fallerY+2]!=EMPTY)return true;
		if(faller[3][1]!=EMPTY && board[fallerX+1][fallerY+1]!=EMPTY)return true;
		if(faller[3][2]!=EMPTY && board[fallerX+2][fallerY+1]!=EMPTY)return true;
		if(faller[3][3]!=EMPTY && board[fallerX+3][fallerY+1]!=EMPTY)return true;
		if(faller[2][3]!=EMPTY && board[fallerX+3][fallerY+2]!=EMPTY)return true;
		if(faller[1][3]!=EMPTY && board[fallerX+3][fallerY+3]!=EMPTY)return true;
		if(faller[1][2]!=EMPTY && board[fallerX+2][fallerY+3]!=EMPTY)return true;

		return false;
	}
	
	/*
	else if(direction==COLLIDE_SPAWN)
	{
		for(u8 collideX=1;collideX<=3;collideX++)
		{
			for(u8 collideY=1;collideY<=3;collideY++)
			{
				if(faller[collideX][collideY]!=EMPTY && board[collideX+SPAWN_X][collideY+SPAWN_Y]!=EMPTY)
				{
					VDP_drawText("collis spawn", 4, 25);
					return true;
				}
			}
		}
		return false;
	}
	*/
	//printf("no collis\n");
	return false;//should never actually hit
}

void game_rotate(bool isCCW)
{
	u8 tempfaller[4][4];//={0};

	for (u8 tempfallerX=1;tempfallerX<=3;tempfallerX++)
	{
		for (u8 tempfallerY=1;tempfallerY<=3;tempfallerY++)
		{
			tempfaller[tempfallerX][tempfallerY]=faller[tempfallerX][tempfallerY];
		}
	}

	if(isCCW)
	{
		faller[1][1]=tempfaller[3][1];
        faller[2][1]=tempfaller[3][2];
        faller[3][1]=tempfaller[3][3];
        faller[1][2]=tempfaller[2][1];
        faller[3][2]=tempfaller[2][3];
        faller[1][3]=tempfaller[1][1];
        faller[2][3]=tempfaller[1][2];
        faller[3][3]=tempfaller[1][3];
	}
	else//ROTATE CW
	{
        faller[1][1]=tempfaller[1][3];
        faller[2][1]=tempfaller[1][2];
        faller[3][1]=tempfaller[1][1];
        faller[1][2]=tempfaller[2][3];
        faller[3][2]=tempfaller[2][1];
        faller[1][3]=tempfaller[3][3];
        faller[2][3]=tempfaller[3][2];
        faller[3][3]=tempfaller[3][1];
	}

	if( (faller[2][1]!=EMPTY && faller[2][3]!=EMPTY) || (faller[1][2]!=EMPTY && faller[3][2]!=EMPTY) )
	{
		//it was a long piece
		return;
	}
	else
	{
		if(faller[1][1]==EMPTY && faller[2][1]==EMPTY && faller[3][1]==EMPTY)//top row empty
		{
			faller[1][1]=faller[1][2];//copy upwards
			faller[2][1]=faller[2][2];
			faller[3][1]=faller[3][2];

			faller[1][2]=faller[1][3];
			faller[2][2]=faller[2][3];
			faller[3][2]=faller[3][3];

			faller[1][3]=EMPTY;
			faller[2][3]=EMPTY;
			faller[3][3]=EMPTY;
			return;
		}
		if(faller[1][3]==EMPTY && faller[2][3]==EMPTY && faller[3][3]==EMPTY)//bottom row empty
		{
			faller[1][3]=faller[1][2];//copy downwards
			faller[2][3]=faller[2][2];
			faller[3][3]=faller[3][2];

			faller[1][2]=faller[1][1];
			faller[2][2]=faller[2][1];
			faller[3][2]=faller[3][1];

			faller[1][1]=EMPTY;
			faller[2][1]=EMPTY;
			faller[3][1]=EMPTY;
		}
	}
}

void game_intoBoard()
{
	//printf("intoBoard. change to spawning\n");

	for(u8 intoBoardX=1;intoBoardX<=3;intoBoardX++)
	{
		for(u8 intoBoardY=1;intoBoardY<=3;intoBoardY++)
		{
			if(faller[intoBoardX][intoBoardY]!=EMPTY)
			{
				//printf("intoBoard at X:%d Y:%d ~ board width:%d height:%d\n",fallerX+intoBoardX,fallerY+intoBoardY,BOARD_WIDTH,BOARD_HEIGHT);

				board[fallerX+intoBoardX-2][fallerY+intoBoardY-2]=faller[intoBoardX][intoBoardY];
			}
		}
	}

	boardState=GRAVITY;
}

void game_matching()
{
	//printf("matching start (%d pieces)\n",countHowManyPieces());

	//VDP_drawText("game_matching", 6, 23);
	bool hadMatches=false;

	for(u8 j=1;j<=(BOARD_HEIGHT-2);j++)//y - from top to bottom
	{
		for(u8 i=2;i<=(BOARD_WIDTH-3-1);i++)//x - from left to right
		{
			if(board[i][j]!=EMPTY)
			{
				if(board[i][j]==board[i+1][j] && board[i][j]==board[i+1][j+1] && board[i][j]==board[i][j+1])
				{
					//printf("game_matching: square match at X:%d Y:%d\n",i,j);

					matches[i][j]=true;
					matches[i+1][j]=true;
					matches[i][j+1]=true;
					matches[i+1][j+1]=true;

					hadMatches=true;
				}
			}
		}
	}

	if(hadMatches==false)boardState=SPAWNING;
	else if(hadMatches==true)
	{
		matchChainCounter++;
		//if(matchChainCounter>1)printf("CHAIN! %d\n",matchChainCounter);

		boardState=DESTROYING;
	}		
}

void draw_matches()
{
	for(u8 j=1;j<=(BOARD_HEIGHT-2);j++)//y - from top to bottom
	{
		for(u8 i=2;i<=(BOARD_WIDTH-3);i++)//x - from left to right
		{
			if(matches[i][j]==true)
			{
				//printf("draw match section at X:%d Y:%d\n",i,j);

				draw_piece(i, j,7);
			}
		}
	}	
}

void game_destroying()
{
	//printf("game_destroying\n");

	#define DESTROY_TIMER_AMOUNT 30

	u8 matchComboCounter=0;

	if(destroyingTimer==0)
	{
		//printf("game_destroying - initialize destroyingTimer\n");

		destroyingTimer=DESTROY_TIMER_AMOUNT;
	}
	else if(destroyingTimer==1)
	{
		for(u8 j=0;j<=(BOARD_HEIGHT-2);j++)//y - from top to bottom
		{
			for(u8 i=1;i<=(BOARD_WIDTH-3);i++)//x - from left to right
			{
				if(matches[i][j]==true)
				{	
					matchComboCounter++;

					//printf("game_destroying - #%d @ [%d,%d]\n",matchComboCounter,i,j);
					
					matches[i][j]=false;
					board[i][j]=EMPTY;

					score+=(SCORE_ADD_CLEAR_PER_PIECE<<(matchChainCounter-1));
				}
			}
		}

		destroyingTimer=0;
		boardState=GRAVITY;
	}

	//if(matchComboCounter>4)printf("COMBO! %d\n",matchComboCounter);

	//printf("game_destroying finished - with %d pieces\n",countHowManyPieces());
}

void game_gameOver()
{
	VDP_drawText("game over", 4, 27);
	gameState=GAMEOVER;
}

/*
u8 countHowManyPieces()
{
	u8 debug_numPieces=0;

	for(u8 j=BOARD_HEIGHT-8;j<BOARD_HEIGHT-1;j++)//y
	{
		for(u8 i=2;i<(BOARD_WIDTH-3);i++)//x
		{
			if(board[i][j]!=EMPTY)
			{
				debug_numPieces++;
			}	
		}
	}

	//printf("@@@@debug_countHowManyPieces:%d\n",debug_numPieces);

	return debug_numPieces;
}
*/

void game_checkGravity()
{
	//printf("game_checkGravity (with %d pieces)\n",countHowManyPieces());
	
	//VDP_drawText("game_checkGravity", 7, 24);

	bool hadGravity=false;

	for(u8 y=BOARD_HEIGHT-1;y>0;y--)//y - from bottom to top
	{
		for(u8 x=1;x<=BOARD_WIDTH;x++)//x - from left to right
		{
			if(board[x][y]!=EMPTY)
			{
				if(board[x][y+1]==EMPTY)
				{
					//VDP_drawText("we had gravity", 8, 25);
					hadGravity=true;
					gravity[x][y]=true;						
				}
			}
		}
	}

//change board state depending on outcome (no more gravity or further gravity to check)
	if(hadGravity==true)boardState=PROCESSGRAVITY;
	else if(hadGravity==false)boardState=MATCHING;

	//printf("game_checkGravity finished. now with %d pieces\n",countHowManyPieces());
}

bool doesItReachBottom(u8 x, u8 y)//repurpose for matching
{
	//printf("doesItReachBottom: [%d,%d]\n",x,y);

	u8 currentSearch=0;
	u8 stack=0;

	u8 searchingX[BOARD_WIDTH*BOARD_HEIGHT]={0};
	u8 searchingY[BOARD_WIDTH*BOARD_HEIGHT]={0};

	searchingX[0]=x;
	searchingY[0]=y;

	bool alreadyChecked[BOARD_WIDTH+1][BOARD_HEIGHT+1]={false};

	while(currentSearch<=stack)
	{
		//printf("doesItReachBottom: loop start. X:%d Y:%d, currentSearch:%d\n",searchingX[currentSearch],searchingY[currentSearch],currentSearch);

		if(searchingY[currentSearch]>=BOARD_HEIGHT-2)
		{
			//printf("!!!!!!!!!!we hit bottom at [%d,%d] - leaving (%d,%d does hit bottom)\n",searchingX[currentSearch],searchingY[currentSearch],x,y);
			
			return true;
		}
		else
		{
			//printf("we didn't hit bottom yet\n");

			alreadyChecked[searchingX[currentSearch]][searchingY[currentSearch]]=true;

			if(board[searchingX[currentSearch]][searchingY[currentSearch]+1]!=EMPTY && board[searchingX[currentSearch]][searchingY[currentSearch]+1]!=WALL && alreadyChecked[searchingX[currentSearch]][searchingY[currentSearch]+1]==false)
			{
				//printf("Y+1 hit at [%d,%d]\n",searchingX[currentSearch],searchingY[currentSearch]);

				stack++;
				searchingX[stack]=searchingX[currentSearch];
				searchingY[stack]=searchingY[currentSearch]+1;

				alreadyChecked[searchingX[currentSearch]][searchingY[currentSearch]+1]=true;
			}

			if(board[searchingX[currentSearch]-1][searchingY[currentSearch]]!=EMPTY && board[searchingX[currentSearch]-1][searchingY[currentSearch]]!=WALL && alreadyChecked[searchingX[currentSearch]-1][searchingY[currentSearch]]==false)
			{
				//printf("X-1 hit at [%d,%d]\n",searchingX[currentSearch],searchingY[currentSearch]);

				stack++;
				searchingX[stack]=searchingX[currentSearch]-1;
				searchingY[stack]=searchingY[currentSearch];

				alreadyChecked[searchingX[currentSearch]-1][searchingY[currentSearch]]=true;
			}

			if(board[searchingX[currentSearch]+1][searchingY[currentSearch]]!=EMPTY && board[searchingX[currentSearch]+1][searchingY[currentSearch]]!=WALL && alreadyChecked[searchingX[currentSearch]+1][searchingY[currentSearch]]==false)
			{
				//printf("X+1 hit at [%d,%d]\n",searchingX[currentSearch],searchingY[currentSearch]);

				stack++;
				searchingX[stack]=searchingX[currentSearch]+1;
				searchingY[stack]=searchingY[currentSearch];

				alreadyChecked[searchingX[currentSearch]+1][searchingY[currentSearch]]=true;
			}

			if(board[searchingX[currentSearch]][searchingY[currentSearch]-1]!=EMPTY && board[searchingX[currentSearch]][searchingY[currentSearch]-1]!=WALL && alreadyChecked[searchingX[currentSearch]][searchingY[currentSearch]-1]==false)
			{
				//printf("Y-1 hit at [%d,%d]\n",searchingX[currentSearch],searchingY[currentSearch]);

				stack++;
				searchingX[stack]=searchingX[currentSearch];
				searchingY[stack]=searchingY[currentSearch]-1;

				alreadyChecked[searchingX[currentSearch]][searchingY[currentSearch]-1]=true;
			}

			currentSearch++;
		}
	}

	return false;
}

void game_processGravity()
{
	//printf("game_processGravity STARTED with %d pieces\n",countHowManyPieces());
	//VDP_drawText("game_processGravity", 9, 25);

	for(u8 y=BOARD_HEIGHT;y>0;y--)//from bottom to top
	{
		for(u8 x=1;x<=BOARD_WIDTH;x++)//from left to right
		{
			if(gravity[x][y]==true)
			{
				//printf("game_processGravity - falling piece at [X:%d Y:%d]\n",x,y);

				board[x][y+1]=board[x][y];
				board[x][y]=EMPTY;

				gravity[x][y]=false;
			}
		}
	}

	boardState=GRAVITY;

	//printf("game_processGravity ENDED with %d pieces\n",countHowManyPieces());
}

void managePause()
{
	#define PAUSE_TIMER_AMT 16

	if(gameState==GAMEOVER)
	{
		isPaused=false;
		return;
	}

	if(pauseTimer>0)pauseTimer--;

	if(INPUT_START==true)
	{
		if(pauseTimer==0)
		{
			//printf("toggle pause %d\n",isPaused);

			pauseTimer=PAUSE_TIMER_AMT;

			if(isPaused==true)
			{
				isPaused=false;
				VDP_drawText("      ", 12, 13);
				draw_board(2,0,BOARD_WIDTH-1,BOARD_HEIGHT-1);
				return;
			}
			else if(isPaused==false)
			{
				isPaused=true;
				VDP_drawText("PAUSED", 12, 13);
				return;
			}			
		}
	}
}

u8 GetRandomValue(u8 rangeStart, u8 rangeEnd)
{
	return (random() % (rangeEnd + 1 - rangeStart)) + rangeStart;
}

void draw_next()
{
	VDP_clearTileMapRect(BG_A, NEXT_QUEUE_X<<1, 4, 10, 20);//clear it

	for(u8 nextPos=1;nextPos<NEXT_QUEUE_AMT;nextPos++)
	{
		for(u8 i=1;i<=3;i++)
		{
			for(u8 j=1;j<=3;j++)
			{
				if(nextfaller[nextPos][i][j]!=EMPTY)
				{
					draw_piece(
						NEXT_QUEUE_X+i,
						4+j+((nextPos-2)<<2)+nextPos, 
						nextfaller[nextPos][i][j]);
				}
			}
		}
	}
}

void create_next(u8 whichPosition)
{
	for(u8 i=1;i<=3;i++)
	{
		for(u8 j=1;j<=3;j++)
		{
			nextfaller[whichPosition][i][j]=EMPTY;
		}
	}

	#define NUMBER_OF_COLORS 3

	u8 color1=GetRandomValue(1,NUMBER_OF_COLORS);
	u8 color2=GetRandomValue(1,NUMBER_OF_COLORS);
	u8 color3;
	if(GetRandomValue(0,1)==1)color3=color1;
	else color3=color2;

	nextfaller[whichPosition][2][2]=color1;

	if(GetRandomValue(0,1)==0)//tall
	{
		nextfaller[whichPosition][1][2]=color2;
		nextfaller[whichPosition][3][2]=color3;
	}
	else//elbow
	{
		nextfaller[whichPosition][2][3]=color2;
		nextfaller[whichPosition][3][3]=color3;
	}
}

void shift_next()
{
	for(int a=1;a<NEXT_QUEUE_AMT-1;a++)
	{
		for(u8 i=1;i<=3;i++)
		{
			for(u8 j=1;j<=3;j++)nextfaller[a][i][j]=nextfaller[a+1][i][j];
		}		
	}
}

void draw_clearBoard()
{
	VDP_clearTileMapRect(BG_A, 2, 0, (BOARD_WIDTH-0)<<1,(BOARD_HEIGHT-0)<<1);
}

void manage_faller()
{
	int sprIndex=0;

	for(u8 i=1;i<=3;i++)
	{
		for(u8 j=1;j<=3;j++)
		{
			if(faller[i][j]!=EMPTY)
			{
				SPR_setPosition(fallerSprite[sprIndex],((fallerX+i-2)<<4),(fallerY+j-2)<<4);
				SPR_setFrame(fallerSprite[sprIndex], faller[i][j]-1);
				sprIndex++;
				//if(sprIndex>2)return;
			}
		}
	}
}

void draw_score()
{
	VDP_drawText("SCORE",DEBUG_STR_X,23);
	VDP_drawText(score_string,DEBUG_STR_X+1,24);
}