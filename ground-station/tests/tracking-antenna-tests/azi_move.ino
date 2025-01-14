/*
   Uno sketch to drive a stepper motor using the AccelStepper library.
   Runs stepper back and forth between limits. (Like Bounce demo program.)
   Works with a ULN-2003 unipolar stepper driver, or a bipolar, constant voltage motor driver
   such as the L298 or TB6612, or a step/direction constant current driver like the a4988.
   Initial Creation: 10/15/21  --jkl  jlarson@pacifier.com
      - Rev 1 - 11/7/21      -jkl
      - Rev 2 = 12/14/21   -jkl
   Edited by Daniel Opara: 1/4/2025
*/

// #define _USE_MATH_DEFINES

// lets use an arm based board in the future pls, would like to use cmath
// #include <cmath>
#include <AccelStepper.h>

// Motor Connections (constant current, step/direction bipolar motor driver)
const int dirPin = 2;
const int stepPin = 5;

// Connected to MS2 on A4988 driver
const int spr = 800;
const double pi = 3.14159265358979323846;

long radiansToSteps(double radians, int stepsPerRev)
{
    return static_cast<int>((radians / (2.0 * pi)) * stepsPerRev);
}

AccelStepper myStepper(AccelStepper::DRIVER, stepPin, dirPin); // works for a4988 (Bipolar, constant current, step/direction driver)

void setup()
{
    // set the maximum speed, acceleration factor,
    // and the target position
    Serial.begin(9600);
    myStepper.setMaxSpeed(1000.0);
    myStepper.setAcceleration(50.0);
    // pointing it to my neighbor's house lol
    Serial.println(-radiansToSteps(2.77042, spr));
    myStepper.move(-radiansToSteps(2.77042, spr));
}

void loop()
{
    // Change direction once the motor reaches target position
    /*
      if (myStepper.distanceToGo() == 0)   // this form also works - pick your favorite!
      myStepper.moveTo(-myStepper.currentPosition());

      // Move the motor one step
      myStepper.run();
    */
    // run() returns true as long as the final position
    //    has not been reached and speed is not 0.
    // if (!myStepper.run()) {
    //   myStepper.moveTo(-myStepper.currentPosition());
    // }
    if (myStepper.distanceToGo() != 0)
    {
        myStepper.run();
    }
    else
    {
        // do something else
    }
}