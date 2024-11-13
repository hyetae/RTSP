#ifndef RTSP_MEDIASTREAMHANDLER_H
#define RTSP_MEDIASTREAMHANDLER_H

#include "TCPHandler.h"
#include "UDPHandler.h"

#include <atomic>
#include <alsa/asoundlib.h>
#include <condition_variable>

typedef enum {
    eMediaStream_Play,
    eMediaStream_Pause,
    eMediaStream_Teardown,
} MediaStreamState;

class MediaStreamHandler {
public:
    MediaStreamHandler();

    void handleMediaStream();

    unsigned char linearToUlaw(int sample);
    void initAlsa(snd_pcm_t*& pcmHandle, snd_pcm_hw_params_t*& params, int& rc, unsigned int& sampleRate, int& dir);
    int captureAudio(snd_pcm_t*& pcmHandle, short*& buffer, int& frames, int& rc, unsigned char*& payload);

    void setCmd(const std::string& cmd);

private:
    MediaStreamState streamState;
    std::mutex streamMutex;
    std::condition_variable condition; // condition variable for streaming state controll
    
    // G.711 stream param
    snd_pcm_t* pcmHandle;
    snd_pcm_hw_params_t* params;
    size_t payloadSize = 0;
    int rc, dir;

};

#endif //RTSP_MEDIASTREAMHANDLER_H
