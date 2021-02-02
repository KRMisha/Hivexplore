/* Include the controller definition */
#include "crazyflieAlgorithm.h"
/* Function definitions for XML parsing */
#include <argos3/core/utility/configuration/argos_configuration.h>
/* 2D vector definition */
#include <argos3/core/utility/math/vector2.h>
/* Logging */
#include <argos3/core/utility/logging/argos_log.h>

/****************************************/
/****************************************/

CCrazyflieAlgorithm::CCrazyflieAlgorithm() :
   m_pcDistance(NULL),
   m_pcPropellers(NULL),
   m_pcRNG(NULL),
   m_pcRABA(NULL),
   m_pcRABS(NULL),
   m_pcPos(NULL),
   m_pcBattery(NULL),
   m_uiCurrentStep(0) {}

/****************************************/
/****************************************/

void CCrazyflieAlgorithm::Init(TConfigurationNode& t_node) {
   try {
      /*
       * Initialize sensors/actuators
       */
      m_pcDistance   = GetSensor<CCI_CrazyflieDistanceScannerSensor>("crazyflie_distance_scanner");
      m_pcPropellers = GetActuator<CCI_QuadRotorPositionActuator>("quadrotor_position");
      /* Get pointers to devices */
      m_pcRABA   = GetActuator<CCI_RangeAndBearingActuator>("range_and_bearing");
      m_pcRABS   = GetSensor<CCI_RangeAndBearingSensor>("range_and_bearing");
      try {
         m_pcPos = GetSensor<CCI_PositioningSensor>("positioning");
      }
      catch(CARGoSException& ex) {}
      try {
         m_pcBattery = GetSensor<CCI_BatterySensor>("battery");
      }
      catch(CARGoSException& ex) {}      
   }
   catch(CARGoSException& ex) {
      THROW_ARGOSEXCEPTION_NESTED("Error initializing the crazyflie sensing controller for robot \"" << GetId() << "\"", ex);
   }
   /*
    * Initialize other stuff
    */
   /* Create a random number generator. We use the 'argos' category so
      that creation, reset, seeding and cleanup are managed by ARGoS. */
   m_pcRNG = CRandom::CreateRNG("argos");

   m_uiCurrentStep = 0;
   Reset();
}

/****************************************/
/****************************************/

void CCrazyflieAlgorithm::ControlStep() {
   enum class State
   {
      SearchingForForwardWall,
      StopAfterWallFound,
      RotationAfterStop
   };

   static State state = State::SearchingForForwardWall;
   static float lastSensorValue = -2;

   // Look battery level
   const CCI_BatterySensor::SReading& sBatRead = m_pcBattery->GetReading();
   LOG << "Battery level: " << sBatRead.AvailableCharge  << std::endl;

   // Look here for documentation on the distance sensor: /root/argos3/src/plugins/robots/crazyflie/control_interface/ci_crazyflie_distance_scanner_sensor.h
   // Read distance sensor
   CCI_CrazyflieDistanceScannerSensor::TReadingsMap sDistRead = m_pcDistance->GetReadingsMap();
   auto iterDistRead = sDistRead.begin();
   if (sDistRead.size() == 4) {
      LOG << "Front dist: " << (iterDistRead++)->second  << std::endl;
      LOG << "Left dist: "  << (iterDistRead++)->second  << std::endl;
      LOG << "Back dist: "  << (iterDistRead++)->second  << std::endl;
      LOG << "Right dist: " << (iterDistRead)->second  << std::endl;
   }

   auto forwardSensorValue = (++sDistRead.begin())->second;

   switch (state)
   {
   case State::SearchingForForwardWall:
      {
         if (sDistRead.begin()->second > 0)
         {
            m_pcPropellers->SetRelativePosition(CVector3(0, 0.5, 0));
            m_pcPropellers->SetRelativeYaw(CRadians::PI_OVER_SIX / 4);
         }
         else if (forwardSensorValue > 0)
         {
            lastSensorValue = forwardSensorValue;
            m_pcPropellers->SetRelativePosition(CVector3(-0.2, 0, 0));
            state = State::StopAfterWallFound;
         }
         else
         {
            m_pcPropellers->SetRelativePosition(CVector3(0.5, 0, 0));
         }
      }
      break;
   case State::StopAfterWallFound:
      if (std::abs(forwardSensorValue - lastSensorValue) < 1 || forwardSensorValue < 0 || forwardSensorValue < lastSensorValue)
      {
         state = State::RotationAfterStop;
      }
      lastSensorValue = forwardSensorValue;
      break;
   case State::RotationAfterStop:
      if (forwardSensorValue > 0)
      {
         m_pcPropellers->SetRelativeYaw(CRadians::PI_OVER_SIX / 3);
      }
      else
      {
         state = State::SearchingForForwardWall;
      }
      break;
   }

   LOG << "Height: " << m_pcPos->GetReading().Position.GetZ() << std::endl;
   m_uiCurrentStep++;
}

/****************************************/
/****************************************/

bool CCrazyflieAlgorithm::fly() {
   CVector3 cPos = m_pcPos->GetReading().Position;
   if(2.f - cPos.GetZ() < 0.01f)
   {
      return false;
   }
   cPos.SetZ(2.0f);
   // cPos.SetX(2.0f);
   m_pcPropellers->SetAbsolutePosition(cPos);
   return true;
}

/****************************************/
/****************************************/

bool CCrazyflieAlgorithm::land() {
   CVector3 cPos = m_pcPos->GetReading().Position;
   if(Abs(cPos.GetZ()) < 0.01f)
   {
      return false;
   }
   cPos.SetZ(0.0f);
   cPos.SetX(0.0f);
   m_pcPropellers->SetAbsolutePosition(cPos);
   return true;
}

/****************************************/
/****************************************/

void CCrazyflieAlgorithm::flyTowardsCenter() {
   CVector3 center(0, 0, 0);
   auto vectorToCenter = center - m_pcPos->GetReading().Position;
   auto normalizedVector = vectorToCenter.Normalize();

   CVector3 direction;
   direction.SetX(normalizedVector.GetX());
   direction.SetY(normalizedVector.GetX());
   direction.SetZ(normalizedVector.GetX());
   LOG << "Unit Vector | X: " << direction.GetX() << " | Y: " << direction.GetY() << " | Z: " << direction.GetZ() << std::endl;
   m_pcPropellers->SetAbsolutePosition(direction);
}

/****************************************/
/****************************************/

void CCrazyflieAlgorithm::Reset() {
}

/****************************************/
/****************************************/

/*
 * This statement notifies ARGoS of the existence of the controller.
 * It binds the class passed as first argument to the string passed as
 * second argument.
 * The string is then usable in the XML configuration file to refer to
 * this controller.
 * When ARGoS reads that string in the XML file, it knows which controller
 * class to instantiate.
 * See also the XML configuration files for an example of how this is used.
 */
REGISTER_CONTROLLER(CCrazyflieAlgorithm, "crazyflie_sensing_controller")
