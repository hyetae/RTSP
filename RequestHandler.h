#ifndef RTSP_REQUESTHANDLER_H
#define RTSP_REQUESTHANDLER_H
#include <string>
#include "UDPHandler.h"
#include "ClientSession.h"
#include "MediaStreamHandler.h"

class RequestHandler {
public:
    RequestHandler(MediaStreamHandler& mediaStreamHandler, UDPHandler& udpHandler);

    // RTSP 요청 처리
    void handleRequest(int clientSocket, ClientSession* session);

private:
    MediaStreamHandler& mediaStreamHandler;
    UDPHandler& udpHandler;
    bool isAlive;

    string parseMethod(const string& request);

    int parseCSeq(const string& request);

    pair<int, int> parsePorts(const string& request);

    bool parseAccept(const string& request);

    void handleOptionsRequest(int clientSocket, int cseq);

    void handleDescribeRequest(int clientSocket, int cseq, ClientSession* session, const string& request);

    void handleSetupRequest(int clientSocket, int cseq, ClientSession* session, const string& request);

    void handlePlayRequest(int clientSocket, int cseq, ClientSession* session);

    void handleTeardownRequest(int clientSocket, int cseq, ClientSession* session);
};

#endif //RTSP_REQUESTHANDLER_H