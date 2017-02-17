//
// GraphicalUI.cpp
//
// Handles FLTK integration and other user interface tasks
//
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <thread>
#include <chrono>

#ifndef COMMAND_LINE_ONLY

#include <FL/fl_ask.H>
#include "debuggingView.h"

#include "GraphicalUI.h"
#include "../RayTracer.h"

#define MAX_INTERVAL 500

#ifdef _WIN32
#define print sprintf_s
#else
#define print sprintf
#endif

using namespace std;

bool GraphicalUI::stopTrace = false;
bool GraphicalUI::doneTrace = true;
GraphicalUI* GraphicalUI::pUI = NULL;
char* GraphicalUI::traceWindowLabel = "Raytraced Image";
bool TraceUI::m_debug = false;

//------------------------------------- Help Functions --------------------------------------------
GraphicalUI* GraphicalUI::whoami(Fl_Menu_* o)	// from menu item back to UI itself
{
	return ((GraphicalUI*)(o->parent()->user_data()));
}

//--------------------------------- Callback Functions --------------------------------------------

void GraphicalUI::cb_load_cubemap(Fl_Menu_* o, void* v) 		// added by lihang liu
{
	pUI = whoami(o);
	pUI->m_cubeMapChooser->show();
}

void GraphicalUI::cb_load_scene(Fl_Menu_* o, void* v) 
{
	pUI = whoami(o);

	static char* lastFile = 0;
	char* newfile = fl_file_chooser("Open Scene?", "*.ray", NULL );

	if (newfile != NULL) {
		char buf[256];		
		if (pUI->raytracer->loadScene(newfile)) {			
			print(buf, "Ray <%s>", newfile);
			stopTracing();	// terminate the previous rendering
		} else print(buf, "Ray <Not Loaded>");		
		pUI->m_mainWindow->label(buf);
		pUI->m_debuggingWindow->m_debuggingView->setDirty();		
		if( lastFile != 0 && strcmp(newfile, lastFile) != 0 )
			pUI->m_debuggingWindow->m_debuggingView->resetCamera();

		pUI->m_debuggingWindow->redraw();
	}
}

void GraphicalUI::cb_save_image(Fl_Menu_* o, void* v) 
{
	pUI = whoami(o);

	char* savefile = fl_file_chooser("Save Image?", "*.bmp", "save.bmp" );
	if (savefile != NULL) {
		pUI->m_traceGlWindow->saveImage(savefile);
	}
}

void GraphicalUI::cb_exit(Fl_Menu_* o, void* v)
{
	pUI = whoami(o);

	// terminate the rendering
	stopTracing();

	pUI->m_traceGlWindow->hide();
	pUI->m_mainWindow->hide();
	pUI->m_debuggingWindow->hide();
	TraceUI::m_debug = false;
}

void GraphicalUI::cb_exit2(Fl_Widget* o, void* v) 
{
	pUI = (GraphicalUI *)(o->user_data());

	// terminate the rendering
	stopTracing();

	pUI->m_traceGlWindow->hide();
	pUI->m_mainWindow->hide();
	pUI->m_debuggingWindow->hide();
	TraceUI::m_debug = false;
}

void GraphicalUI::cb_about(Fl_Menu_* o, void* v) 
{
	fl_message("RayTracer Project for CS384g.");
}

void GraphicalUI::cb_sizeSlides(Fl_Widget* o, void* v)
{
	pUI=(GraphicalUI*)(o->user_data());

	// terminate the rendering so we don't get crashes
	stopTracing();

	pUI->m_nSize=int(((Fl_Slider *)o)->value());
	int width = (int)(pUI->getSize());
	int height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);
	pUI->m_traceGlWindow->resizeWindow(width, height);
}

void GraphicalUI::cb_depthSlides(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_nDepth=int( ((Fl_Slider *)o)->value() ) ;
}

void GraphicalUI::cb_refreshSlides(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->refreshInterval=clock_t(((Fl_Slider *)o)->value()) ;
}

void GraphicalUI::cb_filterSlides(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_nFilterWidth=int( ((Fl_Slider *)o)->value() ) ;
}

void GraphicalUI::cb_cubeMapCheckButton(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_usingCubeMap=int( ((Fl_Check_Button *)o)->value() ) ;
}

void GraphicalUI::cb_kdTreeCheckButton(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_usingKdTree=int( ((Fl_Check_Button *)o)->value() ) ;
}

void GraphicalUI::cb_threadNumSlides(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_nThreadNum=int( ((Fl_Slider *)o)->value() ) ;
}

void GraphicalUI::cb_superSamplingNumSlides(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_nSuperSamplingNum=int( ((Fl_Slider *)o)->value() ) ;
}

void GraphicalUI::cb_termThresSlides(Fl_Widget* o, void* v)
{
	((GraphicalUI*)(o->user_data()))->m_ntermThres=int( ((Fl_Slider *)o)->value() ) ;
}

void GraphicalUI::cb_debuggingDisplayCheckButton(Fl_Widget* o, void* v)
{
	pUI=(GraphicalUI*)(o->user_data());
	pUI->m_displayDebuggingInfo = (((Fl_Check_Button*)o)->value() == 1);
	if (pUI->m_displayDebuggingInfo)
	  {
	    pUI->m_debuggingWindow->show();
	    pUI->m_debug = true;
	  }
	else
	  {
	    pUI->m_debuggingWindow->hide();
	    pUI->m_debug = false;
	  }
}

static int numRunningThread = 0;	// used for updating UI
static int cur_coordinate = 0;

void GraphicalUI::cb_render(Fl_Widget* o, void* v) {
	char buffer[256];

	pUI = (GraphicalUI*)(o->user_data());
	doneTrace = stopTrace = false;
	if (pUI->raytracer->sceneLoaded())
	{
		int width = pUI->getSize();
		int height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);
		int origPixels = width * height;
		pUI->m_traceGlWindow->resizeWindow(width, height);
		pUI->m_traceGlWindow->show();
		pUI->raytracer->traceSetup(width, height);

		// Save the window label
    	const char *old_label = pUI->m_traceGlWindow->label();
    	// start timer
    	chrono::time_point<std::chrono::system_clock> c_start, c_end;
    	c_start = chrono::system_clock::now();

        // start multi thread
        int numThread = pUI->m_nThreadNum;
        numRunningThread = numThread;
		printf("num thread: %d\n", numThread);
		thread myThreads[numThread];

		// init cur_coordinate
		cur_coordinate = 0;

		for (int t=0;t<numThread;++t) {
			myThreads[t] = thread(&GraphicalUI::thread_tracePixel, pUI, numThread, t);
		}
		// update UI here
		while (true) {
			if (stopTrace || numRunningThread==0)
				break;
			// sleep
			std::this_thread::sleep_for (std::chrono::milliseconds(pUI->refreshInterval*100));
			// refresh UI
			c_end = chrono::system_clock::now();
			chrono::duration<double> t = c_end-c_start;
			sprintf(buffer, "(%.2f seconds) %s", t.count(), old_label);
			pUI->m_traceGlWindow->label(buffer);
			pUI->m_traceGlWindow->refresh();
			Fl::check();
			if (Fl::damage()) { Fl::flush(); }
		}
		for (int t=0;t<numThread;++t) {
			myThreads[t].join();
		}
		
		doneTrace = true;
		stopTrace = false;
		// end timer
		c_end = chrono::system_clock::now();
		chrono::duration<double> t = c_end-c_start;
		std::cout << "total time = " << t.count() << " seconds, rays traced = " << width*height << std::endl;

		// Restore the window label
		pUI->m_traceGlWindow->label(old_label);
		pUI->m_traceGlWindow->refresh();
		
	}
}

int GraphicalUI::thread_tracePixel(GraphicalUI* pUI, int numThread, int t) {
	int width = pUI->getSize();
	int height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);

	for (int y = 0; y < height; y++)
	{
		if (y%numThread != t)
			continue;
	    for (int x = 0; x < width; x++)
	    {
	    	if (stopTrace) { return 0; }
			pUI->raytracer->tracePixel(x, y);
			// pUI->m_debuggingWindow->m_debuggingView->setDirty();
	    }
	    if (stopTrace) { return 0; }
	}

	// while (true) {
	// 	int t_coord = (cur_coordinate++);

	// 	int t_width = t_coord % width;
	// 	int t_height = t_coord/width;
	// 	if (t_height >= height) 
	// 		break;
	// 	pUI->raytracer->tracePixel(t_width,t_height);
	// }

	numRunningThread --;
}

void GraphicalUI::cb_stop(Fl_Widget* o, void* v)
{
	pUI = (GraphicalUI*)(o->user_data());
	stopTracing();
}

int GraphicalUI::run()
{
	Fl::visual(FL_DOUBLE|FL_INDEX);

	m_mainWindow->show();

	return Fl::run();
}

void GraphicalUI::alert( const string& msg )
{
	fl_alert( "%s", msg.c_str() );
}

void GraphicalUI::setRayTracer(RayTracer *tracer)
{
	TraceUI::setRayTracer(tracer);
	m_traceGlWindow->setRayTracer(tracer);
	m_debuggingWindow->m_debuggingView->setRayTracer(tracer);
}

// menu definition
Fl_Menu_Item GraphicalUI::menuitems[] = {
	{ "&File", 0, 0, 0, FL_SUBMENU },
	{ "&Load CubeMap...",	FL_ALT + 'c', (Fl_Callback *)GraphicalUI::cb_load_cubemap },
	{ "&Load Scene...",	FL_ALT + 'l', (Fl_Callback *)GraphicalUI::cb_load_scene },
	{ "&Save Image...", FL_ALT + 's', (Fl_Callback *)GraphicalUI::cb_save_image },
	{ "&Exit", FL_ALT + 'e', (Fl_Callback *)GraphicalUI::cb_exit },
	{ 0 },

	{ "&Help",		0, 0, 0, FL_SUBMENU },
	{ "&About",	FL_ALT + 'a', (Fl_Callback *)GraphicalUI::cb_about },
	{ 0 },

	{ 0 }
};

void GraphicalUI::stopTracing()
{
	stopTrace = true;
}

GraphicalUI::GraphicalUI() : refreshInterval(10) {
	// init.
	m_mainWindow = new Fl_Window(100, 40, 450, 459, "Ray <Not Loaded>");
	m_mainWindow->user_data((void*)(this));	// record self to be used by static callback functions
	// install menu bar
	m_menubar = new Fl_Menu_Bar(0, 0, 440, 25);
	m_menubar->menu(menuitems);

	// set up "render" button
	m_renderButton = new Fl_Button(360, 37, 70, 25, "&Render");
	m_renderButton->user_data((void*)(this));
	m_renderButton->callback(cb_render);

	// set up "stop" button
	m_stopButton = new Fl_Button(360, 65, 70, 25, "&Stop");
	m_stopButton->user_data((void*)(this));
	m_stopButton->callback(cb_stop);

	// install depth slider
	m_depthSlider = new Fl_Value_Slider(10, 40, 180, 20, "Recursion Depth");
	m_depthSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_depthSlider->type(FL_HOR_NICE_SLIDER);
	m_depthSlider->labelfont(FL_COURIER);
	m_depthSlider->labelsize(12);
	m_depthSlider->minimum(0);
	m_depthSlider->maximum(10);
	m_depthSlider->step(1);
	m_depthSlider->value(m_nDepth);
	m_depthSlider->align(FL_ALIGN_RIGHT);
	m_depthSlider->callback(cb_depthSlides);

	// install size slider
	m_sizeSlider = new Fl_Value_Slider(10, 65, 180, 20, "Screen Size");
	m_sizeSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_sizeSlider->type(FL_HOR_NICE_SLIDER);
	m_sizeSlider->labelfont(FL_COURIER);
	m_sizeSlider->labelsize(12);
	m_sizeSlider->minimum(64);
	m_sizeSlider->maximum(1024);
	m_sizeSlider->step(2);
	m_sizeSlider->value(m_nSize);
	m_sizeSlider->align(FL_ALIGN_RIGHT);
	m_sizeSlider->callback(cb_sizeSlides);

	// install refresh interval slider
	m_refreshSlider = new Fl_Value_Slider(10, 90, 180, 20, "Screen Refresh Interval (0.1 sec)");
	m_refreshSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_refreshSlider->type(FL_HOR_NICE_SLIDER);
	m_refreshSlider->labelfont(FL_COURIER);
	m_refreshSlider->labelsize(12);
	m_refreshSlider->minimum(1);
	m_refreshSlider->maximum(300);
	m_refreshSlider->step(1);
	m_refreshSlider->value(refreshInterval);
	m_refreshSlider->align(FL_ALIGN_RIGHT);
	m_refreshSlider->callback(cb_refreshSlides);

	// install filter slider for cube map
	m_filterSlider = new Fl_Value_Slider(100, 115, 180, 20, "Filter Width");
	m_filterSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_filterSlider->type(FL_HOR_NICE_SLIDER);
	m_filterSlider->labelfont(FL_COURIER);
	m_filterSlider->labelsize(12);
	m_filterSlider->minimum(1);
	m_filterSlider->maximum(10);
	m_filterSlider->step(1);
	m_filterSlider->value(m_nFilterWidth);
	m_filterSlider->align(FL_ALIGN_RIGHT);
	m_filterSlider->callback(cb_filterSlides);
	m_filterSlider->deactivate();

	// set up cube map checkbox
	m_cubeMapCheckButton = new Fl_Check_Button(10, 115, 70, 20, "CubeMap");
	m_cubeMapCheckButton->user_data((void*)(this));
	m_cubeMapCheckButton->callback(cb_cubeMapCheckButton);
	m_cubeMapCheckButton->value(m_usingCubeMap);
	m_cubeMapCheckButton->deactivate();

	// set up KdTree checkbox
	m_kdTreeCheckButton = new Fl_Check_Button(10, 140, 70, 20, "KdTree");
	m_kdTreeCheckButton->user_data((void*)(this));
	m_kdTreeCheckButton->callback(cb_kdTreeCheckButton);
	m_kdTreeCheckButton->value(m_usingKdTree);

	// set up thread number slider
	m_threadNumSlider = new Fl_Value_Slider(10, 165, 180, 20, "Thread Number");
	m_threadNumSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_threadNumSlider->type(FL_HOR_NICE_SLIDER);
	m_threadNumSlider->labelfont(FL_COURIER);
	m_threadNumSlider->labelsize(12);
	m_threadNumSlider->minimum(1);
	m_threadNumSlider->maximum(16);
	m_threadNumSlider->step(1);
	m_threadNumSlider->value(m_nThreadNum);
	m_threadNumSlider->align(FL_ALIGN_RIGHT);
	m_threadNumSlider->callback(cb_threadNumSlides);

	// set up super sampling number slider
	m_superSamplingNumSlider = new Fl_Value_Slider(10, 190, 180, 20, "Super Sapling Number");
	m_superSamplingNumSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_superSamplingNumSlider->type(FL_HOR_NICE_SLIDER);
	m_superSamplingNumSlider->labelfont(FL_COURIER);
	m_superSamplingNumSlider->labelsize(12);
	m_superSamplingNumSlider->minimum(1);
	m_superSamplingNumSlider->maximum(16);
	m_superSamplingNumSlider->step(1);
	m_superSamplingNumSlider->value(m_nSuperSamplingNum);
	m_superSamplingNumSlider->align(FL_ALIGN_RIGHT);
	m_superSamplingNumSlider->callback(cb_superSamplingNumSlides);


	// set up termination threshold slider
	m_termThresSlider = new Fl_Value_Slider(10, 215, 180, 20, "Threshold (*0.001)");
	m_termThresSlider->user_data((void*)(this));	// record self to be used by static callback functions
	m_termThresSlider->type(FL_HOR_NICE_SLIDER);
	m_termThresSlider->labelfont(FL_COURIER);
	m_termThresSlider->labelsize(12);
	m_termThresSlider->minimum(0);
	m_termThresSlider->maximum(100);
	m_termThresSlider->step(0);
	m_termThresSlider->value(m_ntermThres);
	m_termThresSlider->align(FL_ALIGN_RIGHT);
	m_termThresSlider->callback(cb_termThresSlides);


	// set up debugging display checkbox
	m_debuggingDisplayCheckButton = new Fl_Check_Button(10, 429, 140, 20, "Debugging display");
	m_debuggingDisplayCheckButton->user_data((void*)(this));
	m_debuggingDisplayCheckButton->callback(cb_debuggingDisplayCheckButton);
	m_debuggingDisplayCheckButton->value(m_displayDebuggingInfo);

	m_mainWindow->callback(cb_exit2);
	m_mainWindow->when(FL_HIDE);
	m_mainWindow->end();

	// load cube map view
	m_cubeMapChooser = new CubeMapChooser();
	m_cubeMapChooser->setCaller(this);

	// image view
	m_traceGlWindow = new TraceGLWindow(100, 150, m_nSize, m_nSize, traceWindowLabel);
	m_traceGlWindow->end();
	m_traceGlWindow->resizable(m_traceGlWindow);

	// debugging view
	m_debuggingWindow = new DebuggingWindow();
}

#endif

