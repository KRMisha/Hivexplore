#include "hivexplore_loop_functions.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <argos3/plugins/robots/crazyflie/simulator/crazyflie_entity.h>
#include "controllers/crazyflie/crazyflie.h"
#include "json.hpp"

using json = nlohmann::json;

namespace {
    const char socketPath[] = "/tmp/hivexplore/socket.sock";
}

// TODO: Change LOGERR to LOG for non-error output once the controller output is removed to spam less

void CHivexploreLoopFunctions::Init(TConfigurationNode& t_tree) {
    LOGERR << "Init!\n";
    StartSocket();
}

void CHivexploreLoopFunctions::Reset() {
    LOGERR << "Reset!\n";
    m_isExperimentFinished = false;
    StartSocket();
}

void CHivexploreLoopFunctions::Destroy() {
    LOGERR << "Destroy!\n";

    // Close socket
    if (close(m_connectionSocket) == -1 && errno != EBADF) {
        std::perror("Unix connection socket close");
    }
    if (close(m_dataSocket) == -1 && errno != EBADF) {
        std::perror("Unix connection socket close");
    }
    if (unlink(socketPath) == -1 && errno != ENOENT) {
        std::perror("Unix socket unlink");
    }
}

void CHivexploreLoopFunctions::PreStep() {
    // Get list of Crazyflie controllers
    CSpace::TMapPerType& entities = GetSpace().GetEntitiesByType("crazyflie");
    std::vector<std::reference_wrapper<CCrazyflieController>> controllers;
    std::transform(entities.begin(), entities.end(), std::back_inserter(controllers), [](const auto& pair) {
        CCrazyflieEntity& crazyflie = *any_cast<CCrazyflieEntity*>(pair.second);
        CCrazyflieController& controller = dynamic_cast<CCrazyflieController&>(crazyflie.GetControllableEntity().GetController());
        return std::ref(controller);
    });

    // Receive param data from server
    while (true) {
        static char buffer[4096] = {};
        ssize_t count = recv(m_dataSocket, buffer, sizeof(buffer), MSG_DONTWAIT);

        // Restart simulation if server disconnects
        if (count == 0) {
            std::cerr << "Unix socket connection closed\n";
            Stop();
            return;
        } else if (count == -1) {
            // Restart simulation in case of socket error
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::perror("Unix socket recv");
                Stop();
                return;
            }
            // Loop until there is no more data to receive
            break;
        }

        // TODO: Remove
        std::cerr << buffer;
    }

    // TODO: Send relevant messages
    char allo[] = "allo";
    if (++i % 1 == 0) {
        ssize_t count = send(m_dataSocket, allo, sizeof(allo), MSG_DONTWAIT);
        if (count == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            // Restart simulation in case of socket error
            std::perror("Unix socket send");
            Stop();
        }
    }
}

void CHivexploreLoopFunctions::PostStep() {
}

bool CHivexploreLoopFunctions::IsExperimentFinished() {
    return m_isExperimentFinished;
}

void CHivexploreLoopFunctions::PostExperiment() {
    std::cerr << "PostExperiment!\n";

    // Close socket
    if (close(m_connectionSocket) == -1) {
        std::perror("Unix connection socket close");
    }
    if (close(m_dataSocket) == -1) {
        std::perror("Unix connection socket close");
    }
    if (unlink(socketPath) == -1) {
        std::perror("Unix socket unlink");
    }
}

void CHivexploreLoopFunctions::StartSocket() {
    // Remove socket if it already exists
    if (unlink(socketPath) == -1 && errno != ENOENT) {
        std::perror("Unix socket unlink");
        std::exit(EXIT_FAILURE);
    }

    // Create Unix domain socket
    m_connectionSocket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (m_connectionSocket == -1) {
        std::perror("Unix socket creation");
        std::exit(EXIT_FAILURE);
    }

    // Bind socket to socket name
    std::memset(&m_socketName, 0, sizeof(m_socketName));
    m_socketName.sun_family = AF_UNIX;
    std::strncpy(m_socketName.sun_path, socketPath, sizeof(m_socketName.sun_path));

    int ret = bind(m_connectionSocket, reinterpret_cast<const struct sockaddr*>(&m_socketName), sizeof(m_socketName));
    if (ret == -1) {
        std::perror("Unix socket bind");
        std::exit(EXIT_FAILURE);
    }

    // Prepare to listen for connections
    ret = listen(m_connectionSocket, 1);
    if (ret == -1) {
        std::perror("Unix socket listen");
        std::exit(EXIT_FAILURE);
    }

    // Wait for incoming connection
    std::cout << "Waiting for Unix socket connection\n";
    m_dataSocket = accept(m_connectionSocket, nullptr, nullptr);
    if (m_dataSocket == -1) {
        std::perror("Unix socket accept");
        std::exit(EXIT_FAILURE);
    }

    std::cout << "Unix socket connection accepted\n";
}

void CHivexploreLoopFunctions::Stop() {
    LOGERR << "Stopping simulation... Please do the following to restart the mission:\n"
              "1. Press the restart button on the ARGoS client - the simulation will freeze\n"
              "2. Restart the server\n"
              "3. Press the play button on the ARGoS client\n";
    m_isExperimentFinished = true;
}

REGISTER_LOOP_FUNCTIONS(CHivexploreLoopFunctions, "hivexplore_loop_functions")
