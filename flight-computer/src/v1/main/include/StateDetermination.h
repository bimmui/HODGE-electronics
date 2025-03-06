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

// KF concept: how closely calculated future values are to prev values
// we set this to 0.5 to err on the side of safety
#define CA 0.5

// something for complementary filtering for the zero-velocity update feature
#define ACCEL_THRESHOLD 0.1

#define MAIN_DEPLOY_ALTITUDE 213.36 // meters, bode set it to 700 feet

enum class state
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

typedef struct
{
    float kf_altitude;
    float kf_vert_velo;
    float kf_accel;
} kf_vals;

class StateDeterminer
{
public:
    StateDeterminer();
    ~StateDeterminer();
    kf_vals determineState(float accel_data[3], float gyro_data[3], float altitude);
    // void switchGroundState(BBManager &manager, uint64_t packet);

private:
    AltitudeEstimator estimator;

    // used for ensuring the time is properly setup
    bool first_step;

    // prev values
    float prev_alt;
    float prev_accel;
    float prev_velo;

    bool main_attempted;
    float launch_start_time;
    state curr_state;

    void updatePrevEstimates(float altitude, float acceleration, float velocity);
};

#endif