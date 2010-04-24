// ==================================================================
// Islands -- LD17
// ==================================================================
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <crtdbg.h>
#endif

#include <stdarg.h>

#include <iostream>
#include <fstream>
#include <string>

#include <time.h>

#include <SDL.h>
#include <SDL_endian.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <il/il.h>
#include <il/ilu.h>
#include <il/ilut.h>

#include <prmath/prmath.hpp>

#include "debug.h"
#include "IslandGame.h"

// 30 ticks per sim frame
#define STEPTIME (33)

void errorMessage( const char *msg, ... )
{
	char buff[1024];
	va_list args;
	va_start( args, msg );
	vsprintf( buff, msg, args );
	va_end( args );

	// todo: on linux or other real OS, just printf
	MessageBoxA( NULL, buff, "ld17_island ERROR", MB_OK | MB_ICONSTOP );
}

//============================================================================
int main( int argc, char *argv[] )
{
	// Initialize SDL
	if (SDL_Init( SDL_INIT_NOPARACHUTE | SDL_INIT_VIDEO ) < 0 ) 
	{
		errorMessage( "Unable to init SDL: %s\n", SDL_GetError() );
	}

		// I can't live without my precious printf's
#ifdef WIN32
#  ifndef NDEBUG
	AllocConsole();
	SetConsoleTitle( L"ld17 island demo CONSOLE" );
	freopen("CONOUT$", "w", stdout );
#  endif
#endif

	// todo: make configurable
	//int sx = 1280;
	//int sy = 1024;
	int sx = 800, sy=600;
	bool fullscreen = false;
	Uint32 mode_flags = SDL_OPENGL;
	if (fullscreen) 
	{
		mode_flags |= SDL_FULLSCREEN;
	}

	if (SDL_SetVideoMode( sx, sy, 32, mode_flags ) == 0 ) 
	{		
		errorMessage( SDL_GetError() ) ;
		exit(1);
	}
		
	SDL_WM_SetCaption( "LD17 -- Islands", NULL );

	// initialize DevIL
	ilInit();
	ilutRenderer( ILUT_OPENGL );

	//=====[ Init Game ]======
	DBG::g_verbose_level = DBG::kVerbose_Dbg;
	IslandGame *game = new IslandGame();

	game->loadLevel( "gamedata/testlevel.oel" );
	game->buildMap();

	//=====[ Main loop ]======
	Uint32 ticks = SDL_GetTicks(), ticks_elapsed, sim_ticks = 0;
	bool done = false;
	while(!done)
	{
		SDL_Event event;

		while (SDL_PollEvent( &event ) ) 
		{
			switch (event.type )
			{
				case SDL_KEYDOWN:

					switch( event.key.keysym.sym ) 
					{						
						case SDLK_ESCAPE:
							done = true;
							break;
					}
					break;

				case SDL_QUIT:
					done = true;
					break;
			}
		}

		// Timing
		ticks_elapsed = SDL_GetTicks() - ticks;
		ticks += ticks_elapsed;

		// fixed sim update
		sim_ticks += ticks_elapsed;
		while (sim_ticks > STEPTIME) 
		{
			sim_ticks -= STEPTIME;						

			//game->update( (float)STEPTIME / 1000.0f );
			
		}	

		// redraw as fast as possible		
		float dtRaw = (float)(ticks_elapsed) / 1000.0f;
		
		game->update( dtRaw ); 
		game->redraw(  );
		

		SDL_GL_SwapBuffers();

	}

	SDL_Quit();	

	delete game;

	return 0;
}