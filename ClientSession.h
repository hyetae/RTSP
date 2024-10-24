#ifndef RTSP_CLIENTSESSION_H
#define RTSP_CLIENTSESSION_H

#include <map>
#include <queue>
#include <mutex>
#include <chrono>
#include <string>
#include <iostream>

using namespace std;

class ClientSession {
public:
    ClientSession(int sessionId);

    int getSessionId() const;

    int getVersion() const;

    string getState() const;

    void setState(const string& newState);

private:
    int sessionId, version;
    string state;
    mutex mtx;
};

#endif //RTSP_CLIENTSESSION_H