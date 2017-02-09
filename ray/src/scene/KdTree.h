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

#include "../vecmath/vec.h"
#include "../vecmath/mat.h"

using namespace std;


struct Interface
{
	int index;
	double value;
};

struct BBoxComparatorX
{
    bool operator()( const BoundingBox& lx, const BoundingBox& rx) const {
        return lx.getMin()[0]<rx.getMin()[0];
    }
};

struct BBoxComparatorY
{
    bool operator()( const BoundingBox& lx, const BoundingBox& rx) const {
        return lx.getMin()[1]<rx.getMin()[1];
    }
};

struct BBoxComparatorZ
{
    bool operator()( const BoundingBox& lx, const BoundingBox& rx) const {
        return lx.getMin()[2]<rx.getMin()[2];
    }
};


template<class T>
class KdTree {

	typedef std::vector<T*> ObjVec;
	typedef std::vector<BoundingBox> BBoxVec;
	typedef typename std::vector<T*>::const_iterator giter;

public:
	KdTree(ObjVec objs, int maxObjNum);

	~KdTree() {
		if (leftChild)
			delete leftChild;
		if (rightChild)
			delete rightChild;
	}

  	bool intersect(ray& r, isect& i) const;
  	bool intersectLocal(ray& r, isect& i) const;
  	void print() const;
  	void printObjects(ObjVec objs, int axis);
  	const BoundingBox& bounds() const { return treeBounds; }

private:
	KdTree<T>* leftChild;
	KdTree<T>* rightChild;
  	ObjVec objects;
  	BoundingBox treeBounds;
  	Interface interface;

  	int maxObjNum;

  	void add( T* obj );
  	Interface getBestInterface();
  	void split(Interface infa);
};


template<class T>
KdTree<T>::KdTree(ObjVec objs, int maxObjNum) {
	// property initilization
	leftChild = NULL;
	rightChild = NULL;
	this->maxObjNum = maxObjNum;
	for(giter j=objs.begin(); j!=objs.end(); ++j) {
		add(*j);
	}

	// cout << "-----\n new tree\n";
	// print();
	// getchar();

	// check whether to split
	if (objs.size()>=maxObjNum) {
		interface = getBestInterface();
		split(interface);
	}
}

template<class T>
bool KdTree<T>::intersect(ray& r, isect& i) const {
	// printf("in\n");
	// print();
	// check the bounding box first
	double tmin, tmax;
	if (!treeBounds.intersect(r, tmin, tmax)) return false;
	// recursively calls to left and right child
	bool res = false;
	if (leftChild) {
		isect cur1, cur2;
		if (leftChild->intersect(r, cur1)) {
			i = cur1;
			res = true;
		}
		if (rightChild->intersect(r,cur2)) {
			if (!res || cur2.t<i.t) {
				i = cur2;
				res = true;
			}
		}
	} else {
		res = intersectLocal(r,i);
	}

	// check the result
	if (!res) i.setT(1000.0);
	return res;
}

template<class T>
bool KdTree<T>::intersectLocal(ray& r, isect& i) const {
	bool have_one = false;
	for(giter j = objects.begin(); j != objects.end(); ++j) {
		isect cur;
		if( (*j)->intersect(r, cur) ) {
			if(!have_one || (cur.t < i.t)) {
				i = cur;
				have_one = true;
			}
		}
	}
	if(!have_one) i.setT(1000.0);
	return have_one;
}

template<class T>
void KdTree<T>::print() const{
	cout << "objects size " << objects.size() <<endl;
}

template<class T>
void KdTree<T>::printObjects(ObjVec objs, int axis) {
	cout << "objects size " << objs.size() <<endl;
	for(giter j=objs.begin(); j!=objs.end(); ++j) {
		T* obj = (*j);
		cout << "(" << obj->getBoundingBox().getMin()[axis] << ",";
		cout << obj->getBoundingBox().getMax()[axis] <<")";
	}
	cout <<"\n";
}

template<class T>
void KdTree<T>::add( T* obj ) {
	obj->ComputeBoundingBox();
	treeBounds.merge(obj->getBoundingBox());
	objects.push_back(obj);
}


template<class T>
void KdTree<T>::split(Interface infa) {
	ObjVec leftObjs;
	ObjVec rightObjs;
	int i = infa.index;
	double v = infa.value;
	for(giter j=objects.begin(); j!=objects.end(); ++j) {
		T* obj = (*j);
		Vec3d minPoint = obj->getBoundingBox().getMin();
		Vec3d maxPoint = obj->getBoundingBox().getMax();
		if (minPoint[i]<v) {
			leftObjs.push_back(obj);
		} else {
			rightObjs.push_back(obj);
		}
	}
	// cout << "split by " <<i<<" of "<<v<<"\n";
	// cout << "	left " ;
	// printObjects(leftObjs, i);
	// cout << "	right " ;
	// printObjects(rightObjs, i);
	leftChild = new KdTree(leftObjs, maxObjNum);
	rightChild = new KdTree(rightObjs, maxObjNum);
}

// ray -r 8 ../scenes/hitchcock.ray output.bmp
// build tree: 0.006711
// total time = 5.47897 seconds, rays traced = 262144
// template<class T>
// Interface KdTree<T>::getBestInterface() {
// 	// collect all bounding boxes
// 	BBoxVec boxVec;
// 	for(giter j=objects.begin(); j!=objects.end(); ++j) {
// 		T* obj = (*j);
// 		boxVec.push_back(obj->getBoundingBox());
// 	}
// 	// find maximal axis length
// 	Vec3d minPoint = treeBounds.getMin();
// 	Vec3d maxPoint = treeBounds.getMax();
// 	double absx = maxPoint[0]-minPoint[0];
// 	double absy = maxPoint[1]-minPoint[1];
// 	double absz = maxPoint[2]-minPoint[2];
// 	// sort bounding boxes
// 	int index = 0;
// 	if (absx>absy && absx>absz) {
// 		std::sort(boxVec.begin(), boxVec.end(),BBoxComparatorX());
// 		index = 0;
// 	}
// 	else if (absy>absx && absy>absz) {
// 		std::sort(boxVec.begin(), boxVec.end(),BBoxComparatorY());
// 		index = 1;
// 	}
// 	else {
// 		std::sort(boxVec.begin(), boxVec.end(),BBoxComparatorZ());
// 		index = 2;
// 	}
// 	// get the middle box
// 	BoundingBox midBox = boxVec.at(boxVec.size()/2);
// 	Interface res = {index, midBox.getMin()[index]};
// 	return res;
// }

// ray -r 8 ../scenes/hitchcock.ray output.bmp
// build tree: 0.005939
// total time = 4.70256 seconds, rays traced = 262144
template<class T>
Interface KdTree<T>::getBestInterface() {
	// collect all bounding boxes
	BBoxVec boxVec;
	for(giter j=objects.begin(); j!=objects.end(); ++j) {
		T* obj = (*j);
		boxVec.push_back(obj->getBoundingBox());
	}
	// find maximal axis length
	Vec3d minPoint = treeBounds.getMin();
	Vec3d maxPoint = treeBounds.getMax();
	double absx = maxPoint[0]-minPoint[0];
	double absy = maxPoint[1]-minPoint[1];
	double absz = maxPoint[2]-minPoint[2];
	// sort bounding boxes
	int index = 0;
	if (absx>absy && absx>absz) {
		index = 0;
	} else if (absy>absx && absy>absz) {
		index = 1;
	} else {
		index = 2;
	}
	// get the middle point
	double value = (maxPoint[index]+minPoint[index])/2;
	Interface res = {index, value};
	return res;
}






#endif // __KDTREE_H__
