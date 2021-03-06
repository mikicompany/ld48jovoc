#ifndef ENEMY_H
#define ENEMY_H

#include "behavior.h"

enum
{
	Enemy_REDBUG,
	Enemy_GREENBUG,
	Enemy_BLUEBUG,
	Enemy_SNOUTY,
	
	Enemy_SHOOTY_N,
	Enemy_SHOOTY_S,
	Enemy_SHOOTY_E,
	Enemy_SHOOTY_W,
};

class IronAndAlchemyGame;
class Entity;
class Physics;

class Enemy : public Behavior 
{
public:
	Enemy( Entity *owner );

	// enemy movement
	virtual void movement( IronAndAlchemyGame *game, float dtFixed );
	virtual void collideWorld( IronAndAlchemyGame *game );

	Physics *m_physics;
	int m_walkDir;
};

#endif