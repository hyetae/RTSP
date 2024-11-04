#include "RequestHandler.h"
#include "TCPHandler.h"
#include "UDPHandler.h"
#include "utils.h"

#include <iostream>
#include <string>
#include <sstream>
#include <thread>

#define TCP TCPHandler::getInstance()

using namespace std;

RequestHandler::RequestHandler(MediaStreamHandler& mediaStreamHandler, UDPHandler& udpHandler)
        : mediaStreamHandler(mediaStreamHandler), udpHandler(udpHandler), isAlive(true) {}

void RequestHandler::handleRequest(int clientSocket, ClientSession* session) {
    cout << "클라이언트 스레드 생성" << endl;

    // 클라이언트 세션이 종료되지 않는 한 계속 반복
    while (isAlive) {
        // RTSP 요청 기다림
        string request = TCP.receiveRTSPRequest(clientSocket);
        if (request.empty()) {
            cerr << "Invalid RTSP request received." << endl;
            return;
        }

        // 메소드 파싱
        string method = parseMethod(request);

        // 요청 번호 파싱
        int cseq = parseCSeq(request);
        if (cseq == -1) {
            cerr << "CSeq parsing failed." << endl;
            return;
        }

        if (method == "OPTIONS") {
            handleOptionsRequest(clientSocket, cseq);
        } else if (method == "DESCRIBE") {
            handleDescribeRequest(clientSocket, cseq, session, request);
        } else if (method == "SETUP") {
            handleSetupRequest(clientSocket, cseq, session, request);
        } else if (method == "PLAY") {
            handlePlayRequest(clientSocket, cseq, session);
        } else if (method == "TEARDOWN") {
            handleTeardownRequest(clientSocket, cseq, session);
        } else {
            cerr << "Unsupported RTSP method: " << method << endl;
        }
    }
}

string RequestHandler::parseMethod(const string& request) {
    istringstream requestStream(request);
    string method;
    requestStream >> method;
    return method;
}

int RequestHandler::parseCSeq(const string& request) {
    istringstream requestStream(request);
    string line;
    while (getline(requestStream, line)) {
        if (line.find("CSeq") != string::npos) {
            istringstream lineStream(line);
            string label;
            int cseq;
            lineStream >> label >> cseq;
            return cseq;
        }
    }
    return -1; // CSeq not found
}

pair<int, int> RequestHandler::parsePorts(const string& request) {
    istringstream requestStream(request);
    string line;
    while (getline(requestStream, line)) {
        if (line.find("client_port=") != string::npos) {
            istringstream lineStream(line);
            string label;

            while (getline(lineStream, label, '/')) {
                string portRange;
                getline(lineStream, portRange);
                size_t eqPos = portRange.find('=') + 1;
                size_t dashPos = portRange.find('-');

                if (dashPos != string::npos) {
                    int rtpPort = stoi(portRange.substr(eqPos, dashPos - eqPos));
                    int rtcpPort = stoi(portRange.substr(dashPos + 1));
                    return {rtpPort, rtcpPort};
                }
            }
        }
    }
    return {-1, -1};
}

bool RequestHandler::parseAccept(const string& request) {
    istringstream requestStream(request);
    string line;
    while (getline(requestStream, line))
        if (line.find("application/sdp") != string::npos)
            return true;
    return false;
}

void RequestHandler::handleOptionsRequest(int clientSocket, int cseq) {
    string response = "RTSP/1.0 200 OK\r\n"
                           "CSeq: " + to_string(cseq) + "\r\n"
                           "Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n"
                           "\r\n";
    TCP.sendRTSPResponse(clientSocket, response);
}

void RequestHandler::handleDescribeRequest(int clientSocket, int cseq, ClientSession* session, const string& request) {
    string ip = "";
    string sdp = "";
    string response = "";
    if (parseAccept(request)) {
        response = "RTSP/1.0 200 OK\r\n";

        ip = utils::getIP();
        sdp = "v=0\r\n"
              "o=- "
              + to_string(session->getSessionId())
              + " " + to_string(session->getVersion())
              + " IN IP4 " + ip + "\r\n"
              "s=Audio Stream\r\n"
              "c=IN IP4 " + ip + "\r\n"
              "t=0 0\r\n"
              "m=audio " + to_string(udpHandler.getUDPServerPort().first)
              + " RTP/AVP 0\r\n"
                "a=rtpmap:0 PCMU/8000\r\n";
    } else
        response = "RTSP/1.0 406 Not Acceptable\r\n";

    response += "CSeq: " + to_string(cseq) + "\r\n"
                "Content-Base: rtsp://" + ip + ":8554/\r\n"
                "Content-Type: application/sdp\r\n"
                "Content-Length: " + to_string(sdp.size()) + "\r\n"
                "\r\n" + sdp;
    TCP.sendRTSPResponse(clientSocket, response);
}

void RequestHandler::handleSetupRequest(int clientSocket, int cseq, ClientSession* session, const string& request) {
    session->setState("SETUP");

    // 요청으로부터 클라이언트의 RTP, RTCP 포트 파싱
    auto ports = parsePorts(request);
    if (ports.first < 0 || ports.second < 0) {
        cerr << "클라이언트 포트가 없습니다" << endl;
        return;
    }

    udpHandler.setUDPPort(ports.first, ports.second);

    string response = "RTSP/1.0 200 OK\r\n"
                           "CSeq: " + to_string(cseq) + "\r\n"
                           "Session: " + to_string(session->getSessionId()) + "\r\n"
                           "Transport: RTP/AVP;unicast;client_port="
                           + to_string(udpHandler.getUDPClientPort().first) + "-"
                           + to_string(udpHandler.getUDPClientPort().second) + ";"
                           "server_port=" + to_string(udpHandler.getUDPServerPort().first)
                           + "-" + to_string(udpHandler.getUDPServerPort().second) + "\r\n"
                           "Session: " + to_string(session->getSessionId())
                           + "\r\n"
                             "\r\n";
    TCP.sendRTSPResponse(clientSocket, response);

    // RTP/RTCP 스레드 생성
    thread mediaStreamThread(&MediaStreamHandler::handleMediaStream, &mediaStreamHandler);
    mediaStreamThread.detach();
}

void RequestHandler::handlePlayRequest(int clientSocket, int cseq, ClientSession* session) {
    // 세션의 현재 상태 변경
    session->setState("PLAY");

    string response = "RTSP/1.0 200 OK\r\n"
                           "CSeq: " + to_string(cseq) + "\r\n"
                           "Session: " + to_string(session->getSessionId())
                           + "\r\n"
                             "\r\n";
    TCP.sendRTSPResponse(clientSocket, response);
    // RTP/RTCP 제어 상태 변경
    mediaStreamHandler.setCmd("PLAY");
}

void RequestHandler::handleTeardownRequest(int clientSocket, int cseq, ClientSession* session) {
    session->setState("TEARDOWN");

    string response = "RTSP/1.0 200 OK\r\n"
                           "CSeq: " + to_string(cseq) + "\r\n"
                           "Session: " + to_string(session->getSessionId())
                           + "\r\n"
                             "\r\n";

    TCP.sendRTSPResponse(clientSocket, response);
    // 반복문 종료
    isAlive = false;
    // RTP/RTCP 제어 상태 변경
    mediaStreamHandler.setCmd("TEARDOWN");
}
