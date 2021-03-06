#ifndef EDITOR_H
#define EDITOR_H

#include <Cavern.h>

class Editor
{
public:
	Editor( GLuint fntID, std::vector<Shape*> &shapes );

	void update( float dt );
	void redraw();
	void keypress( SDL_KeyboardEvent &key );
	void mousepress( SDL_MouseButtonEvent &mouse );

	void newLevel( vec2f size );
	void stampActiveShape();	

	enum {
		Tool_SELECT,
		Tool_PLACE
	};

//protected:
	void frameView();

	vec3f segColor( int type );

	int m_tool;
	int m_sort;

	Cavern *m_level;
	bool m_showHelp;
	GLuint m_fntFontId;

	// editor view
	bool m_useEditView;
	vec2f m_viewport, m_viewsize;

	// fixed game size view
	vec2f m_gameview;
	
	// mouse stuff
	vec2f m_mousePos;
	vec2f m_dragStart;
	int currSegType;
	bool drawLine;

	// selected shapes we're currently editing)	
	std::vector<Shape*> m_selShapes;
	
	// all shapes we can pick from
	std::vector<Shape*> &m_shapes;

	size_t m_actShapeIndex;
	Shape *m_activeShape;

	// icons
	Shape *iconSpawnPoint,
		  *iconRedGem, *iconRedGemBig;

	void initEditor();
	bool isInit;
};

#endif