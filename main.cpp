#include "portalComm.hpp"
#include "portalRTC.hpp"
#include "portalZed.hpp"
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif
#include <fcntl.h>

using namespace std;

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

int main(int argc, char* argv[])
{
    // std::signal(SIGINT, handleSignal);
    char userInput;

    portal::Zed zed;
    zed.setResloution(1280, 720);
    zed.setBitMode(zedMode::EIGHT);
    if (zed.startZed())
        return -1;
    cout << "Open zed, good" << endl;

   
    portal::Comm comm("https://api.portal301.com/portalComm_v0/");
    //portal::Comm comm("https://192.168.0.29:3333/portalComm_v0/");

    comm.setOnTask();
    comm.registering();

    comm.io.socket()->on("zed:command", [&zed](sio::event& ev) {
        cout << "[ event Name ] " << ev.get_name() << endl;
        if (ev.get_messages().at(0)->get_map().find("targetSetting") != ev.get_messages().at(0)->get_map().end())
        {
            string targetSetting = ev.get_messages().at(0)->get_map()["targetSetting"]->get_string();

            if (targetSetting == "distance-max") {
                double value = ev.get_messages().at(0)->get_map()["value"]->get_double();
                cout << "value : " << value << endl;

                cout << "set Distance ..." << endl;
                if (zed.getMinDistance() >= value) {
                    cout << "Invalid Value(minValue > maxValue)" << zed.getMinDistance() << zed.getMaxDistance() << endl;
                    return;
                }

                if (value > 20.0 || value < 1.0) {
                    cout << "Invalid Value in ZED Max Distance" << endl;
                    return;
                }
                cout << "set..." << endl;
                zed.setMaxDistance((float)value);
            }
            else if (targetSetting == "distance-min") {
                double value = ev.get_messages().at(0)->get_map()["value"]->get_double();
                cout << "value : " << value << endl;

                if (zed.getMaxDistance() <= value) {
                    cout << "Invalid Value(minValue > maxValue)" << zed.getMinDistance() << zed.getMaxDistance() << endl;
                    return;
                }
                if (value < 0.2) {
                    cout << "Invalid Value in ZED Min Distance" << endl;
                    return;
                }
                zed.setMinDistance((float)value);

            }
            else if (targetSetting == "bit") {
                double value = ev.get_messages().at(0)->get_map()["value"]->get_int();

                if ((int)value == 8) zed.setBitMode(zedMode::EIGHT);
                else if ((int)value == 12) zed.setBitMode(zedMode::TWELVE);
                else cout << "Invalid Value in zed Bit Mode" << endl;
            }
        }
    });

    portal::RTC portalRTC(&comm);
    portalRTC.setOnSignaling();

    //zed.datachannel = portalRTC.getChannel();

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
    PrintConsole("Waiting for Connection ... ", ForeColour::Blue);

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
        if (portalRTC.getChannelStatus())
        {
            // pass by reference
            auto [rgb, depth, quaternion] = zed.extractFrame(portalRTC.getChannelStatus());

            if (quaternion == "NULL")
            {
                cout << "zed camera cannot grap. plz check the zed camera" << endl;
                break;
            }

            if (portalRTC.getChannelBufferedAmount() != 0) {
                //cout << "[Buffered Amount : " << portalRTC.getChannelBufferedAmount() << "] Do not Send Data" << endl;
                continue;
            }

            portalRTC.sendDataToChannel("RGB", &rgb); // pass by reference
            portalRTC.sendDataToChannel("DEPTH", &depth);
            portalRTC.sendDataToChannel("SENSOR", quaternion);
        }
        else
        {
            // cout << "channel not opened" << endl;
        }
    }
    zed.close();

    return 0;
}