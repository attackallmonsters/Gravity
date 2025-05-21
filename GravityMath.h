#ifndef GRAVITYMATH_H
#define GRAVITYMATH_H

#include <utility>

// Holds delta
struct Vector
{
    double x;
    double y;
};

class GravityMath
{
public:
    GravityMath();

    // Calculates the radius of a body relative to the center
    double calcRadiusFromCenter(double x, double y);
    // Calulates the relative position [deltax deltay] of two positions
    Vector calcRelativePositionVector(double x1, double y1, double x2, double y2) const;
    // Calculates the Euclidean distance based on deltax, deltay
    double calcEuclideanDistance(double dx, double dy) const;
    // Calculates the Euclidean distance from coordinates
    double calcEuclideanDistance(double x1, double y1, double x2, double y2) const;
    // Computes position-based damping factor (grows with distance from origin)
    double calcPositionDamping(double x, double y, double baseDamping) const;
    // Computes velocity magnitude (speed) from vx/vy
    double calcSpeed(double vx, double vy) const;
    // Computes acceleration from vx/vy
    double calcAcceleration(double ax, double ay) const;
    // Generates a random angle between 0 and 2π
    double randomAngle() const;
    // Generates a random double between min and max
    double randomRange(double min, double max) const;
    // Returns a velocity vector with random direction and magnitude
    Vector randomImpulse(double minStrength, double maxStrength) const;
    // Clamps the given velocity to a min/max speed, returns scaled pair
    Vector clampSpeed(double vx, double vy, double vmin, double vmax) const;

protected:
private:
};

#endif // GRAVITYMATH_H