// Mars lander simulator
// Version 1.11
// Mechanical simulation functions
// Gabor Csanyi and Andrew Gee, August 2019

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation, to make use of it
// for non-commercial purposes, provided that (a) its original authorship
// is acknowledged and (b) no modified versions of the source code are
// published. Restriction (b) is designed to protect the integrity of the
// exercise for future generations of students. The authors would be happy
// to receive any suggested modifications by private correspondence to
// ahg@eng.cam.ac.uk and gc121@eng.cam.ac.uk.

#include "lander.h"
#include <vector>

bool system_engaged = false;
double initial_altitude;
int first_iteration = 1;

void autopilot (double g_force, double mass)
  // Autopilot to adjust the engine throttle, parachute and attitude control
{
    static double vel_engaged;
    double vel_radial, Kp, Kh, e, power_output, weight_throttle, h, alt_engage, chute_engage;
    
    Kp = 0.05;
    h = position.abs() - MARS_RADIUS;
    alt_engage = initial_altitude * 0.5;
    weight_throttle = g_force / MAX_THRUST;
    vel_radial = velocity * position.norm();
    chute_engage = alt_engage - (initial_altitude - 10000) / 1.943;
    
    if ((h < chute_engage) && parachute_status == NOT_DEPLOYED) {
        
        if (safe_to_deploy_parachute()) {
            parachute_status = DEPLOYED;
        }
       
    }
    
    
    if (!system_engaged) {

        if (h < alt_engage) {
            system_engaged = true;
            vel_engaged = vel_radial;
        }

    }
    
    else {
        
        Kh = -((0.7) * (1 / Kp) + 0.5 + vel_engaged) / alt_engage;
        e = -(0.5 + Kh * h + vel_radial);
        power_output = Kp * e;

        if (power_output <= -weight_throttle) {
            throttle = 0;
        }

        if ((-weight_throttle < power_output) && (power_output < 1 - weight_throttle)) {
            throttle = weight_throttle + power_output;
        }

        else {
            throttle = 1;
        }
    
    }
      
}


void numerical_dynamics (void)
  // This is the function that performs the numerical integration to update the
  // lander's pose. The time step is delta_t (global variable).
{
    double density, mass;
    vector3d g_force, d_force, t_force, a;
    static vector3d previous_position, current_position;
    
    density = atmospheric_density(position);
    mass = UNLOADED_LANDER_MASS + fuel * FUEL_DENSITY * FUEL_CAPACITY;
    g_force = - (GRAVITY * MARS_MASS * (mass) / position.abs2()) * position.norm();
    d_force = - 0.5 * density * DRAG_COEF_LANDER * 3.14159265 * velocity.abs2() * velocity.norm();
    t_force = thrust_wrt_world();

    if (parachute_status == DEPLOYED) {
        d_force += -0.5 * density * DRAG_COEF_CHUTE * 20 * velocity.abs2() * velocity.norm();
    }

    a = (g_force + d_force + t_force) / mass;
    
    /* 
    Euler Integration: Not In Use
    position = position + delta_t * velocity;
    velocity = velocity + delta_t * a; 
    */
    
    // Verlet Integrator: In Use
    if (first_iteration == 1) {
        previous_position = position;
        position = previous_position + velocity * delta_t + 0.5 * delta_t * delta_t * a;
        velocity = (2 * position - previous_position + a * delta_t * delta_t - previous_position) * 0.5 * 1 / delta_t;
        first_iteration = 0;
    }

    else {
        current_position = position;
        position = 2 * current_position - previous_position + a * delta_t * delta_t;
        previous_position = current_position;
        velocity = (2 * position - previous_position + a * delta_t * delta_t - previous_position) * 0.5 * 1 / delta_t;
    }
    
  // Here we can apply an autopilot to adjust the thrust, parachute and attitude
  if (autopilot_enabled) autopilot(g_force.abs(), mass);

  // Here we can apply 3-axis stabilization to ensure the base is always pointing downwards
  if (stabilized_attitude) attitude_stabilization();
}

void initialize_simulation (void)
  // Lander pose initialization - selects one of 10 possible scenarios
{
  // The parameters to set are:
  // position - in Cartesian planetary coordinate system (m)
  // velocity - in Cartesian planetary coordinate system (m/s)
  // orientation - in lander coordinate system (xyz Euler angles, degrees)
  // delta_t - the simulation time step
  // boolean state variables - parachute_status, stabilized_attitude, autopilot_enabled
  // scenario_description - a descriptive string for the help screen
    
  scenario_description[0] = "circular orbit";
  scenario_description[1] = "descent from 10km";
  scenario_description[2] = "elliptical orbit, thrust changes orbital plane";
  scenario_description[3] = "polar launch at escape velocity (but drag prevents escape)";
  scenario_description[4] = "elliptical orbit that clips the atmosphere and decays";
  scenario_description[5] = "descent from 200km";

  first_iteration = 1;
  system_engaged = false;

  switch (scenario) {

  case 0:
    // a circular equatorial orbit
    position = vector3d(1.2*MARS_RADIUS, 0.0, 0.0);
    velocity = vector3d(0.0, -3247.087385863725, 0.0);
    orientation = vector3d(0.0, 90.0, 0.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 1:
    // a descent from rest at 10km altitude
    position = vector3d(0.0, -(MARS_RADIUS + 10000.0), 0.0);
    velocity = vector3d(0.0, 0.0, 0.0);
    orientation = vector3d(0.0, 0.0, 90.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = true;
    autopilot_enabled = true;
    initial_altitude = position.abs() - MARS_RADIUS;
    break;

  case 2:
    // an elliptical polar orbit
    position = vector3d(0.0, 0.0, 1.2*MARS_RADIUS);
    velocity = vector3d(3500.0, 0.0, 0.0);
    orientation = vector3d(0.0, 0.0, 90.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 3:
    // polar surface launch at escape velocity (but drag prevents escape)
    position = vector3d(0.0, 0.0, MARS_RADIUS + LANDER_SIZE/2.0);
    velocity = vector3d(0.0, 0.0, 5027.0);
    orientation = vector3d(0.0, 0.0, 0.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 4:
    // an elliptical orbit that clips the atmosphere each time round, losing energy
    position = vector3d(0.0, 0.0, MARS_RADIUS + 100000.0);
    velocity = vector3d(4000.0, 0.0, 0.0);
    orientation = vector3d(0.0, 90.0, 0.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 5:
    // a descent from rest at the edge of the exosphere
    position = vector3d(0.0, -(MARS_RADIUS + EXOSPHERE), 0.0);
    velocity = vector3d(0.0, 0.0, 0.0);
    orientation = vector3d(0.0, 0.0, 90.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = true;
    autopilot_enabled = true;
    initial_altitude = position.abs() - MARS_RADIUS;
    break;

  }
}


    



