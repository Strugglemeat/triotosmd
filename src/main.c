#include "defs.h"
#include "triotos_defs.h"

u8 whichScreen=GAME;
char debug_string[20] = "";

int main(bool hard)
{
    u16 ind;

    // disable interrupt when accessing VDP
    SYS_disableInts();
    // initialization
    VDP_setScreenWidth256();
    //VDP_setHilightShadow(1);

    // set all palette to black
    PAL_setColors(16, (u16*) palette_black, 64, DMA);
    VDP_clearTileMapRect(BG_A,0,0,40,28);

    // load background tilesets in VRAM
    ind = TILE_USER_INDEX;
    u16 idx1 = ind;
    VDP_loadTileSet(gameTiles.tileset,idx1,DMA);
    ind += gameTiles.tileset->numTile;
    u16 idx2 = ind;
    VDP_drawImageEx (BG_B, &backgroundPic, TILE_ATTR_FULL(PAL3, FALSE, FALSE, FALSE, idx2), 0, 0, TRUE, DMA);
    ind += backgroundPic.tileset->numTile;

    PAL_setColors(0,  (u16*)gameTiles.palette->data, 16, CPU);
    
    // VDP process done, we can re enable interrupts
    SYS_enableInts();

    SPR_init();

    while(TRUE)
    {
        switch(whichScreen)
        {
            //case MENU:
            //  menu_loop();
            //  break;
            case GAME:
                game_loop();
                break;
            //case OPTION:
            //  option_loop();
            //  break;
            //default:
            //  menu_loop();
        }

    /*
        if(SYS_getFPS()<60)
        {
            sprintf(debug_string,"FPS:%ld", SYS_getFPS());
            VDP_drawText(debug_string,DEBUG_STR_X,0);
        }
        else VDP_clearText(DEBUG_STR_X,0,8);
    */

       //SYS_doVBlankProcess();
    }

    return 0;
}



