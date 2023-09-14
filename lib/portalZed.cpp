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

cv::Mat slMat2cvMat(Mat& input)
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
cv::cuda::GpuMat slMat2cvMatGPU(Mat& input)
{
    std::cout << "GPU Convert" << std::endl;
    // Since cv::Mat data requires a uchar* pointer, we get the uchar1 pointer from sl::Mat (getPtr<T>())
    // cv::Mat and sl::Mat will share a single memory structure
    return cv::cuda::GpuMat(input.getHeight(), input.getWidth(), getOCVtype(input.getDataType()),
        input.getPtr<sl::uchar1>(MEM::GPU), input.getStepBytes(sl::MEM::GPU));
}
#endif
#ifdef HAVE_CUDA
cv::cuda::GpuMat slMat2cvMatGPU(Mat& input);
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
            cout << "[ZED] Error " << returned_state << ", exit program." << endl;
            return EXIT_FAILURE;
        }

        return false;
    }

    void Zed::close()
    {
        cout << "[ZED] Close the zed camera" << endl;
        zed.close();
    }

    void Zed::setResloution(int width, int height)
    {
        this->image_size.height = height;
        this->image_size.width = width;
    }

    void Zed::setBitMode(zedMode::Mode bitMode)
    {
        this->bitMode = bitMode;
    }

    void Zed::setMaxDistance(float distance)
    {
        this->maxDistance = distance;
    }

    float Zed::getMaxDistance() {
        return this->maxDistance;
    }

    void Zed::setMinDistance(float distance)
    {
        this->minDistance = distance;
    }

    float Zed::getMinDistance() {
        return this->minDistance;
    }

    tuple<std::vector<unsigned char>, std::vector<unsigned char>, std::string> Zed::extractFrame()
    {
        std::vector<unsigned char> RGBjpegBuf;
        std::vector<unsigned char> byteData_Merged_depth;

        cv::Mat testMat(image_size.height, image_size.width, CV_8UC3, cv::Scalar(0, 255, 0));

        Mat zedMat_RGB(image_size.width, image_size.height, MAT_TYPE::U8_C4);
        cv::Mat cvMat_RGB = slMat2cvMat(zedMat_RGB);
        cv::Mat rgb_matrix;

        Mat zedMat_depth(image_size.width, image_size.height, MAT_TYPE::F32_C1);
        cv::Mat cvMat_depth = slMat2cvMat(zedMat_depth);
        cv::Mat cvMat_processing_depth;
        cv::Mat cvMat_merged_depth;

        if (zed.grab(runtime_parameters) == ERROR_CODE::SUCCESS)
        {
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

            if (bitMode == zedMode::TWELVE)
            {
                cvMat_processing_depth = (cvMat_depth / this->maxDistance) * 4095.0; // 5m, 12bit
                cv::threshold(cvMat_processing_depth, cvMat_processing_depth, 4095.0, 0, cv::THRESH_TOZERO_INV);
                cvMat_processing_depth.convertTo(cvMat_processing_depth, CV_16UC1);

                cv::Mat top_mask = cv::Mat::ones(cvMat_processing_depth.size(), cvMat_processing_depth.type()) * 0x0F00;
                cv::Mat mid_mask = cv::Mat::ones(cvMat_processing_depth.size(), cvMat_processing_depth.type()) * 0x00F0;
                cv::Mat bot_mask = cv::Mat::ones(cvMat_processing_depth.size(), cvMat_processing_depth.type()) * 0x000F;
                // 1111 = 16 = F, 0011
                cv::Mat top = (cvMat_processing_depth & top_mask) / 16;
                cv::Mat mid = cvMat_processing_depth & mid_mask;
                cv::Mat bot = (cvMat_processing_depth & bot_mask) * 16;

                // cout << bot << endl;
                std::vector<cv::Mat> channels = { top, mid, bot };
                cv::merge(channels, cvMat_merged_depth);
                cvMat_merged_depth.convertTo(cvMat_merged_depth, CV_8UC3);
            }
            else if (bitMode == zedMode::EIGHT)
            {

                //cout << this->maxDistance << endl;
                cvMat_processing_depth = (cvMat_depth); // 4m, 8bit
                cv::threshold(cvMat_processing_depth, cvMat_processing_depth, (double)this->minDistance, 0.0, cv::THRESH_TOZERO);
                cvMat_processing_depth = (cvMat_processing_depth / this->maxDistance) * 255.0;
                //cv::threshold(cvMat_processing_depth, cvMat_processing_depth, (double)this->maxDistance, 0.0, cv::THRESH_TOZERO_INV);
                //cvMat_processing_depth = cvMat_processing_depth * 255.0;
                cv::threshold(cvMat_processing_depth, cvMat_processing_depth, 255.0, 0.0, cv::THRESH_TOZERO_INV);


                /*
                cvMat_processing_depth = (cvMat_depth / this->maxDistance) * 255.0;
                cv::threshold(cvMat_processing_depth, cvMat_processing_depth, 255.0, 0.0, cv::THRESH_TOZERO_INV);
                */

                cvMat_processing_depth.convertTo(cvMat_processing_depth, CV_8UC1);

                std::vector<cv::Mat> channels = { cvMat_processing_depth, cvMat_processing_depth, cvMat_processing_depth };
                cv::merge(channels, cvMat_merged_depth);
                cvMat_merged_depth.convertTo(cvMat_merged_depth, CV_8UC3);

                // cv::Mat newMat(cvMat_processing_depth.rows, cvMat_processing_depth.cols, CV_8UC3);
                // cv::Mat channels[3] = {cvMat_processing_depth, cvMat_processing_depth, cvMat_processing_depth};
                // cv::merge(channels, 3, newMat);
            }
            else
            {
                cout << "something wrong in bitMode variable" << endl;
                exit(-1);
            }

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
            if (true)
            {
                std::vector<int> comp;
                comp.push_back(cv::IMWRITE_JPEG_QUALITY);
                comp.push_back(10);

                cv::imencode(".png", cvMat_merged_depth, byteData_Merged_depth);
                cv::imencode(".jpg", cvMat_RGB, RGBjpegBuf, comp);

                return { RGBjpegBuf, byteData_Merged_depth, quaternion.dump() };
            }
        }
        else
        {
            std::cout << "[ZED] GRAP OUT" << std::endl;
            return { RGBjpegBuf, byteData_Merged_depth, "NULL" };
        }
        return { RGBjpegBuf, byteData_Merged_depth, "NULL" };
    }


    tuple<cv::Mat, std::vector<unsigned char>, std::string> Zed::extractFrame2(bool status)
    {
        std::vector<unsigned char> RGBjpegBuf;
        std::vector<unsigned char> byteData_Merged_depth;

        cv::Mat testMat(image_size.height, image_size.width, CV_8UC3, cv::Scalar(0, 255, 0));

        Mat zedMat_RGB(image_size.width, image_size.height, MAT_TYPE::U8_C4);
        cv::Mat cvMat_RGB = slMat2cvMat(zedMat_RGB);
        cv::Mat rgb_matrix;

        Mat zedMat_depth(image_size.width, image_size.height, MAT_TYPE::F32_C1);
        cv::Mat cvMat_depth = slMat2cvMat(zedMat_depth);
        cv::Mat cvMat_processing_depth;
        cv::Mat cvMat_merged_depth;

        if (zed.grab(runtime_parameters) == ERROR_CODE::SUCCESS)
        {
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

            if (bitMode == zedMode::TWELVE)
            {
                cvMat_processing_depth = (cvMat_depth / this->maxDistance) * 4095.0; // 5m, 12bit
                cv::threshold(cvMat_processing_depth, cvMat_processing_depth, 4095.0, 0, cv::THRESH_TOZERO_INV);
                cvMat_processing_depth.convertTo(cvMat_processing_depth, CV_16UC1);

                cv::Mat top_mask = cv::Mat::ones(cvMat_processing_depth.size(), cvMat_processing_depth.type()) * 0x0F00;
                cv::Mat mid_mask = cv::Mat::ones(cvMat_processing_depth.size(), cvMat_processing_depth.type()) * 0x00F0;
                cv::Mat bot_mask = cv::Mat::ones(cvMat_processing_depth.size(), cvMat_processing_depth.type()) * 0x000F;
                // 1111 = 16 = F, 0011
                cv::Mat top = (cvMat_processing_depth & top_mask) / 16;
                cv::Mat mid = cvMat_processing_depth & mid_mask;
                cv::Mat bot = (cvMat_processing_depth & bot_mask) * 16;

                // cout << bot << endl;
                std::vector<cv::Mat> channels = { top, mid, bot };
                cv::merge(channels, cvMat_merged_depth);
                cvMat_merged_depth.convertTo(cvMat_merged_depth, CV_8UC3);
            }
            else if (bitMode == zedMode::EIGHT)
            {
                
                //cout << this->maxDistance << endl;
                cvMat_processing_depth = (cvMat_depth); // 4m, 8bit
                cv::threshold(cvMat_processing_depth, cvMat_processing_depth, (double)this->minDistance, 0.0, cv::THRESH_TOZERO);
                cvMat_processing_depth = (cvMat_processing_depth / this->maxDistance) * 255.0;
                //cv::threshold(cvMat_processing_depth, cvMat_processing_depth, (double)this->maxDistance, 0.0, cv::THRESH_TOZERO_INV);
                //cvMat_processing_depth = cvMat_processing_depth * 255.0;
                cv::threshold(cvMat_processing_depth, cvMat_processing_depth, 255.0, 0.0, cv::THRESH_TOZERO_INV);
                

                /*
                cvMat_processing_depth = (cvMat_depth / this->maxDistance) * 255.0;
                cv::threshold(cvMat_processing_depth, cvMat_processing_depth, 255.0, 0.0, cv::THRESH_TOZERO_INV);
                */

                cvMat_processing_depth.convertTo(cvMat_processing_depth, CV_8UC1);

                std::vector<cv::Mat> channels = { cvMat_processing_depth, cvMat_processing_depth, cvMat_processing_depth };
                cv::merge(channels, cvMat_merged_depth);
                cvMat_merged_depth.convertTo(cvMat_merged_depth, CV_8UC3);

                // cv::Mat newMat(cvMat_processing_depth.rows, cvMat_processing_depth.cols, CV_8UC3);
                // cv::Mat channels[3] = {cvMat_processing_depth, cvMat_processing_depth, cvMat_processing_depth};
                // cv::merge(channels, 3, newMat);
            }
            else
            {
                cout << "something wrong in bitMode variable" << endl;
                exit(-1);
            }

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
                std::vector<int> comp;
                comp.push_back(cv::IMWRITE_JPEG_QUALITY);
                comp.push_back(10);

                std::vector<int> depth_comp;
                depth_comp.push_back(cv::IMWRITE_JPEG_QUALITY);
                depth_comp.push_back(95);


                cv::imencode(".png", cvMat_merged_depth, byteData_Merged_depth);
                cv::imencode(".jpg", cvMat_RGB, RGBjpegBuf, comp);

                return { cvMat_RGB, byteData_Merged_depth, quaternion.dump() };
            }
        }
        else
        {
            std::cout << "[ZED] GRAP OUT" << std::endl;
            return { cvMat_RGB, byteData_Merged_depth, "NULL" };
        }
        return { cvMat_RGB, byteData_Merged_depth, "NULL" };
    }
} // namespace portal