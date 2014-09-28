#include <iostream>
#include <core/sutherlandhodgmanclipper.h>
#include <core/gpumemory.h>
#include <core/constant.h>

#include <vector>
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

ostream & operator <<(ostream &os,const Triangle &t){

    os << "========================\n";
    os << t.p1[0] <<" " << t.p1[1] << " " << t.p1[2] << endl;
    os << t.p2[0] <<" " << t.p2[1] << " " << t.p2[2] << endl;
    os << t.p3[0] <<" " << t.p3[1] << " " << t.p3[2] << endl;
    os << "========================\n";
    return os;
}

int main(){

    Triangle *data;
    GPUMemory::alloc<Triangle>(Constant::SF_PRIMITIVESETUPOUT,3,data);
//    data[0] = glm::vec4(0.5f,0.5f,0.5f,1.0f);
//    data[1] = glm::vec4(-1.0f,0.5f,1.0f,1.0f);
//    data[2] = glm::vec4(1.0f,0.5f,2.0f,1.0f);

    SutherlandHodgmanClipper clipper;
//    clipper.execute();

    vector<vec4> inPolygon;
    vector<Triangle> out;

    inPolygon.push_back(glm::vec4(-0.5f,-0.5f,0.0f,1.0f));
    inPolygon.push_back(glm::vec4(0.5f,-0.5f,0.0f,1.0f));
    inPolygon.push_back(glm::vec4(0.5f,0.5f,0.0f,1.0f));
    inPolygon.push_back(glm::vec4(0.0f,0.0f,0.0f,1.0f));
    inPolygon.push_back(glm::vec4(-0.5f,0.5f,0.0f,1.0f));

//    inPolygon.push_back(glm::vec4(0.0f,0.5f,0.0f,1.0f));
//    inPolygon.push_back(glm::vec4(0.0f,-0.5f,0.0f,1.0f));
//    inPolygon.push_back(glm::vec4(2.0f,-0.5f,0.0f,1.0f));
//    inPolygon.push_back(glm::vec4(0.25f,0.0f,0.0f,1.0f));
//    inPolygon.push_back(glm::vec4(1.0f,0.5f,0.0f,1.0f));


    clipper.polygonToTriangle(inPolygon,out);

    for (int var = 0; var < out.size(); ++var) {
        cout << "Tri"<<var<<endl;
        cout << out.at(var);
    }


    return 0;
}