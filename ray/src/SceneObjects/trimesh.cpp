#include <cmath>
#include <float.h>
#include <algorithm>
#include <assert.h>
#include "trimesh.h"
#include "../ui/TraceUI.h"
extern TraceUI* traceUI;

using namespace std;

Trimesh::~Trimesh()
{
    for( Materials::iterator i = materials.begin(); i != materials.end(); ++i )
        delete *i;
}

// must add vertices, normals, and materials IN ORDER
void Trimesh::addVertex( const Vec3d &v )
{
    vertices.push_back( v );
}

void Trimesh::addMaterial( Material *m )
{
    materials.push_back( m );
}

void Trimesh::addNormal( const Vec3d &n )
{
    normals.push_back( n );
}

// Returns false if the vertices a,b,c don't all exist
bool Trimesh::addFace( int a, int b, int c )
{
    int vcnt = vertices.size();

    if( a >= vcnt || b >= vcnt || c >= vcnt ) return false;

    TrimeshFace *newFace = new TrimeshFace( scene, new Material(*this->material), this, a, b, c );
    newFace->setTransform(this->transform);
    if (!newFace->degen) faces.push_back( newFace );


    // Don't add faces to the scene's object list so we can cull by bounding box
    // scene->add(newFace);
    return true;
}

char* Trimesh::doubleCheck()
// Check to make sure that if we have per-vertex materials or normals
// they are the right number.
{
    if( !materials.empty() && materials.size() != vertices.size() )
        return "Bad Trimesh: Wrong number of materials.";
    if( !normals.empty() && normals.size() != vertices.size() )
        return "Bad Trimesh: Wrong number of normals.";

    return 0;
}

bool Trimesh::intersectLocal(ray& r, isect& i) const
{
    double tmin = 0.0;
    double tmax = 0.0;
    typedef Faces::const_iterator iter;
    bool have_one = false;
    for( iter j = faces.begin(); j != faces.end(); ++j ) {
        isect cur;
        if( (*j)->intersectLocal( r, cur ) ) {
            if( !have_one || (cur.t < i.t) ) {
                i = cur;
                have_one = true;
            }
       }
    }
    if( !have_one ) i.setT(1000.0);
    return have_one;
}

bool TrimeshFace::intersect(ray& r, isect& i) const {
  return intersectLocal(r, i);
}

// Intersect ray r with the triangle abc.  If it hits returns true,
// and put the parameter in t and the barycentric coordinates of the
// intersection in alpha, beta and gamma.
// bool TrimeshFace::intersectLocal(ray& r, isect& i) const
// {
//     const Vec3d& A = parent->vertices[ids[0]];
//     const Vec3d& B = parent->vertices[ids[1]];
//     const Vec3d& C = parent->vertices[ids[2]];

//     // YOUR CODE HERE
//     Vec3d P = r.p;
//     Vec3d d = r.d;
//     Vec3d n = (B-A)^(C-A);
//     if (!n.iszero())
//         n.normalize();
//     double d_ = -(n*C);

//     // get intersection point Q
//     if (n*d == 0)
//         return false;

//     double t = -(n*P+d_)/(n*d);
//     if (t<RAY_EPSILON)                  // important
//         return false;

//     Vec3d Q = r.at(t);

//     // // check whether Q is in abc
//     // //
//     // double Ax, Ay, Bx, By, Cx, Cy, Qx, Qy;
//     // // project ABC along with the maximal direction of normal
//     // int pi,pj;
//     // if (abs(n[0])>abs(n[1]) && abs(n[0])>abs(n[2])) {
//     //     pi = 1;
//     //     pj = 2;
//     // } else if (abs(n[1])>abs(n[0]) && abs(n[1])>abs(n[2])) {
//     //     pi = 0;
//     //     pj = 2;
//     // } else {
//     //     pi = 0;
//     //     pj = 1;
//     // }
//     // Ax = A[pi];
//     // Ay = A[pj];
//     // Bx = B[pi];
//     // By = B[pj];
//     // Cx = C[pi];
//     // Cy = C[pj];
//     // Qx = Q[pi];
//     // Qy = Q[pj];

//     // // calculate area
//     // double alpha = getDeterminant(Qx,Qy,1,Bx,By,1,Cx,Cy,1);
//     // if (alpha<0) return false;
//     // double beta = getDeterminant(Ax,Ay,1,Qx,Qy,1,Cx,Cy,1);
//     // if (beta<0) return false;
//     // double gamma = getDeterminant(Ax,Ay,1,Bx,By,1,Qx,Qy,1);
//     // if (gamma<0) return false;

//     // double deno = alpha+beta+gamma;
//     // alpha /= deno;
//     // beta /= deno;
//     // gamma /= deno;

//     // specify isect i
//     i.t = t;
//     i.N = n;
//     i.setObject(this);
//     i.setMaterial(*material);
//     // i.setBary(alpha, beta, gamma);

//     return true;
// }


bool TrimeshFace::intersectLocal(ray& r, isect& i) const
{
    const Vec3d& A = parent->vertices[ids[0]];
    const Vec3d& B = parent->vertices[ids[1]];
    const Vec3d& C = parent->vertices[ids[2]];

    // YOUR CODE HERE
    Vec3d P = r.p;
    Vec3d d = r.d;
    Vec3d n = (A-C)^(B-C);
    if (!n.iszero())
        n.normalize();
    double d_ = -(n*A);

    // get intersection point Q
    if (n*d == 0)
        return false;

    double t = -(n*P+d_)/(n*d);
    if (t<RAY_EPSILON)                  // important
        return false;

    Vec3d Q = r.at(t);

    // alpha, beta, gamma need to be switched here
    // check whether Q is in abc
    double gamma = n*((B-A)^(Q-A));
    if (gamma<0) return false;
    double alpha =  n*((C-B)^(Q-B));
    if (alpha<0) return false;
    double beta = n*((A-C)^(Q-C)); 
    if (beta<0) return false;

    double deno = alpha+beta+gamma;
    alpha /= deno;
    beta /= deno;
    gamma /= deno;

    // specify isect i
    i.t = t;
    i.N = n;
    i.setObject(this);
    i.setMaterial(*material);
    i.setBary(alpha, beta, gamma);
    i.setUVCoordinates(Vec2d(alpha, beta));

    // smooth shader
    if (parent->normals.size()>0) {
        const Vec3d& NA = parent->normals[ids[0]];
        const Vec3d& NB = parent->normals[ids[1]];
        const Vec3d& NC = parent->normals[ids[2]];
        i.N = alpha*NA + beta*NB + gamma*NC;
        i.N.normalize();
    }

    return true;
}




double TrimeshFace::getDeterminant(double ax, double ay, double az,
                        double bx, double by, double bz,
                        double cx, double cy, double cz) const    // added by lihang liu
{
    return az*(bx*cy-by*cx)-bz*(ax*cy-cx*ay)+cz*(ax*by-ay*bx);
}

void Trimesh::generateNormals()
// Once you've loaded all the verts and faces, we can generate per
// vertex normals by averaging the normals of the neighboring faces.
{
    int cnt = vertices.size();
    normals.resize( cnt );
    int *numFaces = new int[ cnt ]; // the number of faces assoc. with each vertex
    memset( numFaces, 0, sizeof(int)*cnt );
    
    for( Faces::iterator fi = faces.begin(); fi != faces.end(); ++fi )
    {
        Vec3d faceNormal = (**fi).getNormal();
        
        for( int i = 0; i < 3; ++i )
        {
            normals[(**fi)[i]] += faceNormal;
            ++numFaces[(**fi)[i]];
        }
    }

    for( int i = 0; i < cnt; ++i )
    {
        if( numFaces[i] )
            normals[i]  /= numFaces[i];
    }

    delete [] numFaces;
    vertNorms = true;
}
