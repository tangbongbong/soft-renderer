#include "core/primitive.h"
#include "core/gpumemory.h"
#include "core/constant.h"

#include <iostream>
#include <vector>
#include <algorithm>

using std::vector;

Primitive::Primitive()
{
}

bool Primitive::setup(PrimitiveType type, int count)
{
    bool error = GPUMemory::retrieve(Constant::SF_POSITION,
                                             vertexCount,
                                             verteices);
    if(!error){
        std::cerr << "Retrieve sf_position failed!" << std::endl;
        return false;
    }

    switch (type) {
    case TRIANGLES:{
        Triangle* objects;
        int size = count < vertexCount ? count : vertexCount;
        size = size/3;
        GPUMemory::alloc(Constant::SF_PRIMITIVESETUPOUT,
                         size,
                         objects);

#pragma omp parallel for
        for (int i = 0; i < size; i++) {
            objects[i].p1 = verteices[i*3];
            objects[i].p2 = verteices[i*3+1];
            objects[i].p3 = verteices[i*3+2];
        }
    }
        break;
    default:
        break;
    }

    return true;
}

bool Primitive::setupByIndex(PrimitiveType type, int count)
{
    bool error = GPUMemory::retrieve(Constant::SF_POSITION,vertexCount,verteices);
    if(!error){
        std::cerr << "Retrieve sf_position failed!\n";
        return false;
    }
    error = GPUMemory::retrieve(Constant::SF_POSITIONINDEX,indexSize,indexData);
    if(!error){
        std::cerr << "Retrieve sf_position_index failed!\n";
        return false;
    }


    return true;
}

/*
 *  algo: www.cnblogs.com/graphics/archive/2010/08/05/1793393.html
 */
bool Triangle::inside(PointObject p) const
{

    p = p/p.w;
    vec3 v0(p3.getPos() - p1.getPos());
    vec3 v1(p2.getPos() - p1.getPos());
    vec3 v2(p.getPos() - p1.getPos());

    float dot00 = glm::dot(v0,v0);
    float dot01 = glm::dot(v0,v1);
    float dot02 = glm::dot(v0,v2);
    float dot11 = glm::dot(v1,v1);
    float dot12 = glm::dot(v1,v2);

    float inverDeno = 1 / (dot00 * dot11 - dot01 * dot01) ;

    float u = (dot11 * dot02 - dot01 * dot12) * inverDeno ;
    // if u out of range, return directly
    if (u < 0 || u > 1)
    {
        return false ;
    }

    float v = (dot00 * dot12 - dot01 * dot02) * inverDeno ;
    // if v out of range, return directly
    if (v < 0 || v > 1)
    {
        return false ;
    }
    return u + v <= 1 ;

}

void Triangle::extremeValue(glm::vec2 &_min, glm::vec2 &_max) const
{
    float xMin;
    float yMin;
    float xMax;
    float yMax;

    xMin = fmin(p1.getPos().x,p2.getPos().x);
    xMin = fmin(xMin,p3.getPos().x);

    yMin = fmin(p1.getPos().y,p2.getPos().y);
    yMin = fmin(yMin,p3.getPos().y);

    xMax = fmax(p1.getPos().x,p2.getPos().x);
    xMax = fmax(xMax,p3.getPos().x);

    yMax = fmax(p1.getPos().y,p2.getPos().y);
    yMax = fmax(yMax,p3.getPos().y);

    _min.x = xMin;
    _min.y = yMin;
    _max.x = xMax;
    _max.y = yMax;
}

inline bool between(float low,float high,float vaule){
    if (vaule >=low && vaule <= high)
        return true;
    return false;
}

/**
 * @brief Triangle::intersect this triangle
 *        intersect with line y=c
 * @param y
 * @param out
 */
void Triangle::intersect(float y,float min_x,float max_x, std::set<float> &result)
{
    float y1;
    float y2;
    float y3;

    //1
    y1 = p1.getPos().y;
    y2 = p2.getPos().y;
    if(y1 > y2)
        std::swap(y1,y2);

    if (between(y1,y2,y) && p2.getPos().y!=p1.getPos().y){
        float u1 = (y-p1.getPos().y)/(p2.getPos().y-p1.getPos().y);
        float x;
        x= p1.getPos().x + u1*(p2.getPos().x-p1.getPos().x);
        if(between(min_x,max_x,x))
            result.insert(x);
    }

    //2
    y1 = p1.getPos().y;
    y3 = p3.getPos().y;
    if(y1 > y3)
        std::swap(y1,y3);

    if (between(y1,y3,y) && p3.getPos().y!=p1.getPos().y){
        float u2 = (y-p1.getPos().y)/(p3.getPos().y-p1.getPos().y);
        float x;
        x= p1.getPos().x + u2*(p3.getPos().x-p1.getPos().x);
        if(between(min_x,max_x,x))
            result.insert(x);
    }

    //3
    y2 = p2.getPos().y;
    y3 = p3.getPos().y;
    if(y2 > y3)
        std::swap(y2,y3);
    if (between(y2,y3,y) && p3.getPos().y!=p2.getPos().y){
        float u3 = (y-p2.getPos().y)/(p3.getPos().y-p2.getPos().y);
        float x;
        x= p2.getPos().x + u3*(p3.getPos().x-p2.getPos().x);
        if(between(min_x,max_x,x))
            result.insert(x);
    }
}

void Triangle::perspectiveDivide()
{
    p1.x = p1.x / p1.w;
    p1.y = p1.y / p1.w;
    p1.z = p1.z / p1.w;
    p1.w = 1.0f;
    p2.x = p2.x / p2.w;
    p2.y = p2.y / p2.w;
    p2.z = p2.z / p2.w;
    p2.w = 1.0f;
    p3.x = p3.x / p3.w;
    p3.y = p3.y / p3.w;
    p3.z = p3.z / p3.w;
    p3.w = 1.0f;
}

glm::vec3 Triangle::normal() const
{
    glm::vec3 v1(p2.getPos()-p1.getPos());
    glm::vec3 v2(p3.getPos()-p1.getPos());
    glm::vec3 normal = glm::cross(v1,v2);
    return normal;
}

glm::vec4 Triangle::downPoint() const
{
    glm::vec4 p;

    p = p1.getPos().y < p2.getPos().y ? p1.getPos():p2.getPos();
    p = p.y < p3.getPos().y ? p:p3.getPos();

    return p;
}
