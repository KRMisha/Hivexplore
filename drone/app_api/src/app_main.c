/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2020 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * api_app.c - App layer application that calls app API functions to make
 *             sure they are compiled in CI.
 */

#include <math.h>
#include <string.h>

#include "app.h"

#include "FreeRTOS.h"
#include "task.h"

#include "debug.h"

#include "ledseq.h"
#include "crtp_commander_high_level.h"
#include "locodeck.h"
#include "mem.h"
#include "log.h"
#include "param.h"
#include "pm.h"
#include "app_channel.h"
#include "commander.h"
#include "sitaw.h"

#include "app_main.h"

#define DEBUG_MODULE "APPAPI"

// Min max helper macros
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)
#define ABS(a) ((a < 0) ? -a : a)

// Constants
static const uint16_t OBSTACLE_DETECTED_THRESHOLD = 300;
static const uint16_t EDGE_DETECTED_THRESHOLD = 400;
static const float EXPLORATION_HEIGHT = 0.2f; // 0.5f
static const float CRUISE_VELOCITY = 0.1f; // 0.2f
static const float MAXIMUM_VELOCITY = 0.5f; // 0.7f
static const uint16_t METER_TO_MILLIMETER_FACTOR = 1000;
static const uint16_t MAXIMUM_EXPLORE_TICKS = 200;
static const uint16_t MAXIMUM_READING_TICKS = 200;

// States
static mission_state_t missionState = MISSION_STANDBY;
static exploring_state_t exploringState = EXPLORING_IDLE;
static returning_state_t returningState = RETURNING_RETURN;
static emergency_state_t emergencyState = EMERGENCY_LAND;

// Data
static drone_status_t droneStatus = STATUS_STANDBY;
static bool isM1LedOn = false;
static setpoint_t setPoint;

// Readings
static uint16_t frontSensorReading;
static uint16_t leftSensorReading;
static uint16_t backSensorReading;
static uint16_t rightSensorReading;
static uint16_t upSensorReading;
static uint16_t downSensorReading;
static point_t positionReading;
static float yawReading;
static float rollReading;
static float pitchReading;
static uint8_t rssiReading;

// TODO: Remove
// static float kalmanReadingX;
// static float kalmanReadingY;
// static float kalmanReadingZ;
// static float kalmanReadingVX;
// static float kalmanReadingVY;
// static float kalmanReadingVZ;

static point_t initialPosition;

// Targets
static float targetForwardVelocity;
static float targetLeftVelocity;
static float targetHeight;
static float targetYawRate;
static float targetYawToBase;

static uint16_t exploreWatchdog = MAXIMUM_EXPLORE_TICKS;
static uint16_t obstacleClearedCounter = MAXIMUM_READING_TICKS;

void appMain(void) {
    vTaskDelay(M2T(3000));

    const logVarId_t frontSensorId = logGetVarId("range", "front");
    const logVarId_t leftSensorId = logGetVarId("range", "left");
    const logVarId_t backSensorId = logGetVarId("range", "back");
    const logVarId_t rightSensorId = logGetVarId("range", "right");
    const logVarId_t upSensorId = logGetVarId("range", "up");
    const logVarId_t downSensorId = logGetVarId("range", "zrange");
    const logVarId_t positionXId = logGetVarId("stateEstimate", "x");
    const logVarId_t positionYId = logGetVarId("stateEstimate", "y");
    const logVarId_t positionZId = logGetVarId("stateEstimate", "z");
    const logVarId_t yawId = logGetVarId("stateEstimate", "yaw");
    const logVarId_t rollId = logGetVarId("stateEstimate", "roll");
    const logVarId_t pitchId = logGetVarId("stateEstimate", "pitch");
    const logVarId_t rssiId = logGetVarId("radio", "rssi");

    // TODO: Remove
    // const logVarId_t kalmanX = logGetVarId("kalman", "stateX");
    // const logVarId_t kalmanY = logGetVarId("kalman", "stateY");
    // const logVarId_t kalmanZ = logGetVarId("kalman", "stateZ");
    // const logVarId_t kalmanVX = logGetVarId("kalman", "statePX");
    // const logVarId_t kalmanVY = logGetVarId("kalman", "statePY");
    // const logVarId_t kalmanVZ = logGetVarId("kalman", "statePZ");

    const paramVarId_t flowDeckModuleId = paramGetVarId("deck", "bcFlow2");
    const paramVarId_t multirangerModuleId = paramGetVarId("deck", "bcMultiranger");

    const bool isFlowDeckInitialized = paramGetUint(flowDeckModuleId);
    const bool isMultirangerInitialized = paramGetUint(multirangerModuleId);
    bool isOutOfService = !isFlowDeckInitialized || !isMultirangerInitialized;
    if (!isFlowDeckInitialized) {
        DEBUG_PRINT("FlowDeckV2 is not connected\n");
    }
    if (!isMultirangerInitialized) {
        DEBUG_PRINT("Multiranger is not connected\n");
    }

    while (true) {
        vTaskDelay(M2T(10));

        ledSet(LED_GREEN_R, isM1LedOn);

        if (isOutOfService) {
            ledSet(LED_RED_R, true);
            continue;
        }

        frontSensorReading = logGetUint(frontSensorId);
        leftSensorReading = logGetUint(leftSensorId);
        backSensorReading = logGetUint(backSensorId);
        rightSensorReading = logGetUint(rightSensorId);
        upSensorReading = logGetUint(upSensorId);
        downSensorReading = logGetUint(downSensorId);
        positionReading.x = logGetFloat(positionXId);
        positionReading.y = logGetFloat(positionYId);
        positionReading.z = logGetFloat(positionZId);
        yawReading = logGetFloat(yawId);

        rssiReading = logGetUint(rssiId); // TODO: should this be float?
        rollReading = logGetFloat(rollId);
        pitchReading = logGetFloat(pitchId);

        (void)rssiReading; // TODO: Remove (this silences the unused variable compiler warning which is treated as an error)

        // TODO: Remove
        // kalmanReadingX = logGetFloat(kalmanX);
        // kalmanReadingY = logGetFloat(kalmanY);
        // kalmanReadingZ = logGetFloat(kalmanZ);
        // kalmanReadingVX = logGetFloat(kalmanVX);
        // kalmanReadingVY = logGetFloat(kalmanVY);
        // kalmanReadingVZ = logGetFloat(kalmanVZ);

        // DEBUG_PRINT("kalman reading x: %f\n", (double)kalmanReadingX);
        // DEBUG_PRINT("kalman reading y: %f\n", (double)kalmanReadingY);
        // DEBUG_PRINT("kalman reading z: %f\n", (double)kalmanReadingZ);
        // DEBUG_PRINT("kalman reading vx: %f\n", (double)kalmanReadingVX);
        // DEBUG_PRINT("kalman reading vy: %f\n", (double)kalmanReadingVY);
        // DEBUG_PRINT("kalman reading vz: %f\n", (double)kalmanReadingVZ);

        targetForwardVelocity = 0.0;
        targetLeftVelocity = 0.0;
        targetHeight = 0.0;
        targetYawRate = 0.0;
        targetYawToBase = 0.0;

        switch (missionState) {
        case MISSION_STANDBY:
            droneStatus = STATUS_STANDBY;
            break;
        case MISSION_EXPLORING:
            avoidObstacle();
            explore();
            break;
        case MISSION_RETURNING:
            avoidObstacle();
            returnToBase();
            break;
        case MISSION_EMERGENCY:
            emergencyLand();
            break;
        case MISSION_LANDED:
            droneStatus = STATUS_LANDED;
            break;
        }

        if (isCrashed()) {
            isOutOfService = true;
            droneStatus = STATUS_CRASHED;
            memset(&setPoint, 0, sizeof(setpoint_t));
        }

        static const uint8_t TASK_PRIORITY = 3;
        commanderSetSetpoint(&setPoint, TASK_PRIORITY);
    }
}

void avoidObstacle(void) {
    bool isExploringAvoidanceDisallowed =
        missionState == MISSION_EXPLORING && (exploringState == EXPLORING_IDLE || exploringState == EXPLORING_LIFTOFF);
    bool isReturningAvoidanceDisallowed =
        missionState == MISSION_RETURNING && (returningState == RETURNING_LAND || returningState == RETURNING_LAND);

    bool isAvoidanceAllowed = !isExploringAvoidanceDisallowed && !isReturningAvoidanceDisallowed;

    if (isAvoidanceAllowed) {
        // Distance correction required to stay out of range of any obstacle
        uint16_t leftDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, leftSensorReading);
        uint16_t rightDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, rightSensorReading);
        uint16_t frontDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, frontSensorReading);
        uint16_t backDistanceCorrection = calculateDistanceCorrection(OBSTACLE_DETECTED_THRESHOLD, backSensorReading);

        // Velocity required to apply distance correction
        const float AVOIDANCE_SENSITIVITY = MAXIMUM_VELOCITY / OBSTACLE_DETECTED_THRESHOLD;
        targetLeftVelocity += (rightDistanceCorrection - leftDistanceCorrection) * AVOIDANCE_SENSITIVITY;
        targetForwardVelocity += (backDistanceCorrection - frontDistanceCorrection) * AVOIDANCE_SENSITIVITY;
    }
}

void explore(void) {
    switch (exploringState) {
    case EXPLORING_IDLE: {
        droneStatus = STATUS_STANDBY;

        DEBUG_PRINT("Idle\n");
        memset(&setPoint, 0, sizeof(setpoint_t));

        // Check if any obstacle is in the way before taking off
        if (upSensorReading > EXPLORATION_HEIGHT * METER_TO_MILLIMETER_FACTOR) {
            DEBUG_PRINT("Liftoff\n");
            initialPosition = positionReading;
            exploringState = EXPLORING_LIFTOFF;
        }
    } break;
    case EXPLORING_LIFTOFF: {
        droneStatus = STATUS_LIFTOFF;

        if (liftoff()) {
            exploringState = EXPLORING_EXPLORE;
        }
    } break;
    case EXPLORING_EXPLORE: {
        droneStatus = STATUS_FLYING;

        if (!forward()) {
            exploringState = EXPLORING_ROTATE;
        }
    } break;
    case EXPLORING_ROTATE: {
        droneStatus = STATUS_FLYING;

        if (rotate()) {
            exploringState = EXPLORING_EXPLORE;
        }
    } break;
    }
}

void returnToBase(void) {
    // TODO: Add RSSI ? (rssiLandingThreashold)
    // If returned to base, land
    static const double distanceToReturnEpsilon = 0.005;
    if (fabs((double)initialPosition.x - (double)positionReading.x) < distanceToReturnEpsilon &&
        fabs((double)initialPosition.y - (double)positionReading.y) < distanceToReturnEpsilon) {
        returningState = RETURNING_LAND;
    }

    switch (returningState) {
    case RETURNING_RETURN: {
        droneStatus = STATUS_FLYING;

        // Calculate rotation angle to turn towards base
        targetYawToBase = atan2(initialPosition.y - positionReading.y, initialPosition.x - positionReading.x) * 360.0 / (2.0 * M_PI);
        DEBUG_PRINT("Target Yaw: %f\n", (double)targetYawToBase);
        DEBUG_PRINT("Current yaw: %f\n", (double)yawReading);
        DEBUG_PRINT("Difference: (%f, %f)\n",
                    (double)(initialPosition.x - positionReading.x),
                    (double)(initialPosition.y - positionReading.y));

        // If the drone is towards its base
        static const double yawEpsilon = 10;
        double yawDifference = fabs(targetYawToBase - yawReading);
        DEBUG_PRINT("Yaw difference: %f\n", yawDifference);
        if (yawDifference < yawEpsilon || yawDifference > (360.0 - yawEpsilon)) {
            DEBUG_PRINT("I'm done turning!\n");

            // Go to explore algorithm when a wall is detected in front of the drone
            // TODO: Change name
            static const uint16_t EDGE_DETECTED_THRESHOLD_RETURN = 800;
            if (frontSensorReading < EDGE_DETECTED_THRESHOLD_RETURN) {
                // update way point might be useless
                // targetHeight += EXPLORATION_HEIGHT;
                // updateWaypoint();
                returningState = RETURNING_ROTATE;
            } else {
                // Go to base using absolute positions
                setPoint.mode.x = modeAbs;
                setPoint.mode.y = modeAbs;
                setPoint.position.x = initialPosition.x;
                setPoint.position.y = initialPosition.y;
            }
        } else {
            // Turn drone towards its base
            DEBUG_PRINT("I need to rotate\n");
            targetHeight += EXPLORATION_HEIGHT;
            updateWaypoint();
            setPoint.mode.yaw = modeAbs;
            setPoint.attitude.yaw = targetYawToBase;
        }
    } break;
    case RETURNING_ROTATE: {
        droneStatus = STATUS_FLYING;
        DEBUG_PRINT("Obstacle. Rotating\n");

        if (rotate()) {
            returningState = RETURNING_FORWARD;
        }
    } break;
    case RETURNING_FORWARD: {
        droneStatus = STATUS_FLYING;
        DEBUG_PRINT("Forward \n");

        // Obstacle has been passed go back to returning with absolute positions
        static const uint16_t OPEN_SPACE_THRESHOLD = 300;
        if ((rightSensorReading > EDGE_DETECTED_THRESHOLD + OPEN_SPACE_THRESHOLD && obstacleClearedCounter == 0) || exploreWatchdog == 0) {
            // Reset counters
            exploreWatchdog = MAXIMUM_EXPLORE_TICKS;
            obstacleClearedCounter = MAXIMUM_READING_TICKS;
            returningState = RETURNING_RETURN;
            break;
        }

        if (!forward()) {
            obstacleClearedCounter = MAXIMUM_READING_TICKS;
            returningState = RETURNING_ROTATE;
        }
        exploreWatchdog--;

        // Reset right sensor reading counter if obstacle on the right is detected
        if (rightSensorReading > EDGE_DETECTED_THRESHOLD) {
            obstacleClearedCounter--;
        } else {
            obstacleClearedCounter = MAXIMUM_READING_TICKS;
        }

    } break;
    case RETURNING_LAND: {
        droneStatus = STATUS_LANDING;

        if (land()) {
            returningState = RETURNING_IDLE;
        }
    } break;
    case RETURNING_IDLE: {
        droneStatus = STATUS_LANDED;

        memset(&setPoint, 0, sizeof(setpoint_t));
    } break;
    }
}

void emergencyLand(void) {
    switch (emergencyState) {
    case EMERGENCY_LAND:
        droneStatus = STATUS_LANDING;

        if (land()) {
            emergencyState = EMERGENCY_IDLE;
        }
        break;
    case EMERGENCY_IDLE:
        droneStatus = STATUS_LANDED;

        memset(&setPoint, 0, sizeof(setpoint_t));
        break;
    }
}

bool liftoff(void) {
    targetHeight += EXPLORATION_HEIGHT;
    updateWaypoint();
    if (downSensorReading >= EXPLORATION_HEIGHT * METER_TO_MILLIMETER_FACTOR) {
        DEBUG_PRINT("Liftoff finished\n");
        return true;
    }
    return false;
}

bool forward(void) {
    targetHeight += EXPLORATION_HEIGHT;
    targetForwardVelocity += CRUISE_VELOCITY;
    updateWaypoint();
    if (frontSensorReading < EDGE_DETECTED_THRESHOLD) {
        return false;
    }
    return true;
}

bool rotate(void) {
    static const uint16_t OPEN_SPACE_THRESHOLD = 300;
    targetHeight += EXPLORATION_HEIGHT;
    targetYawRate = 50;
    updateWaypoint();
    return frontSensorReading > EDGE_DETECTED_THRESHOLD + OPEN_SPACE_THRESHOLD;
}

bool land(void) {
    updateWaypoint();
    static const uint16_t LANDED_HEIGHT = 50;
    if (downSensorReading < LANDED_HEIGHT) {
        droneStatus = STATUS_LANDED;
        return true;
    }
    return false;
}

bool isCrashed(void) {
    bool isCrashed = false;

    if (sitAwTuDetected()) {
        isCrashed = true;
        DEBUG_PRINT("Drone crash detected. Reason: firmware tumble detected\n");
    }

    if (sitAwFFDetected()) {
        isCrashed = true;
        DEBUG_PRINT("Drone crash detected. Reason: firmware freefall detected\n");
    }

    static const uint8_t maxAngle = 55;
    if (fabs(rollReading) > maxAngle) {
        isCrashed = true;
        DEBUG_PRINT("Drone crash detected. Reason: high roll angle detected\n");
    }

    if (fabs(pitchReading) > maxAngle) {
        isCrashed = true;
        DEBUG_PRINT("Drone crash detected. Reason: high pitch angle detected\n");
    }

    return isCrashed;
}

void updateWaypoint(void) {
    setPoint.velocity_body = true;
    setPoint.mode.x = modeVelocity;
    setPoint.mode.y = modeVelocity;
    setPoint.velocity.x = targetForwardVelocity;
    setPoint.velocity.y = targetLeftVelocity;

    setPoint.mode.yaw = modeVelocity;
    setPoint.attitudeRate.yaw = targetYawRate;

    setPoint.mode.z = modeAbs;
    setPoint.position.z = targetHeight;
}

uint16_t calculateDistanceCorrection(uint16_t obstacleThreshold, uint16_t sensorReading) {
    return obstacleThreshold - MIN(sensorReading, obstacleThreshold);
}

LOG_GROUP_START(hivexplore)
LOG_ADD(LOG_UINT8, droneStatus, &droneStatus)
LOG_GROUP_STOP(hivexplore)

PARAM_GROUP_START(hivexplore)
PARAM_ADD(PARAM_UINT8, missionState, &missionState)
PARAM_ADD(PARAM_UINT8, isM1LedOn, &isM1LedOn)
PARAM_GROUP_STOP(hivexplore)
