# zedApp - Zed Application for PORTAL301 System
### necessary library (you should install this)
socket.io-cpp-client, openssl, libdatachannel, zed-sdk(with cuda), opencv

### brief about this project files
- portalComm.h<br>
communication for webrtc signaling (using socket.io-client-cpp library)

- portalZed.h<br>
Stereo Camera of Stereo Lab (https://www.stereolabs.com/)
extract Frame using zed sdk and opencv.

- portalRTC.h<br>
use libdatachannel library.

# Main Process
1. conneting to signaling server
2. Setup Zed Camera
3. Waiting for webrtc connection request
4. When it's connected, extract frame from zed camera and send frame data using datachannel.
5. loop 4.

# Build with cmake
### build in linux with gcc/g++
1. You must install necessary library
2. git clone this
3. mkdir build && cd build
4. cmake ..
5. make
6. ./program

### build in Windows:64 with msvc (visual studio)
1. You should use vcpkg. open terminal and write command like this :
```console
vcpkg install opencv:x64-windows    
vcpkg install nlohmann-json:x64-windows     
vcpkg install libdatachannel[srtp]:x64-windows    
vcpkg install rapidjson:x64-windows     
vcpkg install socket-io-client:x64-windows
vcpkg integrate install
```
2. 반드시 vcpkg > lib 폴더로 가서 sioclient_tls.lib 을 sioclient.lib으로 이름변경해야 함. 기존 파일은 sioclient0.h로 변경(버그로 인해 mvsc가 sioclient.tls lib파일을 인식하지 못함. 이를 수동으로 해결)     
3. Build with msvc in Visual Studio (setting : x64, release)    
4. PORTAL301_ZED_Application 를 시작 프로젝트로 설정

### Gstreamer 
반드시 링커->추가 라이브러리 디렉터리에 $(GSTREAMER_1_0_ROOT_MSVC_X86_64)\lib 가 포함되어 있는 지,    
링커->추가 종속성에 poGst.lib과 같은 파일이 추가되었는 지 확인할 것.    
Visual Studio 전처리기 정의는 다음과 같음    
```console
%(PreprocessorDefinitions)
WIN32
_WINDOWS
NDEBUG
CMAKE_INTDIR="Release"
```
