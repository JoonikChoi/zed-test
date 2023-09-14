#ifndef PORTAL_ZED
#define PORTAL_ZED

#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <sl/Camera.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace sl;
using namespace std;
using json = nlohmann::json;

namespace zedMode
{
    enum Mode
    {
        EIGHT,
        NINE,
        TEN,
        TWELVE
    };
} // namespace zedMode

namespace portal
{
    class Zed
    {
    private:
        sl::Camera zed;

        InitParameters init_parameters;
        RuntimeParameters runtime_parameters;
        Resolution image_size;
        float maxDistance = 5.0;
        float minDistance = 0.2;
        zedMode::Mode bitMode = zedMode::EIGHT;
    public:
        Zed();

        int startZed();
        void close();
        void setResloution(string resolution);
        void setBitMode(zedMode::Mode bitMode);
        void setMaxDistance(float distance);
        float getMaxDistance();
        void setMinDistance(float distance);
        float getMinDistance();
        tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::string> extractFrame();
        tuple<cv::Mat, std::vector<unsigned char>, std::string> extractFrame2(bool status);
    };
} // namespace portal
#endif