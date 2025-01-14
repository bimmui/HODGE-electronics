/*
   Uno sketch to drive a stepper motor using the AccelStepper library using a step/direction constant current driver
   Made to simply find the approximate max speed of the motors being used, requires manually
    changing the value of max speed in the setup func
   Found that max speed is 2000, subject to chnage based on load tho
*/

#include <AccelStepper.h>

#define stepPin 5
#define dirPin 2

AccelStepper stepper(AccelStepper::DRIVER, stepPin, dirPin);

void setup()
{
    stepper.setMaxSpeed(2000);    // steps/sec
    stepper.setAcceleration(100); // steps/sec²
    stepper.setSpeed(1000);
}

void loop()
{
    // move stepper continuously
    stepper.runSpeed();
}