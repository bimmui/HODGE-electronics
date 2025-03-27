/**************************************************************
 *
 *                     StateDetermination.h
 *
 *     Author(s):  Daniel Opara
 *     Date:       3/20/2024
 *
 *
 *
 **************************************************************/

#ifndef STATE_DETERMINATION_H
#define STATE_DETERMINATION_H

#include <inttypes.h>
#include "altitude.h"

// standard noise deviation, calculated by Daniel TODO: OUTDATED, UPDATE THEM
#define SIGMA_GYRO 8
#define SIGMA_ACCEL 8
#define SIGMA_BARO 8

// TODO: see if this is still needed
// #define CA 0.5

// something for complementary filtering for the zero-velocity update feature
#define ACCEL_THRESHOLD 0.1

#define MAIN_DEPLOY_ALTITUDE 213.36 // meters, bode set it to 700 feet

enum class rocket_state
{
    POWER_ON = 0,
    LAUNCH_READY,
    POWERED_FLIGHT_PHASE,
    BURNOUT_PHASE,
    COAST_PHASE,
    APOGEE_PHASE,
    DROGUE_DEPLOYED,
    MAIN_DEPLOY_ATTEMPT,
    MAIN_DEPLOYED,
    RECOVERY
};

class StateDeterminer
{
public:
    StateDeterminer();
    ~StateDeterminer();
    kf_vals determineState(float accel_data[3], float gyro_data[3], float altitude, unsigned long curr_time);
    // void switchGroundState(BBManager &manager, uint64_t packet);

private:
    Estimator estimator_;

    // used for ensuring the time is properly setup
    bool first_step_;

    bool main_attempted_;
    rocket_state curr_state_;

    void updatePrevEstimates(float altitude, float acceleration, float velocity);
};

#endif