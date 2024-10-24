#include "UDPHandler.h"
#include <string>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
using namespace std;

UDPHandler::UDPHandler(): rtpSocket(-1), rtcpSocket(-1) {}

UDPHandler::~UDPHandler() {
    if (rtpSocket != -1) close(rtpSocket);
    if (rtcpSocket != -1) close(rtcpSocket);
}

void UDPHandler::createUDPSocket() {
    rtpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (rtpSocket == -1) {
        cerr << "rtp 소켓 생성 실패" << endl;
        exit(1);
    }

    rtcpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (rtcpSocket == -1) {
        cerr << "rtcp 소켓 생성 실패" << endl;
        exit(1);
    }

    // server rtp
    memset(&serverRTPAddr, 0, sizeof(serverRTPAddr));
    serverRTPAddr.sin_family = AF_INET;
    serverRTPAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverRTPAddr.sin_port = 0;

    socklen_t len = sizeof(serverRTPAddr);
    serverRTPPort = getsockname(rtpSocket, (struct sockaddr*)&serverRTPAddr, &len);
    if (serverRTPPort < 0) {
        perror("getsockname 실패");
        exit(1);
    }
    serverRTCPPort = serverRTPPort + 1;

    // client rtp
    memset(&clientRTPAddr, 0, sizeof(clientRTPAddr));
    clientRTPAddr.sin_family = AF_INET;
    clientRTPAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientRTPAddr.sin_port = htons(clientRTPPort);

    // server rtp
    memset(&serverRTCPAddr, 0, sizeof(serverRTCPAddr));
    serverRTCPAddr.sin_family = AF_INET;
    serverRTCPAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverRTCPAddr.sin_port = serverRTCPPort;

    // client rtcp
    memset(&clientRTCPAddr, 0, sizeof(clientRTCPAddr));
    clientRTCPAddr.sin_family = AF_INET;
    clientRTCPAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientRTCPAddr.sin_port = htons(clientRTCPPort);
}

void UDPHandler::setUDPPort(int port1, int port2) {
    clientRTPPort = port1;
    clientRTCPPort = port2;
}

pair<int, int> UDPHandler::getUDPServerPort() {
    return {serverRTPPort, serverRTCPPort};
}

pair<int, int> UDPHandler::getUDPClientPort() {
    return {clientRTPPort, clientRTCPPort};
}

void UDPHandler::sendRTPPacket(unsigned char* rtpPacket, size_t packetSize) {
    int sentBytes = sendto(rtpSocket, rtpPacket, packetSize, 0, (struct sockaddr*)&clientRTPAddr, sizeof(clientRTPAddr));
    if (sentBytes == -1) {
        cerr << "rtp 응답 실패" << endl;
        exit(1);
    }
}

void UDPHandler::sendSenderReport(Protos::SenderReport* senderReport, size_t srSize) {
    int sentBytes = sendto(rtcpSocket, senderReport, srSize, 0, (struct sockaddr*)&clientRTCPAddr, sizeof(clientRTCPAddr));
    if (sentBytes == -1) {
        cerr << "sr 전송 실패" << endl;
        exit(1);
    }
}

void UDPHandler::recvReceiverReport() {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    socklen_t len = sizeof(clientRTCPAddr);
    recvfrom(rtcpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientRTCPAddr, &len);

    cout << "Receiver Report:\n" << buffer << endl;
}