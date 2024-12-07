cmake_minimum_required(VERSION 3.29)
project(rtsp)

set(CMAKE_CXX_STANDARD 20)

# 소스 파일 추가
set(SOURCES
        main.cpp
        utils.cpp
        RequestHandler.cpp
        TCPHandler.cpp
        UDPHandler.cpp
        MediaStreamHandler.cpp
        AudioCapture.cpp
        OpusEncoder.cpp
        ClientSession.cpp
        Protos.cpp
)

# ALSA 라이브러리 찾기
find_package(ALSA REQUIRED)

# 실행 파일 생성
add_executable(rtsp ${SOURCES})

# ALSA 라이브러리 링크
target_link_libraries(rtsp ${ALSA_LIBRARIES})

# opus 라이브러리 찾기
find_library(OPUS_LIB opus)

# 찾은 opus 라이브러리와 링크
target_link_libraries(rtsp ${OPUS_LIB})
