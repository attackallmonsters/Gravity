// g.cpp – Implementation of N-body gravitational simulation core
// This file implements the g class defined in g.h

#include "Gravity.h"
#include "GravityMath.h"
#include "m_pd.h" // For pd_error and post (Pure Data logging)

extern t_class *grav_class; // External reference for error logging

Gravity::Gravity()
{
    math = new GravityMath();
    body_count = 3;
    nudge_mode = false;
    nudge_step = 0;

    initParams();
    loadPreset(0);
    post("");
    post("       .    o");
    post("        \\  /|\\        .");
    post("      o---> * <---o");
    post("        /  \\|/        o");
    post("       .    o     .");
    post("");
    post("gravitational system online - chaos engaged");
}

// Destructor
Gravity::~Gravity()
{
    delete math;
}

// Initializes simulation parameters
void Gravity::initParams()
{
    G = 1.0;
    dt = 0.01;
    pos_damping = 0.003;
    vel_damping = 0.005;
    softening = 0.0;
    vmin = 1;
    vmax = 5;
}

// Zeroes all body values
void Gravity::resetBodies()
{
    for (int i = 0; i < BodyCount; ++i)
    {
        bodies[i] = Body{};
        initBodies[i] = Body{};
    }
}

// Sets the gravity constant
void Gravity::setG(double g)
{
    if (g < 0.0 || g > 10.0)
    {
        pd_error(grav_class, "[grav] G must be in (0.1, 10], got %f", g);
        return;
    }

    G = (g < 0.1f) ? 0.1 : g;
}

// Sets the delta time between two simulations steps
void Gravity::setDt(double dt)
{
    if (dt <= 0.0 || dt > 0.1)
    {
        pd_error(grav_class, "[grav] dt must be in (0.001, 0.1], got %f", dt);
        return;
    }
    this->dt = dt;
}

// Sets the position damping factor
void Gravity::setPosDamping(double damp)
{
    if (damp < 0.0 || damp > 0.1)
    {
        pd_error(grav_class, "[grav] posdamp must be in [0.0, 0.1], got %f", damp);
        return;
    }

    pos_damping = damp / 10000;
}

// Sets the velocity damping factor
void Gravity::setVelDamping(double damp)
{
    if (damp < 0.0 || damp > 0.5)
    {
        pd_error(grav_class, "[grav] veldamp must be in [0.0, 0.5], got %f", damp);
        return;
    }

    vel_damping = damp;
}

// Set base softening value to prevent singularities
void Gravity::setSoftening(double s)
{
    if (s < 0.0 || s > 5.0)
    {
        pd_error(grav_class, "[grav] softening must be in (0.0, 5.0], got %f", s);
        return;
    }

    softening = s;
}

// Set the minimal velocity
void Gravity::setVmin(double v)
{
    if (v < 0.1 || v > 1000.0)
    {
        pd_error(grav_class, "[grav] vmin must be in (0.1, 1000.0], got %f", v);
        return;
    }

    vmin = v;
    if (vmax < v)
        vmax = v;
}

// Set the maximal velocity
void Gravity::setVmax(double v)
{
    if (v < 1.0 || v > 10000.0)
    {
        pd_error(grav_class, "[grav] vmax must be in (1.0, 10000.0], got %f", v);
        return;
    }

    vmax = v;

    if (vmin > v)
        vmin = v;
}

// Sets the number of active bodies
void Gravity::setBodyCount(int count)
{
    if (count < 2 || count > BodyCount)
    {
        pd_error(grav_class, "[grav] count must be between 2 and %d, got %d", BodyCount, count);
        return;
    }

    body_count = count;
}

// Sets a bodies mass at simulation time
void Gravity::setBodyMass(int index, double mass)
{
    if (index < 0 || index > BodyCount - 1)
    {
        pd_error(grav_class, "[grav] index must be between 0 and %d, got %d", BodyCount - 1, index);
        return;
    }

    if (mass < 0.1 || mass > 30)
    {
        pd_error(grav_class, "[grav] mass must be between 0.1 and 30, got %f", mass);
        return;
    }

    Body &body = bodies[index];
    body.mass = mass;
}

// Sets position and mass for the black hole
void Gravity::setBlackHole(double x, double y, double mass)
{
    if (x < -500 || x > 500)
    {
        pd_error(grav_class, "[grav] black hole x must be between -500 and 500, got %f", x);
        return;
    }

    if (y < -500 || y > 500)
    {
        pd_error(grav_class, "[grav] black hole y must be between -500 and 500, got %f", y);
        return;
    }

    if (mass < 0 || mass > 10000)
    {
        pd_error(grav_class, "[grav] black hole mass must be between 0 and 10000, got %f", mass);
        return;
    }

    blackHole.x = x;
    blackHole.y = y;
    blackHole.vx = 0;
    blackHole.vy = 0;
    blackHole.ax = 0;
    blackHole.ay = 0;

    blackHole.mass = mass;
}

// Gets the body with a given index
const Body &Gravity::getBody(int index) const
{
    if (index < 0 || index > BodyCount - 1)
    {
        pd_error(grav_class, "[grav] index must be between 0 and %d, got %d => 1. body returned", BodyCount - 1, index);
        return bodies[0];
    }

    return bodies[index];
}

// Returns a copy of all current body states for thread safety
std::vector<Body> Gravity::getBodies() const
{
    return std::vector<Body>(bodies, bodies + BodyCount); // std::vector<Body>
}

// Gets the body with a given index
const Body &Gravity::getInitBody(int index) const
{
    if (index < 0 || index > BodyCount - 1)
    {
        pd_error(grav_class, "[grav] nr must be between 0 and %d, got %d => 1. body returned", BodyCount - 1, index);
        return initBodies[0];
    }

    return initBodies[index];
}

// initializes a bodies starting values
void Gravity::initBody(int index)
{
    Body &body = bodies[index];

    // Compute initial acceleration from all other bodies
    Vector v = computeAcceleration(index);

    // Calculate distance to origin to appy position damping
    double pdamp = math->calcPositionDamping(body.x, body.y, pos_damping);
    body.ax = v.x - body.x * pdamp;
    body.ay = v.y - body.y * pdamp;
}

void Gravity::setBody(int index, double x, double y, double vx, double vy, double mass)
{
    // Validate index range to avoid out-of-bounds access
    if (index < 0 || index > BodyCount - 1)
        return;

    Body &body = bodies[index];
    Body &iBody = initBodies[index];

    body.x = x;
    body.y = y;
    body.vx = vx;
    body.vy = vy;
    body.mass = mass;

    iBody.x = x;
    iBody.y = y;
    iBody.vx = vx;
    iBody.vy = vy;
    iBody.mass = mass;
    iBody.ax = 0;
    iBody.ay = 0;

    initBody(index);
}

// resets the bodies to init values
void Gravity::reset()
{
    for (int i = 0; i < 10; ++i)
    {
        Body &body = bodies[i];
        Body &iBody = initBodies[i];

        body.x = iBody.x;
        body.y = iBody.y;
        body.vx = iBody.vx;
        body.vy = iBody.vy;
        body.ax = iBody.ax;
        body.ay = iBody.ay;

        initBody(i);
    }
}

// Gets the black hole
const Body &Gravity::getBlackHole() const
{
    return blackHole;
}

// Nudges the Bodies when they got stuck
void Gravity::nudge()
{
    nudge_mode = true;
}

// Computes a reduced time step when bodies get very close to each other.
// Ensures more simulation detail during close encounters.
double Gravity::computeAdaptiveDt() const
{
    double minDist = std::numeric_limits<double>::max();

    for (int i = 0; i < body_count; ++i)
    {
        for (int j = i + 1; j < body_count; ++j)
        {
            double dist = math->calcEuclideanDistance(bodies[i].x, bodies[i].y, bodies[j].x, bodies[j].y);
            if (dist < minDist)
                minDist = dist;
        }
    }

    double scale = 0.8f + 0.8f * std::tanh(minDist * 0.8);

    double dampDt = dt * scale;

    return std::max(dampDt, 0.001);
}

// Minimal velocity calculation
void Gravity::applyMinSpeed()
{
    for (int i = 0; i < body_count; ++i)
    {
        Body &body = bodies[i];

        // Skip near center
        double r = math->calcRadiusFromCenter(body.x, body.y);

        if (r < 100.0 * 100.0)
        {
            continue;
        }

        double v = math->calcSpeed(body.vx, body.vy);
        double a = math->calcAcceleration(body.ax, body.ay);

        if (v < vmin && a < 0.01f)
        {
            // Random angle in [0, 2π)
            Vector v = math->randomImpulse(0.02, 0.07);
            body.vx += v.x;
            body.vy += v.y;
        }
    }
}

// This helps prevent them from sticking together by applying a distance-based counter-force.
void Gravity::applyCloseBodyRepulsion(int index, double vmin, double amin, double repel_zone, double repel_max)
{
    if (index < 0 || index >= body_count)
        return;

    Body &body = bodies[index];

    // Skip near center
    double r = math->calcRadiusFromCenter(body.x, body.y);

    if (r < 100.0 * 100.0)
    {
        return;
    }

    // Check if body is stagnating
    double v = math->calcSpeed(body.vx, body.vy);
    double acc = math->calcAcceleration(body.ax, body.ay);

    bool isStagnating = (v < vmin && acc < amin);
    if (!isStagnating)
        return;

    // Strong random impulse to break deadlocks
    double angle = ((double)rand() / RAND_MAX) * 2.0 * M_PI;
    double impulse = 0.02 + ((double)rand() / RAND_MAX) * 0.05;

    body.vx += impulse * std::cos(angle);
    body.vy += impulse * std::sin(angle);

    // Repulsion from nearby bodies
    for (int j = 0; j < body_count + 1; ++j)
    {
        if (j == index)
            continue;

        Body &nbbody = (j < body_count) ? bodies[j] : blackHole;

        if (j == body_count && blackHole.mass == 0)
            continue;

        Vector v = math->calcRelativePositionVector(nbbody.x, nbbody.y, body.x, body.y);

        if (std::abs(v.x) > repel_zone || std::abs(v.y) > repel_zone)
            continue;

        double dist_sqr = v.x * v.x + v.y * v.y;
        if (dist_sqr >= repel_zone * repel_zone)
            continue;

        double dist = std::sqrt(dist_sqr) + 1e-6;
        double norm = 1.0 / dist;

        double factor = (repel_zone - dist) / repel_zone;
        double base_strength = repel_max * factor * factor;

        // Stronger jitter: ±100% of base strength
        double jitter = ((double)rand() / RAND_MAX - 0.5) * base_strength * 2.0;

        double fx = (base_strength + jitter) * v.x * norm;
        double fy = (base_strength + jitter) * v.y * norm;

        body.ax -= fx;
        body.ay -= fy;
    }
}

// Computes the gravitational acceleration on the body at targetIndex
// from all other bodies, including softening to avoid singularities.
Vector Gravity::computeAcceleration(int targetIndex) const
{
    const Body &target = bodies[targetIndex];
    double ax = 0, ay = 0;

    for (int i = 0; i < body_count + 1; ++i)
    {
        // Skip self-interaction
        if (i == targetIndex)
            continue;

        const Body &other = (i < body_count) ? bodies[i] : blackHole;

        if (i == body_count && blackHole.mass == 0)
            continue;

        // Compute relative position vector
        Vector v = math->calcRelativePositionVector(target.x, target.y, other.x, other.y);

        // Compute Euclidean distance between target and other
        double distance = math->calcEuclideanDistance(v.x, v.y);

        // Apply softening to reduce numerical instability at short ranges
        double currentSoftening = std::max(softening, distance * softening);

        // Compute softened squared distance for force calculation
        double distSqr = v.x * v.x + v.y * v.y + currentSoftening * currentSoftening;

        if (distSqr <= 0.0 || distSqr < 0.0001)
        {
            continue;
        }

        // Compute inverse distance and its cube
        double invDist = 1.0 / std::sqrt(distSqr);
        double invDist3 = invDist * invDist * invDist;

        // Accumulate gravitational acceleration components
        ax += G * other.mass * v.x * invDist3;
        ay += G * other.mass * v.y * invDist3;
    }

    // Return the total acceleration vector acting on the target body
    Vector v;
    v.x = ax;
    v.y = ay;
    return v;
}

// Implementation of the ThreeBodySystem methods
// Performs one simulation step using the Leapfrog integration method.
// Updates positions, calculates new accelerations, and updates velocities with damping.
void Gravity::simulate()
{
    double currentDt = computeAdaptiveDt();

    for (int i = 0; i < body_count; ++i)
    {
        Body &body = bodies[i];
        body.x += body.vx * currentDt + 0.5 * body.ax * currentDt * currentDt;
        body.y += body.vy * currentDt + 0.5 * body.ay * currentDt * currentDt;
    }

    // Store current accelerations to be used in velocity update
    double oldAx[body_count], oldAy[body_count];
    for (int i = 0; i < body_count; ++i)
    {
        oldAx[i] = bodies[i].ax;
        oldAy[i] = bodies[i].ay;
    }

    for (int i = 0; i < body_count; ++i)
    {
        Body &body = bodies[i];

        // Compute new acceleration including gravitational and position damping
        Vector v = computeAcceleration(i);

        // Damping increases with distance to prevent runaway trajectories
        double pdamp = math->calcPositionDamping(body.x, body.y, pos_damping);
        body.ax = v.x - body.x * pdamp;
        body.ay = v.y - body.y * pdamp;

        applyCloseBodyRepulsion(i, 0.02, 0.001, 1.0, 0.1);
    }

    // new positions
    for (int i = 0; i < body_count; ++i)
    {
        Body &body = bodies[i];

        // Velocity update using averaged acceleration (Leapfrog step 2)
        body.vx += 0.5 * (oldAx[i] + body.ax) * currentDt;
        body.vy += 0.5 * (oldAy[i] + body.ay) * currentDt;

        if (nudge_mode)
        {
            if (nudge_step == 20)
            {
                nudge_mode = false;
                nudge_step = 0;
            }

            double nudge_factor = 10 * (5.0 + pos_damping);
            Vector v = math->randomImpulse(-nudge_factor / 2, nudge_factor / 2);
            body.vx = v.x;
            body.vy = v.y;

            nudge_step++;
        }

        // Compute velocity magnitude for dynamic velocity damping
        double speed = math->calcSpeed(body.vx, body.vy);

        // Velocity damping increases with speed to limit energy escalation
        double vdamp = vel_damping * (1.0 + speed);
        body.vx *= 1.0 - vdamp;
        body.vy *= 1.0 - vdamp;

        // Clamp velocity to minimum and maximum thresholds
        Vector v = math->clampSpeed(body.vx, body.vy, vmin, vmax);
        body.vx = v.x;
        body.vy = v.y;
    }

    applyMinSpeed();
}

// Loads one of ten predefined body configurations and sets active body count.
void Gravity::loadPreset(int presetIndex)
{
    // Clamp preset index to valid range
    int p = presetIndex < 1 ? 1 : (presetIndex > 14 ? 14 : presetIndex);

    p--;

    resetBodies();

    switch (p)
    {
    case 0:
        // Simple rotating ring around massive center
        setG(0.1);
        setDt(0.01);
        setSoftening(0);
        setPosDamping(0.02);
        setVelDamping(0.005);
        setBodyCount(10);

        setBody(0, 50, 0, 0, 0.4, 1);
        setBody(1, 40, 40, -0.3, 0.3, 1);
        setBody(2, 0, 50, -0.4, 0.0, 1);
        setBody(3, -40, 40, -0.3, -0.3, 1);
        setBody(4, -50, 0, 0, -0.4, 1);
        setBody(5, -40, -40, 0.3, -0.3, 1);
        setBody(6, 0, -50, 0.4, 0.0, 1);
        setBody(7, 40, -8, 0.3, 0.3, 1);
        setBody(8, 0, 0, 0.0, 0.0, 3);
        setBody(9, 0, 30, 0.0, 0.0, 0.5);
        break;
    case 1:
        // Asymmetric cluster with slow drift
        setG(0.1);
        setDt(0.01);
        setSoftening(0);
        setPosDamping(0.02);
        setVelDamping(0.01);
        setBodyCount(10);

        setBody(0, 50, 20, 0.1, 0.05, 1);
        setBody(1, -40, -10, -0.1, 0.1, 1);
        setBody(2, -60, 70, 0.1, -0.05, 1);
        setBody(3, 30, -60, -0.1, -0.1, 1);
        setBody(4, 10, 10, 0.0, 0.0, 2);
        setBody(5, -10, -40, 0.05, 0.0, 0.8);
        setBody(6, 0, -70, 0.0, 0.1, 0.8);
        setBody(7, -80, 20, 0.1, 0.0, 0.8);
        setBody(8, 70, -20, -0.05, 0.1, 0.8);
        setBody(9, 0, 0, 0.0, 0.0, 3);
        break;
    case 2:
        // Spiral start with mild rotation
        setG(0.1);
        setDt(0.01);
        setSoftening(0);
        setPosDamping(0.02);
        setVelDamping(0.01);
        setBodyCount(10);

        for (int i = 0; i < 10; ++i)
        {
            double angle = i * 0.6;
            double radius = 20.0f * i;
            double x = std::cos(angle) * radius;
            double y = std::sin(angle) * radius;
            double vx = -std::sin(angle) * 0.2;
            double vy = std::cos(angle) * 0.2;
            setBody(i, x, y, vx, vy, 1.0);
        }
        break;
    case 3:
        // Symmetrical cross
        setG(0.3);
        setDt(0.008);
        setSoftening(0);
        setPosDamping(0.02);
        setVelDamping(0.01);
        setBodyCount(10);

        for (int i = 0; i < 5; ++i)
        {
            setBody(i, 0, i * 50.0f - 100.0, 0.2, 0, 1.0);
            setBody(i + 5, i * 50.0f - 100.0, 0, 0, -0.2, 1.0);
        }
        break;
    case 4:
        // Circular orbit with center mass
        setG(0.1);
        setDt(0.02);
        setSoftening(0.5);
        setPosDamping(0.02);
        setVelDamping(0.002);
        setBodyCount(10);

        for (int i = 0; i < 10; ++i)
        {
            double angle = 2 * M_PI * i / 9.0;
            double x = 100.0f * std::cos(angle);
            double y = 100.0f * std::sin(angle);
            double vx = -std::sin(angle) * 0.5;
            double vy = std::cos(angle) * 0.5;
            setBody(i, x, y, vx, vy, 1.0);
        }
        setBody(9, 0, 0, 0, 0, 5.0);
        break;
    case 5:
        // Random cluster
        setG(0.5);
        setDt(0.01);
        setSoftening(0.4);
        setPosDamping(0.02);
        setVelDamping(0.01);
        setBodyCount(BodyCount);

        for (int i = 0; i < BodyCount; ++i)
        {
            double x = (rand() % 200) - 100;
            double y = (rand() % 200) - 100;
            double vx = ((rand() % 200) - 100) * 0.005;
            double vy = ((rand() % 200) - 100) * 0.005;
            setBody(i, x, y, vx, vy, 0.5f + (rand() % 100) * 0.01);
        }
        break;
    case 6:
        // Two binary systems plus orbiters
        setG(0.2);
        setDt(0.01);
        setSoftening(0);
        setPosDamping(0.02);
        setVelDamping(0.005);
        setBodyCount(10);

        setBody(0, -50, 0, 0, 0.3, 1);
        setBody(1, -30, 0, 0, -0.3, 1);
        setBody(2, 50, 0, 0, -0.3, 1);
        setBody(3, 30, 0, 0, 0.3, 1);
        setBody(4, 0, 80, -0.3, 0, 1);
        setBody(5, 0, 60, 0.3, 0, 1);
        setBody(6, 0, -60, 0.3, 0, 1);
        setBody(7, 0, -80, -0.3, 0, 1);
        setBody(8, 0, 0, 0, 0, 2);
        setBody(9, 0, 20, 0, 0, 0.5);
        break;
    case 7:
        // Figure-eight approximation
        setG(1.0);
        setDt(0.005);
        setSoftening(0.01);
        setPosDamping(0.0);
        setVelDamping(0.0);
        setBodyCount(3);

        setBody(0, 0, 0, 0.347111, 0.532728, 1);
        setBody(1, 0.970004, -0.243087, -0.347111, 0.532728, 1);
        setBody(2, -0.970004, 0.243087, 0, -1.065456, 1);
        for (int i = 3; i < BodyCount; ++i)
        {
            setBody(i, 0, 0, 0, 0, 0);
        }
        break;
    case 8:
        // Line of increasing mass and spacing
        setG(0.15);
        setDt(0.01);
        setSoftening(0.2);
        setPosDamping(0.02);
        setVelDamping(0.005);
        setBodyCount(BodyCount);

        for (int i = 0; i < BodyCount; ++i)
        {
            double x = i * 50.0;
            double y = 0;
            double vx = 0;
            double vy = (i - 5) * 0.1;
            double mass = 0.5f + 0.5f * i;
            setBody(i, x, y, vx, vy, mass);
        }
        break;
    case 9:
        // Radial outburst from center
        setG(0.2);
        setDt(0.008);
        setSoftening(0);
        setPosDamping(0.02);
        setVelDamping(0.01);
        setBodyCount(BodyCount);

        for (int i = 0; i < BodyCount; ++i)
        {
            double angle = 2 * M_PI * i / 10.0;
            double vx = std::cos(angle) * 0.3;
            double vy = std::sin(angle) * 0.3;
            setBody(i, 0, 0, vx, vy, 1.0);
        }
        break;
    case 10:
        // Asymetric chaos at high speed
        setG(0.15);
        setDt(0.01);
        setSoftening(0.05);
        setPosDamping(0.02);
        setVelDamping(0.0005);
        setBodyCount(5);

        setBody(0, -120, 80, 0.9, -0.4, 1.5);
        setBody(1, 100, 60, -0.5, 0.6, 2.0);
        setBody(2, 0, -100, 0.4, 0.8, 1.2);
        setBody(3, 50, 50, -0.9, -0.2, 0.8);
        setBody(4, -70, -80, 0.6, 0.3, 1.0);
        break;
    case 11:
        // Chaos cluster drift
        setG(0.2);
        setDt(0.008);
        setSoftening(0.05);
        setPosDamping(0.02);
        setVelDamping(0.001);
        setBodyCount(6);

        setBody(0, -40, 20, 0.5, 0.4, 1.0);
        setBody(1, 30, -10, -0.6, 0.3, 1.8);
        setBody(2, 0, 0, 0.1, -0.5, 0.6);
        setBody(3, -30, -30, 0.3, 0.6, 1.2);
        setBody(4, 60, 10, -0.4, -0.3, 1.5);
        setBody(5, -50, 40, 0.7, -0.1, 0.9);
        break;
    case 12:
        // Chaos extreme
        setG(0.25);
        setDt(0.007);
        setSoftening(0.07);
        setPosDamping(0.02);
        setVelDamping(0.0002);
        setBodyCount(7);

        setBody(0, -200, 100, 1.0, -0.3, 1.2);
        setBody(1, 180, 80, -0.8, 0.6, 2.1);
        setBody(2, 0, -90, 0.5, 0.9, 0.7);
        setBody(3, 60, 200, -1.1, -0.2, 1.4);
        setBody(4, -160, -150, 0.9, 0.4, 1.0);
        setBody(5, 30, -70, -0.3, -0.8, 0.8);
        setBody(6, 90, 0, -0.5, 0.5, 1.6);
        break;
    case 13: // Chaos  – scattered triangle with tangential velocity
        setG(0.15);
        setDt(0.009);
        setSoftening(0.05);
        setPosDamping(0.02);
        setVelDamping(0.0005);
        setBodyCount(3);

        // triangle
        setBody(0, -100.0, -50.0, 0.65, 0.3, 1.2);
        setBody(1, 100.0, -50.0, -0.6, 0.35, 1.8);
        setBody(2, 0.0, 120.0, -0.05, -0.7, 2.0);
        break;
    default:
        // Default: all bodies at origin, mass 0
        break;
    }
}
