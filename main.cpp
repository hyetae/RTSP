#include "utils.h"
#include "ClientSession.h"
#include "TCPHandler.h"
#include "UDPHandler.h"
#include "RequestHandler.h"
#include "MediaStreamHandler.h"
#include <thread>
#include <iostream>
#define TCP TCPHandler::getInstance()
using namespace std;

int main() {
    // TCP 소켓 생성
    TCP.createTCPSocket();

    cout << "RTSP 서버 시작" << endl;

    while (true) {
        // 클라이언트 접속 대기
        int clientSocket = TCP.acceptClientConnection();
        if (clientSocket == -1) {
            cerr << "클라이언트 접속 오류" << endl;
            continue; // 오류 시 다음 클라이언트 접속 대기
        }
        cout << "클라이언트 접속 완료" << endl;

        auto udpHandler = UDPHandler();
        auto mediaStreamHandler = MediaStreamHandler(udpHandler);
        auto requestHandler = RequestHandler(mediaStreamHandler, udpHandler);
        auto clientSession = ClientSession((int)utils::getRanNum(16));

        // 클라이언트 세션을 새로운 스레드에서 처리
        thread clientThread(&RequestHandler::handleRequest, requestHandler, clientSocket, &clientSession);
        // 스레드 비동기 처리
        clientThread.detach();
    }

    return 0;
}
