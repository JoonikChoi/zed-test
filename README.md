# zedApp

portalComm.h
communication for webrtc signaling (using socket.io-client-cpp library)

portalZed.h
Stereo Camera of Stereo Lab (https://www.stereolabs.com/)
extract Frame using zed sdk and opencv.

portalRTC.h
use libdatachannel library.

# main.cpp process
1. conneted singaling server
2. Setup Zed Camera
3. Waiting for webrtc connection request
4. When it's connected, extract frame from zed camera and send frame data using datachannel.
