#include <Adafruit_Sensor.h>
#include <Adafruit_LSM9DS1.h>
#include "TrueNorthFunction.h"
#include <AccelStepper.h>
#include "StepperControl.h"
#include "ArrayFunctions.h"
#include <math.h>



float e = 2.71828;
double error;


bool samples_gathered = false;
bool trajectory_locked = false;
bool north_alligned = false;
bool stepper_moving = false;
int samples = 0;
int sample_size = 1000;
double sample_sum = 0;
double avg_yaw = 0;
// float distance_to_north = 0;


void setup()
{
    myStepper.setMaxSpeed(2000);
    myStepper.setAcceleration(1500);
    Serial.begin(115200);
    // myStepper.move(-radiansToSteps(pi*0.5, spr));
    while (!Serial)
        ; // wait for connection
    Serial.println();
    Serial.println("LSM9DS1 AHRS starting");

    if (!lsm.begin())
    {
        Serial.println("Failed to communicate with LSM9DS1.");
        while (1)
            ;
    }
    lsm.setupAccel(lsm.LSM9DS1_ACCELRANGE_2G);
    lsm.setupMag(lsm.LSM9DS1_MAGGAIN_4GAUSS);
    lsm.setupGyro(lsm.LSM9DS1_GYROSCALE_245DPS);

    Serial.println("DO NOT TOUCH APARATUS- Collecting Data in...");
    delay(1000);
    Serial.println("3..");
    delay(1000);
    Serial.println("2...");
    delay(1000);
    Serial.println("1...");
    delay(1000);

    
}

void loop()
{

    static char updated = 0;                // flags for sensor updates
    static int loop_counter = 0;            // sample & update loop counter
    static float Gxyz[3], Axyz[3], Mxyz[3]; // centered and scaled gyro/accel/mag data

    get_scaled_IMU(Gxyz, Axyz, Mxyz);

    Axyz[0] = -Axyz[0]; // fix accel/gyro handedness
    Gxyz[0] = -Gxyz[0]; // must be done after offsets & scales applied to raw data

    if (!samples_gathered){
  
      // if (millis() - lastSampleTime >= 600000){ //this 600,000 is supposed to be sample_Interval
      //   lastSampleTime = millis();
      //   yawArray[loop_counter] = yaw;
      //   loop_counter++;

      // }
      now = micros();
      deltat = (now - last) * 1.0e-6; // seconds since last update
      last = now;

      MahonyQuaternionUpdate(Axyz[0], Axyz[1], Axyz[2], Gxyz[0], Gxyz[1], Gxyz[2],
                           Mxyz[0], Mxyz[1], Mxyz[2], deltat);

      if (millis() - lastPrint > PRINT_SPEED){
      
        roll = atan2((q[0] * q[1] + q[2] * q[3]), 0.5 - (q[1] * q[1] + q[2] * q[2]));
        pitch = asin(2.0 * (q[0] * q[2] - q[1] * q[3]));
        yaw = atan2((q[1] * q[2] + q[0] * q[3]), 0.5 - (q[2] * q[2] + q[3] * q[3]));
        // to degrees
        yaw *= 180.0 / PI;
        pitch *= 180.0 / PI;
        roll *= 180.0 / PI;

        // http://www.ngdc.noaa.gov/geomag-web/#declination
        // conventional nav, yaw increases CW from North, corrected for local magnetic declination

      
        
        yaw = 180 + yaw;
        if (yaw < 0)
            yaw += 360.0;
        if (yaw >= 360.0)
            yaw -= 360.0;


        
      // Serial.print("ye ");
      // Serial.print(yaw, 5);
      // Serial.print(", ");
      // Serial.print(error, 5);
      // Serial.print(", ");
      // Serial.print(loop_counter, 1);
      // Serial.print(", ");
        Serial.println(yaw, 1);
        sample_sum = sample_sum + yaw;
        samples++;

      if (samples >= sample_size) {
            samples_gathered = true; // Mark phase as complete
            avg_yaw = sample_sum / samples;
            Serial.println("Average Yaw is:");
            Serial.println(avg_yaw);
            Serial.println("Yaw data collection complete. Moving to next phase.");
        }

    if (samples_gathered){
      current_pos = avg_yaw;
      Serial.println("The current position in Degrees is:");
      Serial.println(current_pos);
      delay(2000);
      Serial.println("The current position in Radians is:");
      current_pos = current_pos * (pi/180);
      Serial.println(current_pos);
      delay(2000);

      if (current_pos < pi){
        distance_to_north = current_pos;
        // myStepper.move(distance_to_north);
        trajectory_locked = true;
        Serial.println("The distance to true north is:");
        delay(1000);
        Serial.println(distance_to_north);
        delay(1000);
        Serial.println("Alligning with True North!");
        delay(3000);
        
        // myStepper.move(-radiansToSteps(current_pos, spr));
      }

      if (current_pos > pi){
        distance_to_north = current_pos - (2*pi);
        // myStepper.move(distance_to_north);
        trajectory_locked = true;
        Serial.println("The distance to true north is:");
        delay(1000);
        Serial.println(distance_to_north);
        delay(1000);
        Serial.println("Alligning with True North!");
        delay(3000);
        
        // myStepper.move(-radiansToSteps(current_pos, spr));
      }

      }

        

      }
    }

    if (trajectory_locked && !north_alligned && !stepper_moving){
      myStepper.move(distance_to_north);
      Serial.println("Test 1");
      stepper_moving = true;

    if (stepper_moving){
      Serial.println("Test 2");
      while (myStepper.distanceToGo() != 0){
        Serial.println("Test 3");
        myStepper.run();
        }
      // else{
      //   Serial.println("Test 4");
      //   north_alligned = true;
      //   stepper_moving = false;
      //   Serial.println("Alligned with True North!");
      //}

      }
    }   
    
    }


    