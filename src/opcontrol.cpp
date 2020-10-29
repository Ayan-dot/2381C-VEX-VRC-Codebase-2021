/*
  ___  ____   ___  __  _____
 |__ \|___ \ / _ \/_ |/ ____|
    ) | __) | (_) || | |
   / / |__ < > _ < | | |
  / /_ ___) | (_) || | |____
 |____|____/ \___/ |_|\_____|

2381C <Team Captain: allentao7@gmail.com>

This file is part of 2381C's codebase for 2020-21 VEX Robotics VRC Change
Up Competition.

This file can not be copied, modified, or distributed without the express
permission of 2381C.

All relevant mathematical calculations for odometry and motion profiling are
documented and have been explained in extensive detail in our paper about
robot motion. The paper is located in the docs (documentation) folder.

opcontrol.cpp [contains]:
  - Full suite of driver controls used in driver skills, match play, and
    testing odometry
  - Driving, indexing, scoring, and pooping (discarding balls through the back)
*/
#include "main.h"
#include "globals.hpp"
#include "posTracking.cpp"
#include <cmath>

/**
 * Operator control — called during competition and driver skills for driver
 * control and for operating the robot.
 */
void opcontrol()
{
  // variables to hold right vertical tracking wheel encoder position, in
  // intervals of 10 ms
  long double lastposR = 0, currentposR = 0;

  // variables to hold left vertical tracking wheel encoder position, in
  // intervals of 10 ms
  long double lastposL = 0, currentposL = 0;

  // horizontal counterparts of above variables
  long double lastposH = 0, currentposH = 0;

  // angles taken by inertial sensor (IMU), in intervals of 10 ms
  long double newAngle = 0, lastAngle = 0;

  // the control loop
  while (true)
  {
    // joystick controls for the x-drive
    leftFront = master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_Y) + master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X) + master.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_X);
    leftBack = master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_Y) - master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X) + master.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_X);
    rightFront = -master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_Y) + master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X) + master.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_X);
    rightBack = -master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_Y) - master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X) + master.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_X);

    // reverse vertical encoders, find position, convert to inches
    currentposR = verticalEncoder2.get_value() * vertToInch;
    currentposL = verticalEncoder1.get_value() * vertToInch;

    // same as above, horizontal counterpart
    currentposH = horizontalEncoder.get_value() * horiToInch;

    // create and initialize a positionTracking instance for odometry
    positionTracking robotPos(lastAngle, currentposH, lastposH, currentposL, lastposL, currentposR, lastposR);

    // to avoid turning global coordinates into null values when calculations
    // are initializing
    if (!isnan(robotPos.returnX()) || !isnan(robotPos.returnY()))
    {
      // adds the horizontal vector passed by the position tracking class to
      // the global X coordinate
      globalX += robotPos.returnX();

      // same as above, vertical counterpart
      globalY += robotPos.returnY();
    }

    /*
    Perform IMU averaging, which in turn will modify the last angle. Rather
    than raw averaging, a weighted average will be computed.
    */

    // IMU averaging weights
    long double trackingWheelWeight = 0.01;
    long double imuWeight = 0.99;
    long double imuScaling = 0.9965;

    lastAngle = robotPos.returnOrient() * trackingWheelWeight + inertial.get_rotation() * (pi / 180.0) * imuWeight * imuScaling;
    lastposH = currentposH;
    lastposR = currentposR;
    lastposL = currentposL;

    // print x, y coordinates, the angle and IMU rotation to the brain
    pros::lcd::set_text(1, "X:" + std::to_string(globalX));
    pros::lcd::set_text(2, "Y:" + std::to_string(globalY));
    pros::lcd::set_text(6, "A:" + std::to_string(lastAngle));
    pros::lcd::set_text(3, "L:" + std::to_string(verticalEncoder1.get_value()));
    pros::lcd::set_text(4, "R:" + std::to_string(verticalEncoder2.get_value()));
    pros::lcd::set_text(5, "B:" + std::to_string(horizontalEncoder.get_value()));
    pros::lcd::set_text(7, "I: " + std::to_string(inertial.get_rotation() * imuScaling));

    /*
    The intake controls are defined as follows:
      - L1: Intake
      - L2: Outtake
      - Y: Only left intake is turned on
      - X: Only right intake is turned on
    */
    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_L1))
    {
      // start intakes
      leftIntake.move_velocity(200);
      rightIntake.move_velocity(-200);
    }

    else if (master.get_digital(pros::E_CONTROLLER_DIGITAL_L2))
    {
      leftIntake.move_velocity(-200);
      rightIntake.move_velocity(200);
    }

    else
    {
      leftIntake.move_velocity(-0);
      rightIntake.move_velocity(-0);
    }

    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_X))
    {
    }

    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_Y))
    {
      rightIntake.move_velocity(-200);
    }

    // indexing controls (R1 indexes the first ball to the shooting bay and the
    // remaning accordingly)
    shooter.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);

    // we know the ball is indexed if the line tracker reports <= INDEX_THRESHOLD
    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_R1))
    {
      if (line_tracker1.get_value() > INDEX_THRESHOLD)
      {
        // we do not have a ball properly indexed yet
        indexer.move_velocity(-200);
        shooter.move_velocity(10);
      }

      else
      {
        shooter.move_velocity(0);
        shooter.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);

        if (line_tracker2.get_value() > INDEX_THRESHOLD)
        {
          indexer.move_velocity(-200);
        }
      }
    }

    else
    {
      indexer.move_velocity(0);
      shooter.move_velocity(0);
    }

    // shooting controls (R2 fires balls)
    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_R2))
    {
      // shoots indexed ball
      shooter.move_velocity(130);
    }

    else
    {
      if (master.get_digital(pros::E_CONTROLLER_DIGITAL_R1) && line_tracker1.get_value() > INDEX_THRESHOLD)
      {
      }

      else
      {
        shooter.move_velocity(0);
      }
    }

    // discarding controls
    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_L1) && master.get_digital(pros::E_CONTROLLER_DIGITAL_L2))
    {
      shooter.move_velocity(-150);
    }

    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_A))
    {
      int iter = 0;
      inertial.reset();
      while (inertial.is_calibrating())
      {
        pros::lcd::set_text(7, "IMU calibrating ...");
        iter += 10;
        pros::delay(10);
      }
      lastAngle = robotPos.returnOrient();
    }

    // set loop frequency (t = 20ms)
    pros::delay(20);
  }
}
