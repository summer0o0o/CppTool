#pragma once
#include "Eigen/Dense"
using namespace std;
using namespace Eigen;

# define M_PI		3.14159265358979323846	/* pi */

template <typename FloatType>
class Rigid2 {
public:
    using Vector = Eigen::Matrix<FloatType, 2, 1>;
    using Rotation2D = Eigen::Rotation2D<FloatType>;

    Rigid2() : translation_(Vector::Zero()), rotation_(Rotation2D::Identity()) {}
    Rigid2(const Vector& translation, const Rotation2D& rotation)
        : translation_(translation), rotation_(rotation) {}
    Rigid2(const Vector& translation, const double rotation)
        : translation_(translation), rotation_(rotation) {}

    static Rigid2 Rotation(const double rotation) {
        return Rigid2(Vector::Zero(), rotation);
    }

    static Rigid2 Rotation(const Rotation2D& rotation) {
        return Rigid2(Vector::Zero(), rotation);
    }

    static Rigid2 Translation(const Vector& vector) {
        return Rigid2(vector, Rotation2D::Identity());
    }

    static Rigid2<FloatType> Identity() { return Rigid2<FloatType>(); }

    template <typename OtherType>
    Rigid2<OtherType> cast() const {
        return Rigid2<OtherType>(translation_.template cast<OtherType>(),
            rotation_.template cast<OtherType>());
    }

    const Vector& translation() const { return translation_; }

    Rotation2D rotation() const { return rotation_; }

    Rigid2 inverse() const {
        const Rotation2D rotation = rotation_.inverse();
        const Vector translation = -(rotation * translation_);
        return Rigid2(translation, rotation);
    }

private:
    Vector translation_;
    Rotation2D rotation_;
};

template <typename FloatType>
Rigid2<FloatType> operator*(const Rigid2<FloatType>& lhs,
    const Rigid2<FloatType>& rhs) {
    return Rigid2<FloatType>(
        lhs.rotation() * rhs.translation() + lhs.translation(),
        lhs.rotation() * rhs.rotation());
}

template <typename FloatType>
typename Rigid2<FloatType>::Vector operator*(
    const Rigid2<FloatType>& rigid,
    const typename Rigid2<FloatType>::Vector& point) {
    return rigid.rotation() * point + rigid.translation();
}

using Rigid2d = Rigid2<double>;
using Rigid2f = Rigid2<float>;

//===============================

template <typename FloatType>
class Pose3
{
public:
    using Vector = Eigen::Matrix<FloatType, 3, 1>;
    using EulerAngle = Eigen::Matrix<FloatType, 3, 1>;
    using Quaternion = Eigen::Quaternion<FloatType>;
    using AngleAxis = Eigen::AngleAxis<FloatType>;

    Pose3() : translation_(Vector::Zero()), quat_(Quaternion::Identity()), euler_(Vector::Zero()) {}
    Pose3(const Vector& translation, const Quaternion& rotation)
        : translation_(translation), quat_(rotation)
        {
            euler_ = quat_.matrix().eulerAngles(2, 1, 0);
        }
    Pose3(const Vector &translation, const EulerAngle& rotation)
        : translation_(translation), euler_(rotation)
        {
            AngleAxis rollAngle(AngleAxis(euler_(2),Vector::UnitX()));
            AngleAxis pitchAngle(AngleAxis(euler_(1), Vector::UnitY()));
            AngleAxis yawAngle(AngleAxis(euler_(0), Vector::UnitZ()));
            quat_ = yawAngle * pitchAngle * rollAngle;
        }

    static Pose3 Rotation(const Quaternion& rotation) {
        return Pose3(Vector::Zero(), rotation);
    }

    static Pose3 Translation(const Vector& vector) {
        return Pose3(vector, Quaternion::Identity());
    }

    static Pose3 FromArrays(const std::array<FloatType, 4>& rotation,
        const std::array<FloatType, 3>& translation) {
        return Pose3(Eigen::Map<const Vector>(translation.data()),
            Eigen::Quaternion<FloatType>(rotation[0], rotation[1], rotation[2], rotation[3]));
    }

    static Pose3<FloatType> Identity() { return Pose3<FloatType>(); }

    const Vector& translation() const { return translation_; }
    const Quaternion& quaternion() const { return quat_; }
    const EulerAngle& eulerAngle() const { return euler_; }

    Pose3 inverse() const {
        const Quaternion rotation = quat_.conjugate();
        const Vector translation = -(rotation * translation_);
        return Pose3(translation, rotation);
    }

    double distance2d(const Pose3<FloatType> &target) const
    {
        auto a = translation();
        auto b = target.translation();
        return std::sqrt(std::pow(a(1) - b(1), 2) + std::pow(a(2) - b(2), 2));
    }

    double distance3d(const Pose3<FloatType> &target) const
    {
        auto a = translation();
        auto b = target.translation();
        return std::sqrt(std::pow(a(1) - b(1), 2) + std::pow(a(2) - b(2), 2) + std::pow(a(3) - b(3), 2));
    }

    EulerAngle deltaAngle(const Pose3<FloatType> &target) const
    {
        auto a = eulerAngle();
        auto b = target.eulerAngle();
        EulerAngle euler(b(0) - a(0), b(1) - a(1), b(2) - a(2));
        return euler;
    }

    EulerAngle absAngle(const Pose3<FloatType> &target) const
    {
        auto a = eulerAngle();
        auto b = target.eulerAngle();
        EulerAngle euler(std::abs(b(0) - a(0)), std::abs(b(1) - a(1)), std::abs(b(2) - a(2)));
        return euler;
    }

    double &NormalizeAngle(double &theta, double min, double max)
    {
        while (theta >= max)
            theta -= 2 * M_PI;
        while (theta < min)
            theta += 2 * M_PI;
        return theta;
    }
    void Normalize(const EulerAngle &target)
    {
        NormalizeAngle(target(2), roll_min, roll_max);
        NormalizeAngle(target(1), pitch_min, pitch_max);
        NormalizeAngle(target(0), yaw_min, yaw_max);
    }

private:
    Vector translation_;
    Quaternion quat_;
    EulerAngle euler_;

    // set min max
    static double roll_min;
    static double roll_max;
    static double pitch_min;
    static double pitch_max;
    static double yaw_min;
    static double yaw_max;
};
template <typename FloatType> double Pose3<FloatType>::roll_min = -M_PI;
template <typename FloatType> double Pose3<FloatType>::roll_max = M_PI;
template <typename FloatType> double Pose3<FloatType>::pitch_min = -M_PI;
template <typename FloatType> double Pose3<FloatType>::pitch_max = M_PI;
template <typename FloatType> double Pose3<FloatType>::yaw_min = -M_PI;
template <typename FloatType> double Pose3<FloatType>::yaw_max = M_PI;


template <typename FloatType>
Pose3<FloatType> operator*(const Pose3<FloatType>& lhs,
    const Pose3<FloatType>& rhs) {
    return Pose3<FloatType>(
        lhs.quaternion() * rhs.translation() + lhs.translation(),
        (lhs.quaternion() * rhs.quaternion()).normalized());
}

template <typename FloatType>
typename Pose3<FloatType>::Vector operator*(
    const Pose3<FloatType>& rigid,
    const typename Pose3<FloatType>::Vector& point) {
    return rigid.quaternion() * point + rigid.translation();
}

using Pose3d = Pose3<double>;
using Pose3f = Pose3<float>;
