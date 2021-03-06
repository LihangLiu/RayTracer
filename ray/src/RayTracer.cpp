// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>

extern TraceUI* traceUI;

#include <iostream>
#include <fstream>

using namespace std;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
bool debugMode = false;

// used for super sampling
float RandomFloat(float a, float b) {
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates (x,y),
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.

Vec3d RayTracer::trace(double x, double y)
{
  // Clear out the ray cache in the scene for debugging purposes,
  if (TraceUI::m_debug) scene->intersectCache.clear();
  ray r(Vec3d(0,0,0), Vec3d(0,0,0), ray::VISIBILITY);
  scene->getCamera().rayThrough(x,y,r);
  Vec3d ret = traceRay(r, traceUI->getDepth(), Vec3d(1,1,1));
  ret.clamp();
  return ret;
}

Vec3d RayTracer::tracePixel(int i, int j)
{
	Vec3d col(0,0,0);

	if( ! sceneLoaded() ) return col;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);
	double d_x = 0.5/double(buffer_width);
	double d_y = 0.5/double(buffer_height);

	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;

	int sampNum = traceUI->getSuperSamplingNum();
	if (sampNum == 1) {
		col = trace(x,y);
	} else {
		double new_x, new_y;
		for (int s=0;s<sampNum; ++s) {
			new_x = RandomFloat(x-d_x,x+d_x);
			new_y = RandomFloat(y-d_y,y+d_y);
			col += trace(new_x, new_y);
		}
		col /= sampNum;
	}

	pixel[0] = (int)( 255.0 * col[0]);
	pixel[1] = (int)( 255.0 * col[1]);
	pixel[2] = (int)( 255.0 * col[2]);
	return col;
}


// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
// depth: Max depth of recursion
Vec3d RayTracer::traceRay(ray& r, int depth, Vec3d last_factor)
{
	// check recursion depth
	if (depth < 0)
		return Vec3d(0.0,0.0,0.0);

	isect i;
	Vec3d I;

	if(scene->intersect(r, i)) {
		// YOUR CODE HERE

		// An intersection occurred!  We've got work to do.  For now,
		// this code gets the material for the surface that was intersected,
		// and asks that material to provide a color for the ray.  

		// This is a great place to insert code for recursive ray tracing.
		// Instead of just returning the result of shade(), add some
		// more steps: add in the contributions from reflected and refracted
		// rays.

		// shade model
	  	const Material& m = i.getMaterial();
	  	I = m.shade(scene, r, i);

	  	// reflection model
	  	Vec3d Q = r.at(i.t);
	  	Vec3d N = i.N;
	  	Vec3d V = -r.d;
		Vec3d R = 2*N*(N*V)-V;
	  	if (!m.kr(i).iszero()) {
	  		Vec3d cur_factor = prod(m.kr(i), last_factor);
	  		if (cur_factor.length() < traceUI->getTermThres()*0.001) {
	  			
	  		} else {
		  		ray r_2nd(Q, R, ray::REFLECTION);
		  		I += prod(m.kr(i), traceRay(r_2nd, depth-1, cur_factor));
		  	}
	  	}

	  	// refraction model
	  	if (!m.kt(i).iszero()) {
	  		double index;
	  		// inside or outside
	  		if (V*N>0) 
	  			index = 1.0/m.index(i);
	  		else {
	  			index = m.index(i);
	  			N = -N;
	  		}

	  		Vec3d S_i = N*(N*V) - V;
	  		Vec3d S_t = index*S_i;
	  		// Check full reflection
	  		if ((1.0-S_t*S_t) > 0) {
	  			Vec3d cur_factor = prod(m.kr(i), last_factor);
		  		if (cur_factor.length() < traceUI->getTermThres()*0.001) {

		  		} else {

		  			Vec3d T = S_t - N*sqrt(1.0-S_t*S_t);
		  			ray r_2nd(Q,T,ray::REFRACTION);
		  			I += prod(m.kt(i), traceRay(r_2nd, depth-1, cur_factor));		
		  		}
	  		}
	  	}

	} else {
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.
		I = Vec3d(0.0, 0.0, 0.0);
		if (haveCubeMap() && traceUI->isUsingCubeMap()) {
			CubeMap* cubemap = getCubeMap();
			I = cubemap->getColor(r);
		}
	}
	return I;
}

RayTracer::RayTracer()
	: scene(0), buffer(0), buffer_width(256), buffer_height(256), m_bBufferReady(false)
{}

RayTracer::~RayTracer()
{
	delete scene;
	delete [] buffer;
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer;
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene( char* fn ) {
	ifstream ifs( fn );
	if( !ifs ) {
		string msg( "Error: couldn't read scene file " );
		msg.append( fn );
		traceUI->alert( msg );
		return false;
	}
	
	// Strip off filename, leaving only the path:
	string path( fn );
	if( path.find_last_of( "\\/" ) == string::npos ) path = ".";
	else path = path.substr(0, path.find_last_of( "\\/" ));

	// Call this with 'true' for debug output from the tokenizer
	Tokenizer tokenizer( ifs, false );
    Parser parser( tokenizer, path );
	try {
		delete scene;
		scene = 0;
		scene = parser.parseScene();
	} 
	catch( SyntaxErrorException& pe ) {
		traceUI->alert( pe.formattedMessage() );
		return false;
	}
	catch( ParserException& pe ) {
		string msg( "Parser: fatal exception " );
		msg.append( pe.message() );
		traceUI->alert( msg );
		return false;
	}
	catch( TextureMapException e ) {
		string msg( "Texture mapping exception: " );
		msg.append( e.message() );
		traceUI->alert( msg );
		return false;
	}

	if( !sceneLoaded() ) return false;

	// build kdtree
	scene->buildKdTree();

	return true;
}

// void RayTracer::traceSetup(int w, int h)
// {
// 	if (buffer_width != w || buffer_height != h)
// 	{
// 		buffer_width = w;
// 		buffer_height = h;
// 		bufferSize = buffer_width * buffer_height * 3;
// 		delete[] buffer;
// 		buffer = new unsigned char[bufferSize];
// 	}
// 	memset(buffer, 0, w*h*3);
// 	m_bBufferReady = true;
// }

void RayTracer::traceSetup(int w, int h)
{
	if (buffer_width != w || buffer_height != h)
	{
		buffer_width = w;
		buffer_height = h;
		bufferSize = buffer_width * buffer_height * 3;
		delete[] buffer;
		buffer = new unsigned char[bufferSize];
	}
	// memset(buffer, 0, w*h*3);
	m_bBufferReady = true;
}


