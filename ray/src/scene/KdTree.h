#ifndef __KDTREE_H__
#define __KDTREE_H__
// KdTree for speeding up intersect
// To build kdtree:
//		kt = KdTree(); 
//		kt.add(obj);
// 		kt.split();

#include <vector>
#include <algorithm>
#include <map>
#include <string>
#include <memory>

#include "ray.h"
#include "material.h"
#include "camera.h"
#include "bbox.h"
#include "scene.h"

#include "../vecmath/vec.h"
#include "../vecmath/mat.h"



struct Interface
{
	int index;
	double value;
	Interface(int i, int v): index(i),value(v) {}
};



template<class T>
class KdTree {

	typedef std::vector<Geometry*> ObjVec;
	typedef std::vector<Geometry*>::const_iterator giter;

public:
	KdTree(): objects() {}
	KdTree(ObjVec objs) {
		for(giter j=objs.begin(); j!=objs.end(); ++j) {
			add(*j);
		}
	}
	~KdTree();

	void add( Geometry* obj ) {
	    obj->ComputeBoundingBox();
		treeBounds.merge(obj->getBoundingBox());
	    objects.push_back(obj);
  	}

  	void split(Interface infa) {
  		ObjVec leftObjs;
  		ObjVec rightObjs;
  		int i = infa.index;
  		double v = infa.value;
  		for(giter j=objects.begin(); j!=objects.end(); ++j) {
			Geometry* obj = (*j);
			Vec3d minPoint = obj->getBoundingBox().getMin();
			Vec3d maxPoint = obj->getBoundingBox().getMax();
			if (minPoint[i]<v) {
				leftObjs.push_back(obj);
			}
			if (maxPoint[i]>v) {
				rightObjs.push_back(obj);
			}
		}
		leftChild = KdTree(leftObjs);
		rightChild = KdTree(rightObjs);
		// tbd
  	}

  	bool intersect(ray& r, isect& i) const;

  	const BoundingBox& bounds() const { return treeBounds; }

private:
	KdTree* leftChild;
	KdTree* rightChild;
  	ObjVec objects;
  	BoundingBox treeBounds;

  	Interface 
};
















#endif // __KDTREE_H__