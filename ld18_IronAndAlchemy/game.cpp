#include "game.h"
#include "player.h"
#include "physics.h"

#include <luddite/resource.h>
#include <luddite/texture.h>
#include <luddite/random.h>

#include <luddite/tweakval.h>


//#include <luddite/foreach.h>
//#define foreach BOOST_FOREACH
#define foreach(type, it, L) \
	for (type::iterator (it) = (L).begin(); (it) != (L).end(); ++(it))

using namespace Luddite;

void IronAndAlchemyGame::initResources()
{
	// Init the texture db
	TextureDB *db = new TextureDB();
	TextureDB &texDB = TextureDB::singleton();

	// Init the fonts
	HTexture hFontTexture = texDB.getTexture("gamedata/digistrip.png") ;

    GLuint texId = texDB.getTextureId( hFontTexture );
    m_font20 = ::makeFont_Digistrip_20( texId );
    m_font32 = ::makeFont_Digistrip_32( texId );

	// Init the sprite textures & sprites	
	m_sbPlayer = makeSpriteBuff( "gamedata/player_bad.png") ;
	
	Sprite *spr = m_sbPlayer->makeSprite( 0.0, 0.0, 0.25, 0.5 );
    spr->sx = 16; spr->sy = 16;
	spr->x = 120;
	spr->y = 80;
	spr->update();

	// Create the player
	m_player = new Entity( spr );
	m_player->addBehavior( new Physics( m_player ) );
	m_playerCtl = new Player( m_player );
	m_player->addBehavior( m_playerCtl );

	m_entities.push_back( m_player );

	// Init enemies
	m_sbEnemies = makeSpriteBuff( "gamedata/enemies_bad.png");

	// Start with the starting map
	m_mapCurr = loadOgmoFile( "gamedata/level_wide.oel" );
}

void IronAndAlchemyGame::freeResources()
{
	// meh... 
	//m_texDB->singleton().freeTexture( hFontTexture );    

    delete m_font20;
    delete m_font32;  
}

SpriteBuff *IronAndAlchemyGame::makeSpriteBuff( const char *filename )
{
	SpriteBuff *sbNew;

	TextureDB &texDB = TextureDB::singleton();
	HTexture hSpriteTex = texDB.getTexture( filename );
	GLuint texId = texDB.getTextureId( hSpriteTex );

	sbNew = new SpriteBuff( texId );
	m_spriteBuffs.push_back( sbNew );

	return sbNew;
}

void IronAndAlchemyGame::updateSim( float dtFixed )
{
	dbgPoints.clear();

	// update entities
	for (std::list<Entity*>::iterator ei = m_entities.begin();
		ei != m_entities.end(); ++ei )
	{
		(*ei)->updateSim( this, dtFixed );		
	}
}

void IronAndAlchemyGame::updateFree( float dt )
{
}

bool IronAndAlchemyGame::onGround( float x, float y )
{
#if 0
	// TODO: do for real
	int ix, iy;
	ix = (int)x; 
	iy = (int)y+1; // 1px below

	if (m_mapCurr->solid( ix/8, iy/8 ))
	{
		return true;
	}
#endif

	if (y < 9) return true;
	return false;
}

bool IronAndAlchemyGame::collideWorld( Sprite *spr )
{
	// TODO: use actual world
	//if ((spr->x < 8) || (spr->x > 232)) return true;
	//if (spr->y < 16) return true;
	//return false;

	int ix, iy; 
	ix = (int)(spr->x); iy = (int)(spr->y );

	if (_collideWorldPnt( ix, iy - (spr->sy/2)) )
	{
		return true;
	}
	else return false;

#if 0
	// cheat a bit, just check the corner pixels of the sprite
	int ix, iy; 
	ix = (int)(spr->x); iy = (int)(spr->y);

	if (_collideWorldPnt( ix - (spr->sx/2), iy - (spr->sy/2)) ||
		_collideWorldPnt( ix + (spr->sx/2), iy - (spr->sy/2)) ||
		_collideWorldPnt( ix + (spr->sx/2), iy + (spr->sy/2) ) ||
		_collideWorldPnt( ix - (spr->sx/2), iy + (spr->sy/2) ) )
	{
		return true;
	}
	else return false;
#endif
}

bool IronAndAlchemyGame::_collideWorldPnt( int x, int y )
{
	// DBG
	// ground is always solid
	if (y < 0) return true;
	if ((x < 0) || (x>=m_mapCurr->m_width * 8)) return true;
	
	// check against map
	bool result = m_mapCurr->solid( x/8, y/8 );	
	dbgPoints.push_back( DbgPoint(x, y, result?1.0:0.0, result?0.0:0.0, 0.0) );	

	return result;
}

Entity *IronAndAlchemyGame::makeEnemy( int type, float x, float y )
{
	// Make a sprite
	Sprite *spr = m_sbEnemies->makeSprite( type*0.2, 0.0, (type+1)*0.2, 1.0 ); // fixme
    spr->sx = 16; spr->sy = 16;
	spr->x = x; spr->y = y;
	spr->update();

	// Create the enemy entity
	Entity *ent = new Entity( spr );
	ent->addBehavior( new Physics( ent ) );
	Enemy *beh_enemy = new Enemy( ent );
	ent->addBehavior( beh_enemy );

	// add to our entities list
	m_entities.push_back( ent );

	return ent;
}

void IronAndAlchemyGame::render()
{
    glClearColor( 0.592f, 0.509f, 0.274f, 1.0f );    
    glClear( GL_COLOR_BUFFER_BIT );

    // set up camera
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();    
    glOrtho( 0, 240, 0, 160, -1.0, 1.0 );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();    

	// translate view
	float view_x = m_playerCtl->m_physics->x - 120;
	float view_y = m_playerCtl->m_physics->y - 60;

	if (view_x < 0) view_x = 0;
	if (view_y < 0) view_y = 0;
	float mapW  = (m_mapCurr->m_width*8) - 240;
	float mapH  = (m_mapCurr->m_height*8) - _TV(160);
	if (view_x > mapW) view_x = mapW;
	if (view_y > mapH) view_y = mapH;
	
	DBG::info("view %3.2f %3.2f\n", view_x, view_y );

	glTranslated( -view_x, -view_y, 0.0 );

    // do text
    glEnable( GL_BLEND );
    glEnable( GL_TEXTURE_2D );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );    

	// draw the map
	m_mapCurr->renderAll();

#if 0
	if ( onGround( m_playerCtl->m_physics->x,
				   m_playerCtl->m_physics->y ) )
	{
		m_font20->setColor( 0.0f, 1.0f, 0.0f, 1.0f );    
		m_font20->drawString( 10, 130, "Ground" );    
	}
	else
	{
		m_font20->setColor( 1.0f, 1.0f, 0.0f, 1.0f );    
		m_font20->drawString( 10, 130, "NO GRND" );    
	}
#endif

	// draw the sprites
	foreach( std::list<SpriteBuff*>, sbi, m_spriteBuffs )
	{
		(*sbi)->renderAll();
	}

	// draw the dbg points
	glDisable( GL_TEXTURE );
	glDisable( GL_BLEND );
	glBegin( GL_POINTS );
	//DBG::info("%d debug points\n", dbgPoints.size() );
	for (int i=0; i < dbgPoints.size(); i++)
	{
		DbgPoint &p = dbgPoints[i];
		glColor3f( p.r, p.g, p.b );
		glVertex3f( p.x, p.y, 0.01 );
	}
	glEnd();
	glColor3f( 1.0, 1.0, 1.0 );

    // actually draw the text
    m_font20->renderAll();
    m_font32->renderAll();

    m_font32->clear();
    m_font20->clear();    
}


void IronAndAlchemyGame::updateButtons( unsigned int btnMask )
{
	// Movment buttons
	if ((btnMask & BTN_LEFT) && (!(btnMask & BTN_RIGHT)) )
	{
		m_playerCtl->ix = -1;
	}
	else if ((btnMask & BTN_RIGHT) && (!(btnMask & BTN_LEFT)) )
	{
		m_playerCtl->ix = 1;
	}
	else
	{
		m_playerCtl->ix = 0;
	}

	// other buttons
	m_playerCtl->m_jumpPressed = ((bool)(btnMask & BTN_JUMP));
	m_playerCtl->m_firePressed = ((bool)(btnMask & BTN_FIRE));
}
	
// key "events"
void IronAndAlchemyGame::buttonPressed( unsigned int btn )
{
	switch (btn)
	{
		case BTN_JUMP:
			m_playerCtl->jump( this );
			break;

		case BTN_FIRE:
			// todo
			// TMP for testing
			{
				Entity *baddy = makeEnemy( Enemy_REDBUG, randUniform( 10, 200 ), 100 );
			}
			break;
	}
}