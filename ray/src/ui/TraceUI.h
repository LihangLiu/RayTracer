//
// rayUI.h
//
// The header file for the UI part
//

#ifndef __rayUI_h__
#define __rayUI_h__

// who the hell cares if my identifiers are longer than 255 characters:
#pragma warning(disable : 4786)

#include <string>
#include <thread>

using std::string;

class RayTracer;

class TraceUI {
public:
	TraceUI() : m_nDepth(0), m_nSize(512), m_displayDebuggingInfo(false),
                    m_shadows(true), m_smoothshade(true), raytracer(0),
                    m_nFilterWidth(1), m_usingCubeMap(0), m_usingKdTree(1),
                    m_nThreadNum(std::thread::hardware_concurrency()),
                    m_nSuperSamplingNum(1), m_ntermThres(0)
                    {}

	virtual int	run() = 0;

	// Send an alert to the user in some manner
	virtual void alert(const string& msg) = 0;

	// setters
	virtual void setRayTracer( RayTracer* r ) { raytracer = r; }
	void setCubeMap(bool b) { m_gotCubeMap = b; }
	void useCubeMap(bool b) { m_usingCubeMap = b; }
	bool isUsingCubeMap() { return m_usingCubeMap; }
	bool isUsingKdTree() { return m_usingKdTree; }

	// accessors:
	int	getSize() const { return m_nSize; }
	int	getDepth() const { return m_nDepth; }
	int	getFilterWidth() const { return m_nFilterWidth; }
	int getSuperSamplingNum() const { return m_nSuperSamplingNum; }
	int getTermThres() const { return m_ntermThres; }

	bool	shadowSw() const { return m_shadows; }
	bool	smShadSw() const { return m_smoothshade; }

	static bool m_debug;

protected:
	RayTracer*	raytracer;

	int	m_nSize;	// Size of the traced image
	int	m_nDepth;	// Max depth of recursion

	// Determines whether or not to show debugging information
	// for individual rays.  Disabled by default for efficiency
	// reasons.
	bool m_displayDebuggingInfo;
	bool m_shadows;  // compute shadows?
	bool m_smoothshade;  // turn on/off smoothshading?
	bool		m_usingCubeMap;  // render with cubemap
	bool		m_gotCubeMap;  // cubemap defined
	int m_nFilterWidth;  // width of cubemap filter
	bool m_usingKdTree;	// if using kd tree
	int m_nThreadNum;	// thread number
	int m_nSuperSamplingNum;	// the number of samples per pixel
	int m_ntermThres;	// termination threshold *0.001
};

#endif

