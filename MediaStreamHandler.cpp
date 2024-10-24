#include "Protos.h"
#include "utils.h"
#include "MediaStreamHandler.h"
#include <iostream>
#include <cstdint>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <utility>
#include <random>

using namespace std;

MediaStreamHandler::MediaStreamHandler(UDPHandler& udpHandler):
        udpHandler(udpHandler), isStreaming(false), isPaused(false) {}

void MediaStreamHandler::handleMediaStream() {
    Protos protos(utils::getRanNum(32));

    snd_pcm_t* pcmHandle;
    snd_pcm_hw_params_t* params;
    int rc, dir;

    size_t payloadSize = 160;  // 20ms당 160 샘플
    int frames = payloadSize;  // G.711은 8kHz에서 20ms당 160 샘플
    unsigned int sampleRate = 8000 ; // G.711의 샘플링 레이트

    auto buffer = new short[payloadSize];
    auto payload = new unsigned char[payloadSize];

    unsigned int octetCount = 0;
    unsigned int packetCount = 0;
    unsigned short seqNum = (unsigned short)utils::getRanNum(16);
    unsigned int timestamp = (unsigned int)utils::getRanNum(16);

    Protos::SenderReport sr;
    Protos::RTPHeader rtpHeader;

    initAlsa(pcmHandle, params, rc, sampleRate, dir);
    if (rc < 0) {
        fprintf(stderr, "ALSA initialization failed\n");
        return;
    }

    unique_lock<mutex> lock(mtx);

    while (isStreaming) {
        // 스트리밍을 시작하는 신호를 기다림
        condition.wait(lock, [this] { return !isPaused || !isStreaming; });

        if (!isStreaming) break;  // TEARDOWN 요청 시 종료

        if (!isPaused) {
            unsigned char rtpPacket[sizeof(Protos::RTPHeader) + payloadSize];

            if (captureAudio(pcmHandle, buffer, frames, rc, payload) < 0) {
                cerr << "오디오 캡처 실패..." << endl;
                break;
            }

            memset(rtpPacket, 0, sizeof(rtpPacket));
            protos.createRTPHeader(&rtpHeader, seqNum, timestamp);
            memcpy(rtpPacket, &rtpHeader, sizeof(rtpHeader));
            memcpy(rtpPacket + sizeof(rtpHeader), payload, payloadSize);

	        cout << "RTP " << packetCount << " sent" << endl;

            udpHandler.sendRTPPacket(rtpPacket, sizeof(rtpPacket));

            seqNum++;
            timestamp += payloadSize;
            packetCount++;
            octetCount += payloadSize;

            if (packetCount % 10 == 0) {
                cout << "RTCP sent" << endl;
                protos.createSR(&sr, timestamp, packetCount, octetCount);
                udpHandler.sendSenderReport(&sr, sizeof(sr));
                udpHandler.recvReceiverReport();
            }
	    }
        this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

unsigned char MediaStreamHandler::linearToUlaw(int sample) {
    const int MAX = 32767;
    const int BIAS = 0x84;
    int sign = (sample >> 8) & 0x80;
    if (sign != 0) sample = -sample;
    if (sample > MAX) sample = MAX;
    sample += BIAS;
    int exponent = 7;
    int mask;
    for (; exponent > 0; exponent--) {
        mask = 1 << (exponent + 3);
        if (sample >= mask) break;
    }
    int mantissa = (sample >> (exponent + 3)) & 0x0F;
    return ~(sign | (exponent << 4) | mantissa);
}

void MediaStreamHandler::initAlsa(snd_pcm_t*& pcmHandle, snd_pcm_hw_params_t*& params, int& rc, unsigned int& sampleRate, int& dir) {
    // PCM 장치 열기
    rc = snd_pcm_open(&pcmHandle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr, "Unable to open PCM device: %s\n", snd_strerror(rc));
        return;
    }

    // 하드웨어 매개변수 구조체 할당 및 기본값 설정
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcmHandle, params);

    // 인터리브 모드 설정
    rc = snd_pcm_hw_params_set_access(pcmHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0) {
        fprintf(stderr, "Error setting access mode: %s\n", snd_strerror(rc));
        return;
    }

    // 샘플 형식 설정 (16비트 리틀엔디안)
    rc = snd_pcm_hw_params_set_format(pcmHandle, params, SND_PCM_FORMAT_S16_LE);
    if (rc < 0) {
        fprintf(stderr, "Error setting format: %s\n", snd_strerror(rc));
        return;
    }

    // 모노 설정
    rc = snd_pcm_hw_params_set_channels(pcmHandle, params, 1);
    if (rc < 0) {
        fprintf(stderr, "Error setting channels: %s\n", snd_strerror(rc));
        return;
    }

    // 샘플 레이트 설정 (근사치)
    rc = snd_pcm_hw_params_set_rate_near(pcmHandle, params, &sampleRate, &dir);
    if (rc < 0) {
        fprintf(stderr, "Error setting sample rate: %s\n", snd_strerror(rc));
        return;
    }

    // 하드웨어 매개변수 적용
    rc = snd_pcm_hw_params(pcmHandle, params);
    if (rc < 0) {
        fprintf(stderr, "Unable to set HW parameters: %s\n", snd_strerror(rc));
        return;
    }
}

int MediaStreamHandler::captureAudio(snd_pcm_t*& pcmHandle, short*& buffer, int& frames, int& rc, unsigned char*& payload) {
    // ALSA로부터 오디오 데이터 캡처
    rc = snd_pcm_readi(pcmHandle, buffer, frames);
    if (rc == -EPIPE) {
        // 오버런
        cerr << "Overrun occurred" << endl;
        snd_pcm_prepare(pcmHandle);
        return -1;
    } else if (rc < 0) {
        cerr << "Error from read: " << snd_strerror(rc) << endl;
        return -1;
    } else if (rc != frames) {
        cerr << "Short read, read " << rc << " frames" << endl;
    }

    for (int i = 0; i < frames; i++)
        payload[i] = linearToUlaw(buffer[i]);

    return 0;
}

void MediaStreamHandler::setCmd(const string& cmd) {
    lock_guard<mutex> lock(mtx);
    if (cmd == "PLAY") {
        isStreaming = true;
        isPaused = false;
    } else if (cmd == "PAUSE")
        isPaused = true;
    else if (cmd == "TEARDOWN")
        isStreaming = false;
    condition.notify_all();
}