#include <crtdbg.h>

#include <string.h>

#include <vector>
#include <prmath/prmath.hpp>

#include "Mesh.h"

using namespace std;

// Loads an obj file into a display list
GLuint LoadObjFile( const char *objfile, vec3f scale )
{
	vector<vec3f> verts;
	vector<vec2f> sts;
	vector<vec3f> norms;

	GLuint id = glGenLists(1);
	glNewList( id, GL_COMPILE );

	FILE *fp = fopen( objfile, "rt" );
	char line[1024];
	char tok[32];
	while (!feof(fp))
	{
		fgets( line, 1024, fp );
		if (strlen(line) < 2) continue;

		sscanf( line, "%s", tok );
		if (!strcmp(tok, "v" ))
		{
			vec3f v; 
			sscanf( line, "%*s %f %f %f", &(v.x), &(v.y), &(v.z) );

			// uniform scale on load
			v *= scale;

			verts.push_back( v );	
		} 
		else if (!strcmp(tok, "vt" ))
		{		
			vec2f vt;
			sscanf( line, "%*s %f %f", &(vt.x), &(vt.y) );
			sts.push_back( vt );	
		}
		else if (!strcmp(tok, "vn" ))
		{
			vec3f n;
			sscanf( line, "%*s %f %f %f", &(n.x), &(n.y), &(n.z) );
			norms.push_back( n );	
		}
		else if (!strcmp(tok, "f" ))
		{
			//_RPT1( _CRT_WARN, "norms.sizes is %d\n", norms.size() );

			glBegin( GL_POLYGON );
			char *ch = line+2;
			ch = strtok( ch, " " );
			while (ch)
			{
				int vndx, stndx, nndx;
				
				if (strstr( ch, "//" ))
				{
					// Only has two entries, i.e. 4//2
					sscanf( ch, "%d//%d", &vndx, &nndx );
					stndx=1; // dummy
				}
				else
				{
					sscanf( ch, "%d/%d/%d", &vndx, &stndx, &nndx );
				}				
				vndx--; stndx--; nndx--; // obj's start counting at 1

				//_RPT3( _CRT_WARN, "norm %f %f %f\n", norms[nndx].x, norms[nndx].y, norms[nndx].z );
				glNormal3f( norms[nndx].x, norms[nndx].y, norms[nndx].z );
//				glTexCoord2f( sts[stndx].x, sts[stndx].y );
				glVertex3f( verts[vndx].x, verts[vndx].y, verts[vndx].z );

				ch = strtok( NULL, " " );
			}
			glEnd();
		}
	}

	glEndList();
	return id;
}