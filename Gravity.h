// Gravity.h – Simulation core for N-body gravitational interaction
// This header defines the Body struct and the Gravity class which implements the physics simulation

#ifndef GRAVITY_H
#define GRAVITY_H

#include <utility>
#include <limits>
#include <cmath>
#include <algorithm>
#include <vector>
#include "GravityMath.h"

// Represents a single body in 2D space
struct Body
{
    double x, y;   // Position
    double vx, vy; // Velocity
    double ax, ay; // Acceleration
    double mass;   // Mass
};

// Encapsulates the physics simulation for up to 10 bodies
class Gravity
{
public:
    Gravity();  // Constructor 
    ~Gravity(); // Destructor
    static const int BodyCount = 10; // Number of bodies in the system

    void loadPreset(int presetIndex); // Load a predefined body configuration (0–13)

    void setG(double g);                                // Set gravitational constant
    void setDt(double d);                               // Set simulation time step
    void setPosDamping(double damp);                    // Set position-based damping coefficient
    void setVelDamping(double vd);                      // Set velocity-based damping coefficient
    void setSoftening(double s);                        // Set base softening value to prevent singularities
    void setVmin(double v);                             // Set the minimum velocity
    void setVmax(double v);                             // Set the maximum velocity
    void setBodyCount(int count);                       // Set how many bodies are active (2–10)
    void setBodyMass(int index, double mass);           // Sets a bodies mass at simulation time
    void setBlackHole(double x, double y, double mass); // Sets position and mass for the black hole

    void nudge(); // Nudges the Bodies when they got stuck

    double getG() const { return G; }                    // Get gravitational constant
    double getDt() const { return dt; }                  // Get simulation time step
    double getVmin() const { return vmin; }              // Gets the minimum velocity
    double getVmax() const { return vmax; }              // Gets the maximumn velocity
    double getPosDamping() const { return pos_damping; } // Get position damping coefficient
    double getVelDamping() const { return vel_damping; } // Get velocity damping coefficient
    double getSoftening() const { return softening; }    // Get base softening value
    int getBodyCount() const { return body_count; }      // Get current number of active bodies

    const Body &getBlackHole() const;         // Gets the black hole
    const Body &getBody(int index) const;     // Get body by index (current state)
    std::vector<Body> getBodies() const;      // Returns a copy of all current body states for thread safety
    const Body &getInitBody(int index) const; // Get initial body state by index

    void setBody(int index, double x, double y, double vx, double vy, double mass); // Set initial values for a body

    void reset();    // Reset all bodies to initial state and reinitialize
    void simulate(); // Perform one simulation step

private:
    GravityMath *math;                 // Physics calculations
    void initParams();                 // Initialize default simulation parameters
    void resetBodies();                // Resets every body value to 0
    void initBody(int index);          // Initialize a single body’s acceleration
    double computeAdaptiveDt() const;  // Adaptive timestep depending on proximity
    void applyMinSpeed();              // Minimal velocity calculation

    // This helps prevent them from sticking together by applying a distance-based counter-force.
    void applyCloseBodyRepulsion(int index, double vmin, double amin, double repel_zone, double repel_max);

    Vector computeAcceleration(int targetIndex) const; // Calculate acceleration on one body

    Body initBodies[BodyCount]; // Array holding initial body states
    Body bodies[BodyCount];     // Array holding current body states
    Body blackHole;             // The black hole

    double G;           // Gravitational constant
    double dt;          // Timestep
    double pos_damping; // Damping based on distance from origin
    double vel_damping; // Damping based on body speed
    double softening;   // Base value to prevent singularities
    double vmin;        // Minimum vewlociy
    double vmax;        // Maximum velocity
    int body_count;     // Number of active bodies
    bool nudge_mode;    // nudge indicator for simulation
    int nudge_step;     // Current simulation step in nudging mode
};

#endif // GRAVITY_H
