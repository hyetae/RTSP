#include "TCPHandler.h"
#include <string>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

TCPHandler::TCPHandler(): tcpPort(554), tcpSocket(-1) {}
TCPHandler::~TCPHandler() { if (tcpSocket != -1) close(tcpSocket); }

void TCPHandler::createTCPSocket() {
    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket == -1) {
        cerr << "tcp 소켓 생성 실패" << endl;
        exit(1);
    }

    memset(&tcpAddr, 0, sizeof(tcpAddr));
    tcpAddr.sin_family = AF_INET;
    tcpAddr.sin_addr.s_addr = INADDR_ANY;
    tcpAddr.sin_port = htons(tcpPort);

    if (::bind(tcpSocket, (struct sockaddr*)&tcpAddr, sizeof(tcpAddr)) == -1) {
        cerr << "tcp 소켓 바인딩 실패" << endl;
        exit(1);
    }

    if (listen(tcpSocket, 10) == -1) {
        cerr << "tcp 소켓 리스닝 실패" << endl;
        exit(1);
    }
}

int TCPHandler::acceptClientConnection() {
    socklen_t clientAddrLen = sizeof(tcpAddr);
    int clientSocket = accept(tcpSocket, (sockaddr*)&tcpAddr, &clientAddrLen);
    if (clientSocket == -1) {
        cerr << "클라이언트 접속 실패" << endl;
        return -1;
    }

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &tcpAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

    return clientSocket;
}

string TCPHandler::receiveRTSPRequest(int clientSocket) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int receivedBytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (receivedBytes <= 0) {
        cerr << "rtsp 요청 받기 실패" << endl;
        return nullptr;
    }

    cout << "rtsp 요청:\n" << buffer << endl;
    return {buffer};
}

void TCPHandler::sendRTSPResponse(int clientSocket, string& response) {
    int sentBytes = send(clientSocket, response.c_str(), response.size(), 0);
    if (sentBytes == -1) {
        cerr << "rtsp 응답 실패" << endl;
        exit(1);
    }
}