#include "hivexplore_loop_functions.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace {
    const char socketPath[] = "/tmp/hivexplore/socket.sock";
}

void CHivexploreLoopFunctions::Init(TConfigurationNode& t_tree) {
    // Remove socket if it already exists
    if (unlink(socketPath) == -1 && errno != ENOENT) {
        perror("Unix socket unlink");
        std::exit(EXIT_FAILURE);
    }

    // Create Unix domain socket
    m_connectionSocket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (m_connectionSocket == -1) {
        perror("Unix socket creation");
        std::exit(EXIT_FAILURE);
    }

    // Bind socket to socket name
    std::memset(&m_socketName, 0, sizeof(m_socketName));
    m_socketName.sun_family = AF_UNIX;
    std::strncpy(m_socketName.sun_path, socketPath, sizeof(m_socketName.sun_path));

    int ret = bind(m_connectionSocket, reinterpret_cast<const struct sockaddr*>(&m_socketName), sizeof(m_socketName));
    if (ret == -1) {
        perror("Unix socket bind");
        std::exit(EXIT_FAILURE);
    }

    // Prepare to listen for connections
    ret = listen(m_connectionSocket, 1);
    if (ret == -1) {
        perror("Unix socket listen");
        std::exit(EXIT_FAILURE);
    }

    // Wait for incoming connection
    std::cout << "Waiting for Unix socket connection\n";
    m_dataSocket = accept(m_connectionSocket, nullptr, nullptr);
    if (m_dataSocket == -1) {
        perror("Unix socket accept");
        std::exit(EXIT_FAILURE);
    }

    std::cout << "Unix socket connection accepted\n";
}

void CHivexploreLoopFunctions::Reset() {
    LOGERR << "Reset!\n";
}

void CHivexploreLoopFunctions::Destroy() {
    LOGERR << "Destroy!\n";

    close(m_connectionSocket);
    close(m_dataSocket);
    unlink(socketPath);
}

void CHivexploreLoopFunctions::PreStep() {
    static int i = 0; // TODO: Remove

    // TODO: Empty out packets from the buffer in a while loop (while not -1) instead of just one per tick
    // TODO: Avoid recreating buffer every tick
    char buffer[4096] = {};
    ssize_t count = recv(m_dataSocket, buffer, sizeof(buffer), MSG_DONTWAIT);
    if (count == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("Unix socket recv");
    }

    // TODO: Send relevant messages
    char allo[] = "allo";
    if (++i % 1 == 0) {
        count = send(m_dataSocket, allo, sizeof(allo), 0);
        if (count == -1) {
            perror("Unix socket send");
        }
    }

    // TODO: Remove
    std::cerr << buffer;
}

void CHivexploreLoopFunctions::PostExperiment() {
    LOGERR << "PostExperiment!\n";
}

REGISTER_LOOP_FUNCTIONS(CHivexploreLoopFunctions, "hivexplore_loop_functions")
