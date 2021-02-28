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
} // namespace

void CHivexploreLoopFunctions::Init(TConfigurationNode& t_tree) {
    Reset();
}

void CHivexploreLoopFunctions::Reset() {
    m_isExperimentFinished = false;
    StartSocket();

    // Send drone IDs to server

    std::vector<std::reference_wrapper<CCrazyflieController>> controllers = GetControllers();
    std::vector<std::string> droneIds;

    std::transform(controllers.begin(), controllers.end(), std::back_inserter(droneIds), [](const auto& controller) {
        return controller.get().GetId();
    });

    json packet = {
        {"logName", "drone-ids"},
        {"droneId", nullptr},
        {"value", droneIds},
    };
    std::string serializedPacket = packet.dump();

    ssize_t count = send(m_dataSocket, serializedPacket.c_str(), serializedPacket.size(), MSG_DONTWAIT);
    if (count == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        std::perror("Unix socket send");
    }
}

void CHivexploreLoopFunctions::Destroy() {
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
    std::vector<std::reference_wrapper<CCrazyflieController>> controllers = GetControllers();

    // Receive param data from server
    while (true) {
        static char buffer[4096] = {};
        std::fill(std::begin(buffer), std::end(buffer), '\0');
        ssize_t count = recv(m_dataSocket, buffer, sizeof(buffer), MSG_DONTWAIT);

        // Restart simulation if server disconnects
        if (count == 0) {
            LOGERR << "Unix socket connection closed\n";
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

        try {
            json packet = json::parse(buffer);

            std::string droneId = packet["droneId"];
            auto it = std::find_if(controllers.begin(), controllers.end(), [&droneId](const auto& controller) {
                return controller.get().GetId() == droneId;
            });
            if (it != controllers.end()) {
                it->get().SetParamData(packet["paramName"], packet["value"]);
            } else {
                LOGERR << "Unknown drone ID: " << droneId << '\n';
            }
        } catch (const json::exception& e) {
            LOGERR << "JSON error: " << e.what() << '\n';
            continue;
        }
    }

    // Send log data to server from each Crazyflie every second
    static constexpr std::uint8_t ticksPerSecond = 10;
    if (GetSpace().GetSimulationClock() % ticksPerSecond == 0) {
        for (const auto& controller : controllers) {
            auto logData = controller.get().GetLogData();
            for (const auto& [key, value] : logData) {
                std::visit(
                    [&key = std::as_const(key), &controller, this](const auto& value) {
                        json packet = {
                            {"logName", key},
                            {"droneId", controller.get().GetId()},
                            {"value", value},
                        };
                        std::string serializedPacket = packet.dump();

                        ssize_t count = send(m_dataSocket, serializedPacket.c_str(), serializedPacket.size(), MSG_DONTWAIT);
                        if (count == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                            // Restart simulation in case of socket error
                            std::perror("Unix socket send");
                            Stop();
                        }
                    },
                    value);
            }
        }
    }
}

void CHivexploreLoopFunctions::PostStep() {
}

bool CHivexploreLoopFunctions::IsExperimentFinished() {
    return m_isExperimentFinished;
}

void CHivexploreLoopFunctions::PostExperiment() {
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

std::vector<std::reference_wrapper<CCrazyflieController>> CHivexploreLoopFunctions::GetControllers() {
    CSpace::TMapPerType& entities = GetSpace().GetEntitiesByType("crazyflie");
    std::vector<std::reference_wrapper<CCrazyflieController>> controllers;
    std::transform(entities.begin(), entities.end(), std::back_inserter(controllers), [](const auto& pair) {
        CCrazyflieEntity& crazyflie = *any_cast<CCrazyflieEntity*>(pair.second);
        CCrazyflieController& controller = dynamic_cast<CCrazyflieController&>(crazyflie.GetControllableEntity().GetController());
        return std::ref(controller);
    });
    return controllers;
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
    LOG << "Stopping simulation...\n"
           "Please do the following to restart the mission:\n"
           "1. Press the restart button on the ARGoS client - the simulation will freeze\n"
           "2. Restart the server\n"
           "3. Press the play button on the ARGoS client\n";
    m_isExperimentFinished = true;
}

REGISTER_LOOP_FUNCTIONS(CHivexploreLoopFunctions, "hivexplore_loop_functions")
