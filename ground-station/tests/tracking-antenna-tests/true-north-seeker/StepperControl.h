#include <AccelStepper.h>

const int dirPin = 2;
const int stepPin = 5;

const int spr = 1600;
const double pi = 3.14159265358979323846;

float current_pos = 0;
float distance_to_north = 0;

long radiansToSteps(double radians, int stepsPerRev)
{
    return static_cast<int>((radians / (2.0 * pi)) * stepsPerRev);
}

long degreesToRadians(double degrees)
{
    return degrees * (pi / 180);
}

AccelStepper myStepper(AccelStepper::DRIVER, stepPin, dirPin);