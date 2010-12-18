#include <stdio.h>
#include <stdlib.h>
#include <string>

#ifdef WIN32
# define WIN32_LEAN_AND_MEAN
# include <winsock2.h>
# include <windows.h>
# include <crtdbg.h>
#endif

#include <luddite/GLee.h>
#include <GL/gl.h>
#include <GL/glu.h>

// Luddite tools
#include <luddite/luddite.h>
#include <luddite/debug.h>
#include <luddite/texture.h>
#include <luddite/font.h>
#include <luddite/avar.h>

#include <SDL.h>
#include <SDL_endian.h>

#include "bonsai.h"

#include "glsw.h"

#include "PVRT/PVRTVector.h"
#include "PVRT/PVRTMatrix.h"
#include "PVRT/PVRTQuaternion.h"

// Stupid X11 uses 'Font' too
using namespace Luddite;

// 30 ticks per sim frame
#define STEPTIME (33)

#ifndef WIN32
#define _stricmp strcasecmp
#endif

// ===========================================================================
// Global resources
TextureDB g_texDB;

HTexture hFontTexture;

// Could use a HandleMgr and HFont for fonts too, but since there will
// be just one just do directly
Luddite::Font *g_font20 = NULL;
Luddite::Font *g_font32 = NULL;

USE_AVAR( float );
AnimFloat g_textX;

Bonsai *g_treeLand;

PVRTMat4 matModelview;
PVRTMat4 matProjection;
PVRTMat4 matMVP;

PVRTQUATERNION quatCam;

// ===========================================================================
GLuint compileShader( const char *shaderText, GLenum shaderType )
{
    GLint status;    
    GLuint shader;
    shader = glCreateShader( shaderType );
    
    // compile the shader
    glShaderSource( shader, 1, &shaderText, NULL );
    glCompileShader( shader );
    
    // Check for errors
    GLint logLength;
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logLength );
	DBG::info( "Loglength %d >0 %s\n", logLength, (logLength>0)?"true":"false" );

    if (logLength > 1)
    {
        char *log = (char *)malloc(logLength);
        glGetShaderInfoLog( shader, logLength, &logLength, log );		

        DBG::error("Error compiling shader:\n'%s'\n", log );
        free(log);        
    }
    
    glGetShaderiv( shader,GL_COMPILE_STATUS, &status );
    if (status==0)
    {
        glDeleteShader( shader );
        // TODO: throw exception
        DBG::warn("Compile status is bad\n" );
        
        return 0;        
    }

    return shader;    
}

// ===========================================================================
void printShaderLog( GLuint program )
{
	GLint logLength;
    glGetProgramiv( program, GL_INFO_LOG_LENGTH, &logLength );
    if (logLength > 0 )
    {
        char *log = (char*)malloc(logLength);
        glGetProgramInfoLog( program, logLength, &logLength, log );
        DBG::info ("Link Log:\n%s\n", log );
        free(log);        
    }
	
}

// ===========================================================================
void link(GLuint program )
{
    GLint status;
    
    glLinkProgram( program );
	
	printShaderLog( program );
	
    glGetProgramiv( program, GL_LINK_STATUS, &status);
    if (status==0)
    {
		DBG::error("ERROR Linking shader\n");        
    }    
}


// ===========================================================================
bool loadShader( const char *shaderKey, GLuint &program )
{
	DBG::info("loadShader %s ...\n", shaderKey );
	
    // Create shader program
    program = glCreateProgram();

    // vertex and fragment shaders
    GLuint vertShader, fragShader;    

    // Get the vertex shader part
    char fullShaderKey[256];
    sprintf(fullShaderKey, "%s.Vertex", shaderKey );    
    const char *vertShaderText = glswGetShader( fullShaderKey );
    if (!vertShaderText)
    {
		DBG::warn("Couldn't find shader key: %s\n", fullShaderKey );
		return false;
    }

    // Compile the vertex shader
    vertShader = compileShader( vertShaderText, GL_VERTEX_SHADER );

    // Get fragment shader
    sprintf(fullShaderKey, "%s.Fragment", shaderKey );    
    const char *fragShaderText = glswGetShader( fullShaderKey );
    if (!fragShaderText)
    {
        DBG::warn("Couldn't find shader key: %s\n", fullShaderKey );
		return false;
    }

    DBG::info( "compile fragment shader\n" );
    
    // Compile the fragment shader
    fragShader = compileShader( fragShaderText, GL_FRAGMENT_SHADER );

    // Attach shaders
    glAttachShader( program, vertShader );
    glAttachShader( program, fragShader );

    DBG::info("... bind attrs\n" );
	
	// Bind Attrs (todo put in subclass)
	// FIXME: some shaders dont have all these attrs..
	glBindAttribLocation( program, Attrib_POSITION, "position" );
	glBindAttribLocation( program, Attrib_TEXCOORD, "texcoord" );
	glBindAttribLocation( program, Attrib_NORMAL,   "normal" );    
	glBindAttribLocation( program, Attrib_COLOR,   "color" );
	
    
    //  Link Shader
    DBG::info("... links shaders\n" );
    link( program );
    
    // Release vert and frag shaders
    glDeleteShader( vertShader );
    glDeleteShader( fragShader );    

	
	// print shader params
	DBG::info(" ----- %s ------\n", shaderKey );
	int activeUniforms;
	glGetProgramiv( program, GL_ACTIVE_UNIFORMS, &activeUniforms );
	
	DBG::info(" Active Uniforms: %d\n", activeUniforms );
	
    return true;    
}


// ===========================================================================
void game_init()
{
    DBG::info( "Game init\n" );    
    hFontTexture = g_texDB.getTexture("gamedata/digistrip.png") ;

    GLuint texId = g_texDB.getTextureId( hFontTexture );
    g_font20 = ::makeFont_Digistrip_20( texId );
    g_font32 = ::makeFont_Digistrip_32( texId );

    // set up the pulse Avar
    g_textX.pulse( 0, 800 - g_font32->calcWidth( "HELLO" ), 10.0, 0.0 );    

	// set up shaders
	glswInit();
	glswSetPath( "gamedata/", ".glsl" );

	PVRTMatrixQuaternionIdentity( quatCam );

}

// ===========================================================================
// update on a fixed sim step, do any updates that may even remotely
// effect gameplay here
void game_updateSim( float dtFixed )
{
	// rotate
	PVRTQUATERNION qrot;
	PVRTMatrixQuaternionRotationAxis( qrot, PVRTVec3( 0.0, 1.0, 0.0 ), 20.0*(M_PI/180.0)*dtFixed );
	PVRTMatrixQuaternionMultiply( quatCam, quatCam, qrot );

    // Updates all avars
    AnimFloat::updateAvars( dtFixed );    
}

// Free running update, useful for stuff like particles or something
void game_updateFree( float dt )
{
    // do nothing...
}


// ===========================================================================
void game_shutdown()
{
    DBG::info( "Game shutdown\n" );
    
    g_texDB.freeTexture( hFontTexture );    

    delete g_font20;
    delete g_font32;    
}

// ===========================================================================
void game_redraw()
{
    glClearColor( 0.592f, 0.509f, 0.274f, 1.0f );    
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// set up the camera
	float aspect = 800.0 / 600.0;
	PVRTMatrixPerspectiveFovLH( matProjection, 45.0, aspect,
								0.01f, 1000.0f, false );

	glMatrixMode( GL_PROJECTION );
    glLoadIdentity();        

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();    

	PVRTMat4 m;
	PVRTMatrixIdentity( matModelview );

	//PVRTMatrixRotationY( m, 20.0 );
	//PVRTMatrixMultiply( matModelview, matModelview, m );
	PVRTMatrixRotationQuaternion( m, quatCam );
	PVRTMatrixMultiply( matModelview, matModelview, m );	

	PVRTMatrixTranslation( m, 0.0, 0.0, 5.0 );
	PVRTMatrixMultiply( matModelview, matModelview, m );

	PVRTMatrixMultiply( matMVP, matModelview, matProjection );

	//DBG::info( "m[0] %f %f %f\n", matMVP[0][0], matMVP[0][1], matMVP[0][2] );
	g_treeLand->setCamera( matMVP );
	
	// draw land thinggy
	g_treeLand->renderAll();

    // set up camera
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();    
    glOrtho( 0, 800, 0, 600, -1.0, 1.0 );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();    

    // do text
    glEnable( GL_BLEND );
    glEnable( GL_TEXTURE_2D );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );    

    g_font32->setColor( 1.0f, 1.0f, 1.0f, 1.0f );    
    g_font32->drawString( g_textX.animValue(), 300, "HELLO" );    

    // actually draw the text
    g_font20->renderAll();
    g_font32->renderAll();

    g_font32->clear();
    g_font20->clear();    
}


// ===========================================================================
int main( int argc, char *argv[] )
{	

	// I can't live without my precious printf's
#ifdef WIN32
#  ifndef NDEBUG
	AllocConsole();
	SetConsoleTitle( L"ld19 Discovery CONSOLE" );
	freopen("CONOUT$", "w", stdout );
#  endif
#endif

	// Test debug stuff
	DBG::info( "Test of dbg info. Hello %s\n", "world" );
	// Initialize SDL
	if (SDL_Init( SDL_INIT_NOPARACHUTE | SDL_INIT_VIDEO ) < 0 ) 
	{
		DBG::error("Unable to init SDL: %s\n", SDL_GetError() );
		exit(1);
	}

	// cheezy check for fullscreen
	Uint32 mode_flags = SDL_OPENGL;
	for (int i=1; i < argc; i++)
	{
		if (!_stricmp( argv[i], "-fullscreen"))
		{
			mode_flags |= SDL_FULLSCREEN;
		}
	}

	if (SDL_SetVideoMode( 800, 600, 32, mode_flags ) == 0 ) 
	{
		DBG::error( "Unable to set video mode: %s\n", SDL_GetError() ) ;
		exit(1);
	}
		
	SDL_WM_SetCaption( "LD19 Jovoc - Discovery", NULL );

    // Initialize resources
    game_init();    
    atexit( game_shutdown );  

    // init graphics
    glViewport( 0, 0, 800, 600 );

	// Build a treeland thinggy
	g_treeLand = new Bonsai();
	g_treeLand->init();

	g_treeLand->buildAll();

	//=====[ Main loop ]======
	bool done = false;
	Uint32 ticks = SDL_GetTicks(), ticks_elapsed, sim_ticks = 0;	
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

						case SDLK_F6:
							g_texDB.reportUsage();
							break;
					}

					// let the game handle it
					//game->keypress( event.key );
					break;

				case SDL_MOUSEMOTION:					
					break;

				case SDL_MOUSEBUTTONDOWN:					
					//game->mouse( event.button );
					break;

				case SDL_MOUSEBUTTONUP:					
					//game->mouse( event.button );
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

			//printf("update sim_ticks %d ticks_elapsed %d\n", sim_ticks, ticks_elapsed );
			game_updateSim( (float)STEPTIME / 1000.0f );			
		}	

		// redraw as fast as possible		
		float dtRaw = (float)(ticks_elapsed) / 1000.0f;
				
		game_updateFree( dtRaw ); 
        game_redraw();        

		SDL_GL_SwapBuffers();

		// Call this once a frame if using tweakables
        //ReloadChangedTweakableValues();        
	}


    return 1;    
}



