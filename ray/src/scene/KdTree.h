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

// #include "../SceneObjects/trimesh.h"

#include "../vecmath/vec.h"
#include "../vecmath/mat.h"

using namespace std;



struct Interface
{
	int index;
	double value;
};

struct BBoxComparator
{
	int index;
	BBoxComparator(int i) {
		index = i;
	}
    bool operator()( const BoundingBox& lx, const BoundingBox& rx) const {
        return lx.getMin()[index]<rx.getMin()[index];
    }
};

template <typename T> 
struct TComparator 
{ 
  	int axis_;
  	int by_;		// 0: min, 1: max, 2: center
	TComparator(int index, int by = true) {
		axis_ = index;
		by_ = by;
	}
	bool operator()( const T* lx, const T* rx) const {
    	Vec3d minPoint1 = lx->getBoundingBox().getMin();
    	Vec3d minPoint2 = rx->getBoundingBox().getMin();
    	Vec3d maxPoint1 = lx->getBoundingBox().getMax();
    	Vec3d maxPoint2 = rx->getBoundingBox().getMax();
    	if (by_ == 0) {
    		return minPoint1[axis_] < minPoint2[axis_];
    	} else if (by_ == 1) {
    		return maxPoint1[axis_] < maxPoint2[axis_]; 
    	} else {
    		return (minPoint1[axis_]+maxPoint1[axis_]) < (minPoint2[axis_]+maxPoint2[axis_]);
    	}
    }
}; 


// KdTree
//
//

template<class T>
class KdTree {

	typedef std::vector<T*> ObjVec;
	typedef std::vector<BoundingBox> BBoxVec;
	typedef typename std::vector<T*>::const_iterator giter;
	typedef typename std::vector<BoundingBox>::const_iterator biter;

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
  	int getDepth() const;
  	void printObjects(ObjVec objs, int axis);
  	const BoundingBox& bounds() const { return treeBounds; }

private:
	KdTree<T>* leftChild;
	KdTree<T>* rightChild;
  	ObjVec objects;
  	BoundingBox treeBounds;
  	int splitAxis;

  	int maxObjNum;

  	void add( T* obj );

  	void splitByAF();
  	void getMinAF(const ObjVec sorted_objs, double& minAF, int& minI);

  	void splitIfTrimesh();
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

	// cout << "-----\n new trees\n";
	// print();
	// getchar();

	// check whether to split
	if (objs.size()>=maxObjNum) {
		splitByAF();
	} 
	// if not split, check if there is any trimesh
	else {
		splitIfTrimesh();
	}
}

template<class T>
inline bool KdTree<T>::intersect(ray& r, isect& i) const {
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
inline bool KdTree<T>::intersectLocal(ray& r, isect& i) const {
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
int KdTree<T>::getDepth() const{
	if (!leftChild)
		return 1;
	return 1+max(leftChild->getDepth(), rightChild->getDepth());
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


// method 2: area function
//
//

template<class T>
void KdTree<T>::splitByAF() {
	int minI = 0;
	double minAF = 1.0e308;
	int minAxis = 0;
	int minBy = 0;
	// evaluate minI, minAxis and minF over all axises.
	for (int by=0; by<2;++by) {
		for (int axis=0;axis<3;++axis) {
			double cAF;
			int cI;
			std::sort(objects.begin(), objects.end(),TComparator<T>(axis, by));
			getMinAF(objects, cAF, cI);
			if (cAF < minAF) {
				minAF = cAF;
				minI = cI;
				minAxis = axis;
				minBy = by;
			}
		}
	}
	
	splitAxis = minAxis;
	// pass objects to leftObj and rightObj by minI and minAxis
	ObjVec leftObjs;
	ObjVec rightObjs;
	std::sort(objects.begin(), objects.end(),TComparator<T>(minAxis, minBy));
	for (int i=0;i<objects.size();++i) {
		T* obj = objects.at(i);
		if (i<=minI)
			leftObjs.push_back(obj);
		else
			rightObjs.push_back(obj);
	}
	// construct child tree
	// cout << "split by " <<minAxis<<" of "<<minI<<"\n";
	// cout << "	left " ;
	// cout << leftObjs.size() <<endl;
	// printObjects(leftObjs, minAxis);
	// cout << "	right " ;
	// cout << rightObjs.size() <<endl;
	// printObjects(rightObjs, minAxis);
	leftChild = new KdTree(leftObjs, maxObjNum);
	rightChild = new KdTree(rightObjs, maxObjNum);
}

template<class T>
void KdTree<T>::getMinAF(const ObjVec sorted_objs, double& minAF, int& minI) {
	// calculate sA & sB
	int n = sorted_objs.size();
	double* sA_list = new double[n];
	double* sB_list = new double[n];
	BoundingBox leftBounds, rightBounds;		// start merging from left and right, respectively
	for(int i=0;i<n;++i) {
		BoundingBox box1 = sorted_objs.at(i)->getBoundingBox();
		leftBounds.merge(box1);
		sA_list[i] = leftBounds.area();

		BoundingBox box2 = sorted_objs.at(n-1-i)->getBoundingBox();
		rightBounds.merge(box2);
		sB_list[i] = rightBounds.area();
	} 

	// get minimal f(i) = sA*nA+sB*nB
	minAF = 1.0e308;
	minI = 0;
	for (int i=0;i<n-1;++i) {
		double sA = sA_list[i];
		int nA = i+1;
		double sB = sB_list[n-2-i];
		int nB = n-i-1;
		double f = (sA*nA+sB*nB);
		if (f<minAF) {
			minAF = f;
			minI = i;
		}
	}
}


// when maximal number is not reached, 
//	check if there is a trimesh before stop
//
template<class T>
void KdTree<T>::splitIfTrimesh() {
	// check if there is a trimesh
	ObjVec leftObjs;
	ObjVec rightObjs;
	T* trimesh = NULL;
	for (int i=0;i<objects.size();++i) {
		T* obj = objects.at(i);
		if (obj->ifTrimesh() && !trimesh) 
			trimesh = obj;
		else 
			leftObjs.push_back(obj);
	}
	// split the tree if there is a trimesh
	if (trimesh) {
		// construct child tree
		// cout << "split because of trimesh\n";
		// cout << "trimesh size: " << trimesh->getFaces().size() <<endl;
		leftChild = new KdTree(leftObjs, maxObjNum);
		// rightObjs.push_back(trimesh->getFaces());
		// rightObjs.insert(rightObjs.end(), trimesh->getFaces().begin(), trimesh->getFaces().end());
		// // rightObjs.push_back(trimesh->faces.at(0));
		rightChild = new KdTree(trimesh->getFaces(), maxObjNum);
	}

}


#endif // __KDTREE_H__
