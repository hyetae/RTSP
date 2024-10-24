#ifndef RTSP_UDPHANDLER_H
#define RTSP_UDPHANDLER_H

#include "Protos.h"
#include <arpa/inet.h>
using namespace std;

class UDPHandler {
public:
    UDPHandler();
    ~UDPHandler();

    void createUDPSocket();

    void setUDPPort(int port1, int port2);

    pair<int, int> getUDPServerPort();

    pair<int, int> getUDPClientPort();

    void sendRTPPacket(unsigned char* rtpPacket, size_t packetSize);

    void sendSenderReport(Protos::SenderReport* senderReport, size_t srSize);

    void recvReceiverReport();

private:
    int rtpSocket;
    int rtcpSocket;
    int serverRTPPort;
    int clientRTPPort;
    int serverRTCPPort;
    int clientRTCPPort;

    sockaddr_in serverRTPAddr;
    sockaddr_in clientRTPAddr;
    sockaddr_in serverRTCPAddr;
    sockaddr_in clientRTCPAddr;
};


#endif //RTSP_UDPHANDLER_H
