./build build  
./build ./rtsp

ffmpeg -i rtsp://127.0.0.1:554 -f alsa default
