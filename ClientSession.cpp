#include "ClientSession.h"
#include <mutex>
#include <string>

ClientSession::ClientSession(int sessionId)
        : sessionId(sessionId), version(sessionId), state("INIT") {}

int ClientSession::getSessionId() const { return sessionId; }

int ClientSession::getVersion() const { return version; }

std::string ClientSession::getState() const { return state; }

std::pair<int, int> ClientSession::getPort() const { return {rtpPort, rtcpPort}; }

void ClientSession::setPort(int port1, int port2) {
    rtpPort = port1;
    rtcpPort = port2;

    SOCK.createUDPSocket(rtpPort, rtcpPort);
}

void ClientSession::setState(const std::string& newState) {
    std::lock_guard<std::mutex> lock(stateMutex);
    state = newState;
    version++;
}