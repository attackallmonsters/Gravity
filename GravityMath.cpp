#include <cstdlib>
#include <utility>
#include <cmath>
#include "GravityMath.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GravityMath::GravityMath()
{
}

// Calculates the radius of a body relative to the center
double GravityMath::calcRadiusFromCenter(double x, double y)
{
    return x * x + y * y;
}

// Calulates the relative position [deltax deltay] of two positions
Vector GravityMath::calcRelativePositionVector(double x1, double y1, double x2, double y2) const
{
    double dx = x2 - x1;
    double dy = y2 - y1;

    Vector v;
    v.x = dx;
    v.y = dy;
    return v;
}

// Calculates the Euclidean distance of deltax, deltay
double GravityMath::calcEuclideanDistance(double dx, double dy) const
{
    return std::sqrt(dx * dx + dy * dy);
}

// Calculates the Euclidean distance of two points
double GravityMath::calcEuclideanDistance(double x1, double y1, double x2, double y2) const
{
    Vector v = calcRelativePositionVector(x1, y1, x2, y2);
    return calcEuclideanDistance(v.x, v.y);
}

// Computes position damping factor (based on distance from origin)
double GravityMath::calcPositionDamping(double x, double y, double baseDamping) const
{
    double dist = std::sqrt(x * x + y * y);
    return baseDamping * (1.0 + dist);
}

// Computes velocity magnitude (speed)
double GravityMath::calcSpeed(double vx, double vy) const
{
    return std::sqrt(vx * vx + vy * vy);
}

// Computes the acceleration
double GravityMath::calcAcceleration(double ax, double ay) const
{
    return std::sqrt(ax * ax + ay * ay);
}

// Generates random angle in [0, 2π)
double GravityMath::randomAngle() const
{
    return static_cast<double>(rand()) / RAND_MAX * 2.0f * M_PI;
}

// Generates random double in [min, max)
double GravityMath::randomRange(double min, double max) const
{
    return min + static_cast<double>(rand()) / RAND_MAX * (max - min);
}

// Returns a random impulse vector (vx, vy) with random direction and strength
Vector GravityMath::randomImpulse(double minStrength, double maxStrength) const
{
    double angle = randomAngle();
    double strength = randomRange(minStrength, maxStrength);

    Vector v;
    v.x = strength * std::cos(angle);
    v.y = strength * std::sin(angle);
    return v;
}

// Clamps velocity to a range [vmin, vmax]
Vector GravityMath::clampSpeed(double vx, double vy, double vmin, double vmax) const
{
    double speed = calcSpeed(vx, vy);

    if (speed == 0.0f)
        return {vx, vy};

    if (speed < vmin)
    {
        double scale = vmin / speed;
        return {vx * scale, vy * scale};
    }

    if (speed > vmax)
    {
        double scale = vmax / speed;
        return {vx * scale, vy * scale};
    }

    Vector v;
    v.x = vx;
    v.y = vy;
    return v;
}
