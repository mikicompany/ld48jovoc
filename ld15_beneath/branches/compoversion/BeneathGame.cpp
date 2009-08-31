#include <map>

#include <tweakval.h>
#include <gamefontgl.h>

#include <Common.h>
#include <BeneathGame.h>

#include <TinyXml.h>

BeneathGame::BeneathGame() :
	m_isInit( false ),
	m_editor( NULL ),
	m_gameState( GameState_MENU ),
	m_playtest( false ),
	m_level( NULL ),
	m_vel( 0.0f, 0.0f )
{
}


void BeneathGame::update( float dt )
{
	if (m_gameState==GameState_GAME)
	{
		game_update( dt );
	}
	else if (m_gameState==GameState_EDITOR)
	{
		if (m_editor) m_editor->update( dt );
	}
}

void BeneathGame::updateSim( float dt )
{
	if (m_gameState==GameState_GAME)
	{
		game_updateSim( dt );
	}
}

void BeneathGame::game_updateSim( float dt )
{
	vec2f thrustDir( 0.0f, 0.0f );
	float rotateAmt = 0.0;
	const float LATERAL_THRUST_AMT = _TV(1.0f);
	const float FWD_THRUST_AMT = _TV(1.0f);
	const float REV_THRUST_AMT = _TV(0.2f);
	const float THRUST_AMT = _TV(1500.0f);
	const float ROTATE_AMT = _TV(400.0f);
	const float kDRAG = _TV(0.01f);
	const float kDRAG2 = _TV(0.002f);

	// Continuous (key state) keys
	Uint8 *keyState = SDL_GetKeyState( NULL );

	// rotation
	if ( ((keyState[SDLK_LEFT]) && (!keyState[SDLK_RIGHT])) ||
		 ((keyState[SDLK_a]) && (!keyState[SDLK_d])) )
	{
		rotateAmt = -ROTATE_AMT;
	}
	else if ( ((!keyState[SDLK_LEFT]) && (keyState[SDLK_RIGHT])) ||
		     ((!keyState[SDLK_a]) && (keyState[SDLK_d])) )
	{
		rotateAmt = ROTATE_AMT;
	}

	// lateral thrust
	if ((keyState[SDLK_q]) && (!keyState[SDLK_e]))		 
	{
		thrustDir.x = -LATERAL_THRUST_AMT;
	}
	else if ((!keyState[SDLK_q]) && (keyState[SDLK_e]))		     
	{
		thrustDir.x = LATERAL_THRUST_AMT;
	}

	// forward/backward
	if ( ((keyState[SDLK_UP]) && (!keyState[SDLK_DOWN])) ||
		 ((keyState[SDLK_w]) && (!keyState[SDLK_s])) )
	{
		thrustDir.y = FWD_THRUST_AMT;
	}
	else if ( ((!keyState[SDLK_UP]) && (keyState[SDLK_DOWN])) ||
				((!keyState[SDLK_w]) && (keyState[SDLK_s])) )
	{
		thrustDir.y = -REV_THRUST_AMT;
	}

	m_player->angle += rotateAmt *dt;

	// rotate thrust dir
	thrustDir = RotateZ( thrustDir, -(float)(m_player->angle * D2R) );


	// calc drag
	float l = Length( m_vel );
	if (l > 0.1)
	{
		vec2f dragDir = -m_vel;		
		
		float kDrag = kDRAG * l + kDRAG2 * l * l;		
		dragDir.Normalize();
		dragDir *= kDrag;

		m_vel += dragDir * dt;
	}
	else
	{
		m_vel = vec2f( 0.0f, 0.0f );
	}

	// update player
	m_vel += thrustDir * THRUST_AMT * dt;

	// Check for collisions	
	vec2f newPos = m_player->pos + m_vel * dt;
	bool collision = false;
	if (m_level)
	{
		for (int i=0; i < m_level->m_collision.size(); i++)
		{
			Segment &s = m_level->m_collision[i];
			vec2f p;
			float d = distPointLine( s.a, s.b, newPos, p );						
			
			if (d < m_player->m_size.x * 0.7)
			{
				// collide
				if (s.segType == SegType_COLLIDE)
				{

					//printf("COLLISION: dist %f player %3.2f %3.2f seg %3.2f %3.2f -> %3.2f %3.2f\n", 
					//	d, m_player->pos.x, m_player->pos.y,
					//	s.a.x, s.a.y, s.b.x, s.b.y
					//	);

					collision = true;
					m_vel =- m_vel;
					break;
				}
				else if (s.segType == SegType_KILL)
				{
					// TODO: reset level properly
					m_vel = vec2f( 0.0f, 0.0f );					
					m_player->pos = m_level->m_spawnPoint;		
					collision = true;
				}
				// TODO: Dialoge
			}
		}
	}	

	if (!collision)
	{
		m_player->pos = newPos;
	}
}

// "as fast as possible" update for effects and stuff
void BeneathGame::game_update( float dt )
{
//	printf("Update %3.2f\n", dt );
}

void BeneathGame::init()
{
	// Load the font image
	ILuint ilFontId;
	ilGenImages( 1, &ilFontId );
	ilBindImage( ilFontId );		

	//glEnable( GL_DEPTH_TEST );
	glDisable( GL_DEPTH_TEST );
	
	if (!ilLoadImage( (ILstring)"gamedata/andelemo.png" )) {
		printf("Loading font image failed\n");
	}
	
	// Make a GL texture for it
	m_glFontTexId = ilutGLBindTexImage();
	m_fntFontId = gfCreateFont( m_glFontTexId );

	// A .finfo file contains the metrics for a font. These
	// are generated by the Fontpack utility.
	gfLoadFontMetrics( m_fntFontId, "gamedata/andelemo.finfo");

	//printf("font has %d chars\n", 
	//	gfGetFontMetric( m_fntFontId, GF_FONT_NUMCHARS ) );					


	// load all shapes
	loadShapes( "gamedata/shapes_Test.xml" );

	// Now load game shapes
	m_player = Shape::simpleShape( "gamedata/player.png" );
	m_player->pos = vec2f( 300, 200 );
}
	
void BeneathGame::redraw()
{	
	if (!m_isInit)
	{
		m_isInit = true;
		init();
	}
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	pseudoOrtho2D( 0, 800, 0, 600 ) ;


	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	if (m_gameState == GameState_GAME )
	{
		game_redraw();
	}
	else if (m_gameState == GameState_EDITOR )
	{
		if (m_editor)
		{
			m_editor->redraw();
		}
	}
	else if (m_gameState == GameState_MENU )
	{
		glClearColor( _TV( 0.1f ), _TV(0.2f), _TV( 0.4f ), 1.0 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		
		// Title text
		gfEnableFont( m_fntFontId, 32 );	
		gfBeginText();
		glTranslated( _TV(260), _TV(500), 0 );
		gfDrawString( "LD15 Cavern Game" );
		gfEndText();

		// Bottom text
		gfEnableFont( m_fntFontId, 20 );	
		gfBeginText();
		glTranslated( _TV(180), _TV(10), 0 );
		gfDrawString( "LudumDare 15 - Joel Davis (joeld42@yahoo.com)" );
		gfEndText();
	}
}

void BeneathGame::game_redraw()
{
	glClearColor( _TV( 0.2f ), _TV(0.2f), _TV( 0.3f ), 1.0 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	vec2f gameview(m_player->pos.x - 400, m_player->pos.y - 300 );
	pseudoOrtho2D( gameview.x, gameview.x + 800,
					gameview.y, gameview.y + 600 );

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	// draw level
	m_level->draw();

	// Draw player shape
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );		

	m_player->drawBraindead();

	// debug collsion stuff
#if 0
	if (m_level)
	{
		glDisable( GL_TEXTURE_2D );
		glBegin( GL_LINES );
		for (int i=0; i < m_level->m_collision.size(); i++)
		{
			Segment &s = m_level->m_collision[i];
			glColor3f( 1.0f, 1.0f, 0.0f );
			glVertex3f( s.a.x, s.a.y, 0.0f );
			glVertex3f( s.b.x, s.b.y, 0.0f );

			// draw distance to seg
			glColor3f( 0.0, 1.0f, 1.0f );
			vec2f p;
			float d = distPointLine( s.a, s.b, m_player->pos.y, p );
			glVertex3f( m_player->pos.x, m_player->pos.y, 0.0 );
			glVertex3f( p.x, p.y, 0.0 );

		}
		glEnd();

		


		// draw player radius
		glColor3f( 1.0f, 0.0f, 1.0f );
		glBegin( GL_LINE_LOOP );
		float rad = m_player->m_size.x * 0.7;
		for (float t=0; t < 2*M_PI; t += (M_PI/10.0) )
		{
			float s = sin(t);
			float c = cos(t);
			glVertex3f( m_player->pos.x + c * rad,
						m_player->pos.y + s * rad, 0.0 );
		}
		glEnd();
	}
#endif
}

void BeneathGame::keypress( SDL_KeyboardEvent &key )
{
	if (m_gameState==GameState_GAME)
	{
		game_keypress( key );
	}
	else if (m_gameState==GameState_EDITOR)
	{
		if (m_editor) 
		{
			if ( (key.keysym.sym == SDLK_RETURN) && (m_editor->m_level))
			{
				// test a copy of level
				m_playtest = true;
				m_level = new Cavern();
				*m_level = *(m_editor->m_level);

				m_player->pos = m_level->m_spawnPoint;
				m_gameState = GameState_GAME;				
			}
			else
			{
				m_editor->keypress( key );		
			}
		}
	}
	else if (m_gameState==GameState_MENU)
	{
		switch( key.keysym.sym )
		{
		case SDLK_SPACE:			
			m_level = new Cavern();
			m_level->loadLevel( "level.xml", m_shapes );
			m_gameState = GameState_GAME;
			m_vel = vec2f( 0.0f, 0.0f );					
			m_player->pos = m_level->m_spawnPoint;		
			break;

		case SDLK_F8:
			{
				// switch to editor
				m_gameState = GameState_EDITOR;
				if (!m_editor)
				{
					m_editor = new Editor( m_fntFontId, m_shapes );					
				}
			}
			break;
		}
	}
}

void BeneathGame::mouse( SDL_MouseButtonEvent &mouse )
{
	if (m_gameState == GameState_EDITOR)
	{
		if (m_editor) m_editor->mousepress( mouse );
	}
}

void BeneathGame::game_keypress( SDL_KeyboardEvent &key )
{
	switch (key.keysym.sym)
	{
	case SDLK_F5:
		//DBG: reset start pos
		m_vel = vec2f( 0.0f, 0.0f );
		if (m_level) {
			m_player->pos = m_level->m_spawnPoint;
		}
		else
		{
			m_player->pos = vec2f( 0.0f, 0.0f );
		}
		break;
	}

}

void BeneathGame::loadShapes( const char *filename )
{
	TiXmlDocument *xmlDoc = new TiXmlDocument( filename );

	if (!xmlDoc->LoadFile() ) {
		printf("ERR! Can't load %s\n", filename );
	}

	TiXmlElement *xShapeList, *xShape;
	//TiXmlNode *xText;

	xShapeList = xmlDoc->FirstChildElement( "ShapeList" );
	assert( xShapeList );

	xShape = xShapeList->FirstChildElement( "Shape" );
	while (xShape) 
	{
		Shape *shp = new Shape();
		
		shp->name = xShape->Attribute("name");
		shp->m_collide = (!stricmp( xShape->Attribute("collide"), "true" ));
		shp->m_pattern = (!stricmp( xShape->Attribute("pattern"), "true" ));
		if (!stricmp( xShape->Attribute("blend"), "true" ))
		{
			shp->blendMode = Blend_NORMAL;
		}
		shp->m_relief = (!stricmp( xShape->Attribute("relief"), "in" ));

		vec2f st0, sz;
		sscanf( xShape->Attribute("rect"), "%f,%f,%f,%f", 
				&(st0.x), &(st0.y),
				&(sz.x), &(sz.y) );		
	
		shp->m_size = sz;
		shp->m_origSize = sz;

		// get texture and adjust sts
		int texw, texh;
		shp->mapname = std::string("gamedata/") + std::string(xShape->Attribute("map"));
		shp->m_texId = getTexture( shp->mapname, &texw, &texh );

		shp->st0 = st0 / (float)texw;
		shp->st1 = (st0 + sz) / (float)texh;

		m_shapes.push_back( shp );
		xShape = xShape->NextSiblingElement( "Shape" );
	}
		
	// done
	xmlDoc->Clear();
	delete xmlDoc;
}


// don't call this every frame or nothin'..
typedef std::map<std::string,GLuint> TextureDB;
TextureDB g_textureDB;
GLuint getTexture( const std::string &filename, int *xsize, int *ysize )
{
	GLuint texId;

	// See if the texture is already loaded
	TextureDB::iterator ti;
	ti = g_textureDB.find( filename );
	if (ti == g_textureDB.end() )
	{
		// Load the font image
		ILuint ilImgId;
		ilGenImages( 1, &ilImgId );
		ilBindImage( ilImgId );		
	
		if (!ilLoadImage( (ILstring)filename.c_str() )) {
			printf("Loading font image failed\n");
		}
//		printf("Loaded Texture %s\n", filename.c_str() );
	
		// Make a GL texture for it
		texId = ilutGLBindTexImage();		

		// and remember it
		g_textureDB[ filename ] = texId;
	}
	else
	{
		// found the texture
		texId = (*ti).second;
	}

	// now get the size if they asked for it
	if ((xsize) && (ysize))
	{
		glBindTexture( GL_TEXTURE_2D, texId );
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, xsize );
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, ysize );
	}

	return texId;
}

void pseudoOrtho2D( double left, double right, double bottom, double top )
{
#if 0
	// for now, real ortho
	gluOrtho2D( left, right, bottom, top );
#else	
	float w2, h2;
	w2 = (right - left) /2;
	h2 = (top - bottom) /2;
	float nv = _TV( 1.0f );
	glFrustum( -w2, w2, -h2, h2, nv, _TV(5.0f) );
	gluLookAt( left + w2, bottom + h2, nv,
			   left + w2, bottom + h2, 0.0,
			   0.0, 1.0, 0.0 );
#endif
}

float distPointLine( const vec2f &a, const vec2f &b, const vec2f &c, vec2f &p )
{
	vec2f ab = b-a;
	float lmag2 = LengthSquared(ab);
	float r = ((c.x - a.x)*(b.x-a.x) + (c.y-a.y)*(b.y-a.y)) / lmag2;

	if (r < 0.0f )
	{
		p = a;
		return Length(a-c);
	}
	else if (r > 1.0f)
	{
		p = b;
		return Length(b-c);
	}
	else
	{
		p=a + r * ab;
		return Length( c-p );
	}
}