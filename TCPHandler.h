#ifndef RTSP_TCPHANDLER_H
#define RTSP_TCPHANDLER_H

#include <string>
#include <arpa/inet.h>

using namespace std;

class TCPHandler {
public:
    static TCPHandler& getInstance() {
        static TCPHandler instance;
        return instance;
    }

    void createTCPSocket();

    int acceptClientConnection();

    string receiveRTSPRequest(int clientSocket);

    void sendRTSPResponse(int clientSocket, string& response);

private:
    TCPHandler();
    ~TCPHandler();

    int tcpPort;
    int tcpSocket;
    sockaddr_in tcpAddr;
};

#endif //RTSP_TCPHANDLER_H
