#include <cmath>

#include "light.h"

using namespace std;

double DirectionalLight::distanceAttenuation(const Vec3d& P) const
{
  // distance to light is infinite, so f(di) goes to 0.  Return 1.
  return 1.0;
}


Vec3d DirectionalLight::shadowAttenuation(const ray& r, const Vec3d& p) const
{
  // YOUR CODE HERE:
  // You should implement shadow-handling code here.
  isect i;
  ray r_2nd(p, getDirection(p), ray::REFLECTION);
  if(scene->intersect(r_2nd, i)) {
      Vec3d kt = i.material->kt(i);
      return kt;
  }

  return Vec3d(1,1,1);
}

Vec3d DirectionalLight::getColor() const
{
  return color;
}

Vec3d DirectionalLight::getDirection(const Vec3d& P) const
{
  // for directional light, direction doesn't depend on P
  return -orientation;
}

double PointLight::distanceAttenuation(const Vec3d& P) const
{

  // YOUR CODE HERE

  // You'll need to modify this method to attenuate the intensity 
  // of the light based on the distance between the source and the 
  // point P.  For now, we assume no attenuation and just return 1.0
  Vec3d ret = position - P;
  double d = sqrt(ret*ret);
  return min(1.0, 1/(constantTerm+linearTerm*d+quadraticTerm*d*d));
//  return 1.0;
}

Vec3d PointLight::getColor() const
{
  return color;
}

Vec3d PointLight::getDirection(const Vec3d& P) const
{
  Vec3d ret = position - P;
  ret.normalize();
  return ret;
}


Vec3d PointLight::shadowAttenuation(const ray& r, const Vec3d& p) const
{
  // YOUR CODE HERE:
  // You should implement shadow-handling code here.
  isect i;
  ray r_2nd(p, getDirection(p), ray::REFLECTION);
  if(scene->intersect(r_2nd, i)) {
      // check the intersection if before or after the light
      Vec3d q = r_2nd.at(i.t);
      if ((position-p).length()>(q-p).length()) {
        // // soft shadows
        // ray r_3(q, getDirection(p), ray::REFLECTION);     // 
        // isect i_3;
        // if (scene->intersect(r_3, i_3)) {
        //   double factor = max(0.0, 1-i_3.t);
        //   return factor*Vec3d(1,1,1);
        // }
        // standard shadows
        Vec3d kt = i.material->kt(i);
        return kt;
      }
  }

  return Vec3d(1,1,1);
}


