
// #include <gst/gst.h>
// #include "poGst.hpp"
#include "portalComm.hpp"
#include "portalRTC.hpp"

#include <thread>
#include <opencv2/opencv.hpp>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
#include <fcntl.h>

using namespace std;

/* Structure to contain all our information, so we can pass it to callbacks */


#ifdef _WIN32
enum ForeColour
{
    Default = 0x0008,
    Black = 0x0000,
    Blue = 0x0001,
    Green = 0x0002,
    Cyan = 0x0003,
    Red = 0x0004,
    Magenta = 0x0005,
    Yellow = 0x0006,
    White = 0x0007,
};
#else
enum ForeColour
{
    Default = 0,
    Black = 30,
    Red = 31,
    Green = 32,
    Yellow = 33,
    Blue = 34,
    Magenta = 35,
    Cyan = 36,
    White = 37,
};
#endif

void PrintConsole(const char* lpszText, short nColor)
{
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, nColor | 0x0008);
#else
    std::ostringstream oss;
    oss << "\033[1;" << nColor << "m";
    std::cout << oss.str();
#endif

    std::cout << lpszText << std::endl;

#ifdef _WIN32
    SetConsoleTextAttribute(hConsole, 0x0008);
#else
    std::cout << "\033[0m";
#endif
}
char keyboardInput;
char userInput;

int main(int argc, char* argv[])
{
    // std::signal(SIGINT, handleSignal);
    // char userInput;
    
    // portal::Zed zed;
    // zed.setResloution("HD1080");
    // zed.setBitMode(zedMode::EIGHT);
    
    // if (zed.startZed())
    //     return -1;
    // cout << "Open zed, good" << endl;
    

   
    // portal::Comm comm("https://api.portal301.com/portalComm_v0.1/");
    portal::Comm comm("https://192.168.0.35:3333/portalComm_v0.1/");
    // portal::Comm comm("https://192.168.0.29:3333/portalComm_v0.1/");

    portal::Profile profile = {
        "SN000-FAKE-3568",
        "J-test0",       // alias
        "camera",        // type
        "no authLevel",  // authLevel
        "no-master",     // status
        "no location",   // location
        "no createdAt",  // createdAt
        "no descriptions", // descriptions
        "no vender",     // vender
        {}               // apps (empty vector)
    };

    comm.setProfile(profile);
    //comm.fetchAPI((const char*)"https://api.portal301.com");
    // curl�� C���̺귯���� url�� const char* �������� �����ؾ���.
    bool resAPI = comm.createModule((const char*)"https://api.portal301.com/fetch/v0.1/module/register");
    // bool resAPI = comm.createModule((const char*)"https://192.168.0.35/fetch/v0.1/module/register");
    if (!resAPI) {
        std::cout << "API fetch failed." << std::endl;
        return -1;
    }
    comm.connectModule();

    comm.setOnTask();
    comm.registering();

    portal::RTC portalRTC(&comm);
    portalRTC.setOnSignaling();

    
    /*
    thread thread3 = thread(&portal::RTC::receiveThread, &portalRTC);
    thread3.detach();
    poGst gst(&zed, &portalRTC);
    gst.setElements();
    */

   cv::Mat redMatrix(720,1280,CV_8UC3, cv::Scalar(0,0,255));
   cv::Mat greenMatrix(720,1280,CV_8UC3, cv::Scalar(0,255,0));
   std::vector<unsigned char> byte_redMat;
   std::vector<unsigned char> byte_greenMat;
   cv::imencode(".png", redMatrix, byte_redMat);
   cv::imencode(".jpg", greenMatrix, byte_greenMat);


    int controlFlag = 0;

#ifdef __linux
    // Save the current terminal settings
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= ~(ICANON | ECHO); // Turn off canonical mode and echoing

    // Apply the new terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
#endif

    PrintConsole("[Notice] 'q' Press >> exit program", ForeColour::Yellow);
    PrintConsole("Waiting for Connection Request ... ", ForeColour::Blue);

    while (true)
    {

#ifdef _WIN32
        if (_kbhit()) {
            userInput = _getch();
            if (userInput == 'q') {
                cout << "exit..." << endl;
                break;
            }
        }
#else
        // Set STDIN_FILENO to non-blocking mode
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

        if (read(STDIN_FILENO, &keyboardInput, 1) == 1)
        { // Read a single character
            if (keyboardInput == 'q')
            {
                std::cout << "Exiting the program...\n";
                break; // Exit the loop and terminate the program
            }

            // Do something with the input here (if needed)
            std::cout << "You pressed: " << keyboardInput << "\n";
        }
#endif
        
        // cout << "loop" << endl;
        if (portalRTC.areAnyChannelsOpen())
        {
            
            if (controlFlag == 0) {
                /*
                thread thread3 = thread(&portal::RTC::receiveThread, &portalRTC);
                thread3.detach();
                controlFlag = 1;
                */
            }

            
            // pass by reference
            // auto [rgb, depth, quaternion] = zed.extractFrame();

            auto rgb = byte_redMat;
            auto depth = byte_greenMat;

            portalRTC.sendDataToChannel("RGB", &rgb);
            portalRTC.sendDataToChannel("DEPTH", &depth);
            // portalRTC.sendDataToChannel("SENSOR", quaternion);
            usleep(20 * 1000);
        }
        else
        {
            // cout << "channel not opened" << endl;
        }
    }
    // zed.close();

    return 0;
}