#ifndef __APP_MAIN_H__
#define __APP_MAIN_H__

// Enums
typedef enum { MISSION_STANDBY, MISSION_EXPLORING, MISSION_RETURNING, MISSION_EMERGENCY } mission_state_t;
typedef enum { EXPLORING_IDLE, EXPLORING_LIFTOFF, EXPLORING_EXPLORE, EXPLORING_ROTATE } exploring_state_t;
typedef enum { RETURNING_RETURN, RETURNING_LAND, RETURNING_IDLE } returning_state_t;
typedef enum { EMERGENCY_LAND, EMERGENCY_IDLE } emergency_state_t;
typedef enum { STATUS_STANDBY, STATUS_FLYING, STATUS_LANDED, STATUS_CRASHED } drone_status_t;

static void avoidObstacle(void);
static void explore(void);
static void returnToBase(void);
static void emergencyLand(void);
static bool land(void);

static void updateWaypoint(void);
static void updateDroneStatus(void);
static uint16_t calculateDistanceCorrection(uint16_t obstacleThreshold, uint16_t sensorReading);
