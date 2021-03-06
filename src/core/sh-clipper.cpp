#include "core/sh-clipper.h"
#include "core/gpumemory.h"
#include "core/constant.h"
#include "core/vertex.h"
#include <algorithm>


SHClipper::SHClipper()
{
}

void SHClipper::execute()
{
    initialize();
    Sutherland_Hodgman();
}

void SHClipper::Sutherland_Hodgman()
{
    if(!canClip)
        return;

    vector<Triangle> outPrimitive;
    // TODO parallel
    // clip each primitive
    for (int i = 0; i < primitiveCount; ++i) {

        vector<PointObject> *input = new vector<PointObject>;
        vector<PointObject> *output = new vector<PointObject>;

        input->push_back(triangle[i].p1);
        input->push_back(triangle[i].p2);
        input->push_back(triangle[i].p3);

        for (int b = Left; b <= Far; ++b) {
            clip(*input,*output,static_cast<Boundary>(b));
            swap(input,output);
            output->clear();
        }

        if (input->size()>3) {
            polygonToTriangle(*input,outPrimitive);
        }

        if (input->size()==3) {
            Triangle tri;
            tri.p1 = input->at(0);
            tri.p2 = input->at(1);
            tri.p3 = input->at(2);
            outPrimitive.push_back(tri);
        }
        // size less than 3?
        // impossiable, skip
        delete input;
        delete output;
    }
    Triangle *data;
    GPUMemory::alloc<Triangle>(Constant::SF_CLIPOUT,outPrimitive.size(),data);
    GPUMemory::memoryCopy<Triangle>(Constant::SF_CLIPOUT,outPrimitive.size(),&outPrimitive[0]);

}


void SHClipper::clip(vector<PointObject> &input,
                                    vector<PointObject> &output,
                                    Boundary b)
{
    for (size_t i = 0; i < input.size(); ++i) {
        PointObject begin = input[i];
        PointObject end = input[(i+1)%input.size()];

        // both inside, put end to output
        if (inside(begin,b) && inside(end,b))
            output.push_back(end);

        // begin outside, end inside
        // put end and intersect to output
        if ( !inside(begin,b) && inside(end,b)){
            PointObject iPoint = intersect(begin,end,b);

            // must interpolate after intersect
            // end and ipoint careful ! 1-u or u?
            float u = iPoint.distanceTo(end) / begin.distanceTo(end);
            PointObject::interpolate(begin,end,iPoint,1-u);

            // intersect point may same as begin or end point,
            // if does, skip
            if (iPoint!=begin && iPoint!=end)
                output.push_back(iPoint);
            output.push_back(end);
        }

        // begin inside, end outside
        // put intersect to output
        if (inside(begin,b) && !inside(end,b)) {
            PointObject iPoint = intersect(begin,end,b);
            // must interpolate after intersect
            // begin and ipoint careful ! 1-u or u?
            float u = iPoint.distanceTo(begin) / begin.distanceTo(end);
            PointObject::interpolate(begin,end,iPoint,u);

            // intersect point may same as begin or end point,
            // if does, skip
            if (iPoint!=begin && iPoint!=end)
                output.push_back(iPoint);
        }
    }
}

bool SHClipper::inside(PointObject p, Boundary b)
{
    p = p / p.w;
    switch (b) {
    case Left:
        if(p.x < -1.0f)
            return false;
        break;
    case Right:
        if(p.x > 1.0f)
            return false;
        break;
    case Bottom:
        if(p.y < -1.0f)
            return false;
        break;
    case Top:
        if(p.y > 1.0f)
            return false;
        break;
    case Near:
        if(p.z > 1.0f)
            return false;
        break;
    case Far:
        if(p.z < -1.0f)
            return false;
        break;

    default:
        break;
    }
    return true;
}

PointObject SHClipper::intersect(PointObject p1, PointObject p2, Boundary b)
{
    PointObject result;
    result.w = 1.0f;
    p1 = p1 / p1.w;
    p2 = p2 / p2.w;

    float u;
    bool p2In = false;
    switch (b) {
    case Left:{
        u = (-1.0f -p1.x)/(p2.x-p1.x);
        result.x = -1.0f;
        result.y = p1.y + u*(p2.y-p1.y);
        result.z = p1.z + u*(p2.z-p1.z);
        p2In = true;
    }
        break;
    case Right:{
        u = (1.0f -p1.x)/(p2.x-p1.x);
        result.x = 1.0f;
        result.y = p1.y + u*(p2.y-p1.y);
        result.z = p1.z + u*(p2.z-p1.z);
    }
        break;
    case Bottom:{
        u = (-1.0f -p1.y)/(p2.y-p1.y);
        result.x = p1.x + u*(p2.x-p1.x);
        result.y = -1.0f;
        result.z = p1.z + u*(p2.z-p1.z);
    }
        break;
    case Top:{
        u = (1.0f -p1.y)/(p2.y-p1.y);
        result.x = p1.x + u*(p2.x-p1.x);
        result.y = 1.0f;
        result.z = p1.z + u*(p2.z-p1.z);
    }
        break;
    case Near:{
        u = (1.0f -p1.z)/(p2.z-p1.z);
        result.x = p1.x + u*(p2.x-p1.x);
        result.y = p1.y + u*(p2.y-p1.y);
        result.z = 1.0f;
    }
        break;
    case Far:{
        u = (-1.0f -p1.z)/(p2.z-p1.z);
        result.x = p1.x + u*(p2.x-p1.x);
        result.y = p1.y + u*(p2.y-p1.y);
        result.z = -1.0f;
    }
        break;
    default:
        break;
    }
    return result;
}
/**
 * @brief SHClipper::polygonToTriangle
 *        subdivision polygon to triangle
 * @param inPolygon
 * @param out
 */
void SHClipper::polygonToTriangle(vector<PointObject> &inPolygon,
                                  vector<Triangle> &out)
{
    if(inPolygon.size()==3){
        Triangle tri;
        tri.p1 = inPolygon[0];
        tri.p2 = inPolygon[1];
        tri.p3 = inPolygon[2];
        out.push_back(tri);
        return;
    }

    // find middle
    float min_x = inPolygon.front().x / inPolygon.front().w;
    // must initialize the iterator for begin,
    // if middle is first element,
    // middle == polygon.begin() will return false
    vector<PointObject>::iterator middle = inPolygon.begin();
    vector<PointObject>::iterator left  = inPolygon.begin();
    vector<PointObject>::iterator right  = inPolygon.begin();
    for (auto iter = inPolygon.begin(); iter != inPolygon.end(); ++iter) {
        if (iter->x/iter->w < min_x){
            min_x = iter->x/iter->w;
            middle = iter;
        }
    }
    left = middle;
    right = middle;
    // find left
    if  (left==inPolygon.begin()){
        left = inPolygon.end();
    }
    left--;
    // find right
    right++;
    if (right==inPolygon.end()) {
        right=inPolygon.begin();
    }

    // target triangle
    Triangle tri;
    tri.p1 = *left;
    tri.p2 = *middle;
    tri.p3 = *right;
    tri.perspectiveDivide();

    vector<PointObject> polygon;
    // check other point inside
    bool check = false;
    int pIndex;

    int begin = (right - inPolygon.begin()+1)%inPolygon.size();
    int end = left - inPolygon.begin();

    for (int i = begin; i != end;) {
        if (tri.inside(inPolygon.at(i))) {
            check = true;
            pIndex = i;
        }
        // add into new vector
        polygon.push_back(inPolygon.at(i));

        ++i;
        i%=inPolygon.size();
    }

    // yes, split to two polygon
    if(check){
        vector<PointObject> polygon_1;
        vector<PointObject> polygon_2;

        // copy middle to p into 1
        for (int i= &(*middle)-&inPolygon[0]; i != pIndex+1;) {
            polygon_1.push_back(inPolygon.at(i));
            i++;
            i%=inPolygon.size();
        }

        // copy p to middle into 2
        for (int i= pIndex; i != &(*middle)-&inPolygon[0]+1;) {
            polygon_2.push_back(inPolygon.at(i));
            i++;
            i%=inPolygon.size();
        }

        polygonToTriangle(polygon_1,out);
        polygonToTriangle(polygon_2,out);
    }
    else{
        // no, add tri to out
        // delete index point and continue
        out.push_back(tri);
        polygon.push_back(*left);
        polygon.push_back(*right);
        polygonToTriangle(polygon,out);
    }
}












