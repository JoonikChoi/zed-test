#include "portalZed.hpp"

void crossSleep(int milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000); // usleep takes time in microseconds
#endif
}

int getOCVtype(sl::MAT_TYPE type)
{
    int cv_type = -1;
    switch (type)
    {
    case MAT_TYPE::F32_C1:
        cv_type = CV_32FC1;
        break;
    case MAT_TYPE::F32_C2:
        cv_type = CV_32FC2;
        break;
    case MAT_TYPE::F32_C3:
        cv_type = CV_32FC3;
        break;
    case MAT_TYPE::F32_C4:
        cv_type = CV_32FC4;
        break;
    case MAT_TYPE::U8_C1:
        cv_type = CV_8UC1;
        break;
    case MAT_TYPE::U8_C2:
        cv_type = CV_8UC2;
        break;
    case MAT_TYPE::U8_C3:
        cv_type = CV_8UC3;
        break;
    case MAT_TYPE::U8_C4:
        cv_type = CV_8UC4;
        break;
    default:
        break;
    }
    return cv_type;
}

cv::Mat slMat2cvMat(Mat &input)
{
    // Since cv::Mat data requires a uchar* pointer, we get the uchar1 pointer from sl::Mat (getPtr<T>())
    // cv::Mat and sl::Mat will share a single memory structure
    return cv::Mat(input.getHeight(), input.getWidth(), getOCVtype(input.getDataType()),
                   input.getPtr<sl::uchar1>(MEM::CPU), input.getStepBytes(sl::MEM::CPU));
}
#ifdef HAVE_CUDA
/**
 * Conversion function between sl::Mat and cv::Mat
 **/
cv::cuda::GpuMat slMat2cvMatGPU(Mat &input)
{
    std::cout << "GPU Convert" << std::endl;
    // Since cv::Mat data requires a uchar* pointer, we get the uchar1 pointer from sl::Mat (getPtr<T>())
    // cv::Mat and sl::Mat will share a single memory structure
    return cv::cuda::GpuMat(input.getHeight(), input.getWidth(), getOCVtype(input.getDataType()),
                            input.getPtr<sl::uchar1>(MEM::GPU), input.getStepBytes(sl::MEM::GPU));
}
#endif
#ifdef HAVE_CUDA
cv::cuda::GpuMat slMat2cvMatGPU(Mat &input);
#endif // HAVE_CUDA

namespace portal
{
Zed::Zed()
{
    // Set configuration parameters
    init_parameters.camera_resolution = RESOLUTION::HD720;
    // init_parameters.depth_mode = DEPTH_MODE::ULTRA; // Use ULTRA depth mode
    init_parameters.coordinate_units = UNIT::METER; // Use millimeter units (for depth measurements)
    init_parameters.depth_mode = DEPTH_MODE::NEURAL;
    init_parameters.depth_stabilization = 70;
    // runtime_parameters.sensing_mode = SENSING_MODE::STANDARD;
    runtime_parameters.confidence_threshold = 50;
};

int Zed::startZed()
{
    auto returned_state = zed.open(init_parameters);
    if (returned_state != ERROR_CODE::SUCCESS)
    {
        cout << "Error " << returned_state << ", exit program." << endl;
        return EXIT_FAILURE;
    }

    return false;
}

void Zed::close()
{
    cout << "Close the zed camera" << endl;
    zed.close();
}

void Zed::setResloution(int width, int height)
{
    this->image_size.height = height;
    this->image_size.width = width;
}

tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::string> Zed::extractFrame(bool status)
{
    std::vector<unsigned char> RGBjpegBuf;
    std::vector<unsigned char> byteData_Merged_depth;

    cv::Mat testMat(image_size.height, image_size.width, CV_8UC3, cv::Scalar(0, 255, 0));

    Mat zedMat_RGB(image_size.width, image_size.height, MAT_TYPE::U8_C4);
    cv::Mat cvMat_RGB = slMat2cvMat(zedMat_RGB);
    cv::Mat rgb_matrix;

    Mat zedMat_depth(image_size.width, image_size.height, MAT_TYPE::F32_C1);
    cv::Mat cvMat_depth = slMat2cvMat(zedMat_depth);
    cv::Mat cvMat_16bit_depth;
    cv::Mat cvMat_merged_depth;

    if (zed.grab(runtime_parameters) == ERROR_CODE::SUCCESS)
    {

        // std::cout << "grap in" << std::endl;
        SensorsData sensors_data;
        zed.getSensorsData(sensors_data, TIME_REFERENCE::CURRENT);

        json quaternion = {
            {"x", sensors_data.imu.pose.getOrientation()[0]},
            {"y", sensors_data.imu.pose.getOrientation()[1]},
            {"z", sensors_data.imu.pose.getOrientation()[2]},
            {"w", sensors_data.imu.pose.getOrientation()[3]},
        };

        // Retrieve left image
        zed.retrieveImage(zedMat_RGB, VIEW::LEFT);
        // zed.retrieveMeasure(point_cloud, MEASURE::XYZRGBA);
        zed.retrieveMeasure(zedMat_depth, MEASURE::DEPTH);

        cv::cvtColor(cvMat_RGB, cvMat_RGB, cv::COLOR_RGBA2RGB); // Convert from BGRA to BGR

        cvMat_16bit_depth = (cvMat_depth / 5.0) * 4095.0; // 10m, 16bit
        cv::threshold(cvMat_16bit_depth, cvMat_16bit_depth, 4095.0, 0, cv::THRESH_TOZERO_INV);
        cvMat_16bit_depth.convertTo(cvMat_16bit_depth, CV_16UC1);

        cv::Mat top_mask = cv::Mat::ones(cvMat_16bit_depth.size(), cvMat_16bit_depth.type()) * 0x0F00;
        cv::Mat mid_mask = cv::Mat::ones(cvMat_16bit_depth.size(), cvMat_16bit_depth.type()) * 0x00F0;
        cv::Mat bot_mask = cv::Mat::ones(cvMat_16bit_depth.size(), cvMat_16bit_depth.type()) * 0x000F;
        // 1111 = 16 = F, 0011
        cv::Mat top = (cvMat_16bit_depth & top_mask) / 16;
        cv::Mat mid = cvMat_16bit_depth & mid_mask;
        cv::Mat bot = (cvMat_16bit_depth & bot_mask) * 16;

        // cout << bot << endl;
        std::vector<cv::Mat> channels = {top, mid, bot};
        cv::merge(channels, cvMat_merged_depth);
        cvMat_merged_depth.convertTo(cvMat_merged_depth, CV_8UC3);

        /*
        //shapeand type check
        cout << cvMat_depth.cols << " " << cvMat_depth.rows << " " << cvMat_depth.channels() << endl;
        if (cvMat_16bit_depth.type() == 2)
            cout << "CV_16U" << endl;
        else
            cout << "CV Mat Type-number : " << cvMat_16bit_depth.type() << endl;
        */

        cv::imshow("depth", cvMat_merged_depth);
        cv::imshow("Image", cvMat_RGB);

        crossSleep(20);
        cv::waitKey(1);

        // Datachannel
        if (status)
        {
            cout << "channel good" << endl;
            cv::imencode(".png", cvMat_merged_depth, byteData_Merged_depth);

            // auto middle = byteData_Merged_depth.begin() + byteData_Merged_depth.size() / 2;
            // std::vector<unsigned char> first_half(byteData_Merged_depth.begin(), middle);
            // std::vector<unsigned char> second_half(middle, byteData_Merged_depth.end());

            // const std::size_t vectorSize = byteData_Merged_depth.size();
            // const std::size_t segmentSize = vectorSize / 3;
            // const auto firstSegmentBegin = byteData_Merged_depth.begin();
            // const auto firstSegmentEnd = firstSegmentBegin + segmentSize;
            // const auto secondSegmentBegin = firstSegmentEnd;
            // const auto secondSegmentEnd = secondSegmentBegin + segmentSize;
            // const auto thirdSegmentBegin = secondSegmentEnd;
            // const auto thirdSegmentEnd = byteData_Merged_depth.end();

            // std::vector<unsigned char> firstSegment(firstSegmentBegin, firstSegmentEnd);
            // std::vector<unsigned char> secondSegment(secondSegmentBegin, secondSegmentEnd);
            // std::vector<unsigned char> thirdSegment(thirdSegmentBegin, thirdSegmentEnd);

            cv::imencode(".jpg", cvMat_RGB, RGBjpegBuf);
            // auto middle2 = RGBjpegBuf.begin() + RGBjpegBuf.size() / 2;
            // std::vector<unsigned char> first_rgb(RGBjpegBuf.begin(), middle2);
            // std::vector<unsigned char> second_rgb(middle2, RGBjpegBuf.end());

            // interface->client->datachannel->send((std::string) "RGB segment");
            // interface->client->datachannel->send((std::byte *)(first_rgb.data()), first_rgb.size());
            // interface->client->datachannel->send((std::string) "RGB segment");
            // interface->client->datachannel->send((std::byte *)(second_rgb.data()), second_rgb.size());
            // interface->client->datachannel->send((std::string) "RGB-gathering-done");
            // cout << "done" << endl;

            // interface->client->datachannel->send((std::string) "Depth segment");
            // interface->client->datachannel->send((std::byte *)(firstSegment.data()), firstSegment.size());
            // interface->client->datachannel->send((std::string) "Depth segment");
            // interface->client->datachannel->send((std::byte *)(secondSegment.data()), secondSegment.size());
            // interface->client->datachannel->send((std::string) "Depth segment");
            // interface->client->datachannel->send((std::byte *)(thirdSegment.data()), thirdSegment.size());
            // interface->client->datachannel->send((std::string) "Depth-gathering-done");
            // interface->client->datachannel->send((std::string)"Sensor data");
            // interface->client->datachannel->send(quaternion.dump());
            // auto a = quaternion.dump();
            return {RGBjpegBuf, byteData_Merged_depth, quaternion.dump()};
        }
    }
    else
    {
        std::cout << "grap out" << std::endl;
        return {RGBjpegBuf, byteData_Merged_depth, "NULL"};
    }
    return {RGBjpegBuf, byteData_Merged_depth, "NULL"};
}
} // namespace portal
