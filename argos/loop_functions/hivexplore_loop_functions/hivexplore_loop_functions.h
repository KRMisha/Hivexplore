#ifndef HIVEXPLORE_LOOP_FUNCTIONS_H
#define HIVEXPLORE_LOOP_FUNCTIONS_H

#include <sys/un.h>
#include <argos3/core/simulator/loop_functions.h>
#include "controllers/crazyflie/crazyflie.h"
#include "utils/log_name.h"

using namespace argos;

class CHivexploreLoopFunctions : public CLoopFunctions {
public:
    virtual void Init(TConfigurationNode& t_tree) override;
    virtual void Reset() override;
    virtual void Destroy() override;
    virtual void PreStep() override;
    virtual void PostStep() override;
    virtual bool IsExperimentFinished() override;
    virtual void PostExperiment() override;

private:
    void StartSocket();
    bool Send(const LogName& logName, const json& droneId, const json& variables);
    void Stop();
    void SendDroneIdsToServer();
    std::vector<std::reference_wrapper<CCrazyflieController>> GetControllers();

    int m_connectionSocket = -1;
    sockaddr_un m_socketName;
    int m_dataSocket = -1;

    bool m_isExperimentFinished = false;
};

#endif
