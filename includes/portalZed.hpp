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

namespace portal
{
class Zed
{
  private:
    sl::Camera zed;
    InitParameters init_parameters;
    RuntimeParameters runtime_parameters;
    Resolution image_size;

  public:
    Zed();

    int startZed();
    void close();
    void setResloution(int width, int height);
    tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::string> extractFrame(bool status);
};
} // namespace portal

#endif