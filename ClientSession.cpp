#include "ClientSession.h"
#include <mutex>
#include <string>

using namespace std;

ClientSession::ClientSession(int sessionId)
        : sessionId(sessionId), version(sessionId), state("INIT") {}

int ClientSession::getSessionId() const { return sessionId; }

int ClientSession::getVersion() const { return version; }

string ClientSession::getState() const { return state; }

void ClientSession::setState(const string& newState) {
    lock_guard<mutex> lock(mtx);
    state = newState;
    version++;
}