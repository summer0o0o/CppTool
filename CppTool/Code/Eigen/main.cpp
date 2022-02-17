// Eigen.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <iostream>
#include "Eigen/Dense"
#include "Common.h"
//#include "MatrixAndVector.h"
#include "Pose.h"
//#include "EigenRotation.h"
using namespace std;
using namespace Eigen;


void test()
{
    auto v1 = Eigen::Vector3d::Zero();
    auto v2 = Eigen::Vector3d(2, 0, 0);
    auto v3 = Eigen::Vector3d(4, 0, 0);

    auto quat = Quaterniond::Identity();

    AngleAxisd rollAngle(AngleAxisd(0, Vector3d::UnitX()));
    AngleAxisd pitchAngle(AngleAxisd(0, Vector3d::UnitY()));
    AngleAxisd yawAngle(AngleAxisd(M_PI, Vector3d::UnitZ()));
    auto quat2 = yawAngle * pitchAngle * rollAngle;

    Rigid3d a(v1, quat);
    Rigid3d b(v2, quat);
    Rigid3d c(v3, quat2);
    auto d = c * b.inverse() * a;

    std::cout << d.translation() << endl;
    std::cout << d.eulerAngle() << endl;
}


int main()
{
    //test();
    //LeastSquareMethod();
    //TestMatrix();
    //TestVector();
    //testPose3d();
    RankOfMatrix();
    return 0;
}

