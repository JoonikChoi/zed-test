#ifndef PORTAL_GST
#define PORTAL_GST

#include <opencv2/opencv.hpp>
#include <gst/gst.h>

#include "portalZed.hpp"
#include "portalRTC.hpp"

typedef struct _CustomData
{
    cv::Mat* rgb;

    // GstElement
    GstElement* pipeline, * app_source;
    GstElement* video_convert, * video_sink;
    GstElement* queue;
    GstElement* x264enc;
    GstElement* rtph264pay;
    GstElement* rtpvrawpay;
    GstElement* udpsink;

    // ZED CAMERA
    portal::Zed* zed;

    // RTC
    portal::RTC* portalRTC;
    int cntl = 0;
    // Glib Variable
    guint sourceid;               /* To control the GSource */
    GMainLoop* main_loop;         /* GLib's Main Loop */
} CustomData;


class poGst
{
private:

public:

    CustomData data;
    GstBus* bus;

    poGst(portal::Zed* zed, portal::RTC* portalRTC);


    static gboolean
        push_data(CustomData* data)
    {
        GstBuffer* buffer;
        GstFlowReturn ret;
        GstMapInfo map;

        /* Create OpenCV Matrix */
        int width = 1280;
        int height = 720;

        
        auto [rgb, depth, quaternion] = data->zed->extractFrame2(true);

        if (quaternion == "NULL")
        {
            cout << "zed camera cannot grap. plz check the zed camera" << endl;
            return FALSE;
        }

        if (data->portalRTC->areAnyChannelsOpen())
        {
            data->portalRTC->sendDataToChannel("DEPTH", &depth);
            data->portalRTC->sendDataToChannel("SENSOR", quaternion);

            /*
            if (data->portalRTC->getChannelBufferedAmount() != 0) {
                cout << "[Buffered Amount : " << data->portalRTC->getChannelBufferedAmount() << "] Do not Send Data" << endl;
            }
            else {

                if (data->portalRTC->areAnyChannelsOpen())
                {
                    //data->portalRTC->sendDataToChannel("DEPTH", &depth);
                    //data->portalRTC->sendDataToChannel("SENSOR", quaternion);
                }
            }
            */
            
        }
        

        /*
        if (data->rgb) { // if pointer is null, it return false.
            // pointer is exist.
            cv::imshow("Red Matrix", redMatrix);
        }
        else {
            // pointer is null.
            cv::imshow("Red Matrix", greenMatrix);
        }
        */
        //Sleep(20);

        // Create a red-colored matrix
        cv::Mat redMatrix(height, width, CV_8UC3, cv::Scalar(0, 0, 255)); // Red color in BGR format
        cv::Mat greenMatrix(height, width, CV_8UC3, cv::Scalar(0, 255, 0)); // Red color in BGR format

        // Display the red matrix
        //cout << "111111111" << endl;
        //cv::imshow("Gst Matrix", rgb);
        //cout << "22222222" << endl;

        cv::waitKey(1);
        Sleep(50);
        /* Create a new empty buffer */
        buffer = gst_buffer_new_and_alloc(greenMatrix.total() * greenMatrix.elemSize());

        cv::Mat testMatrix;
        /* Convert OpenCV Mat to GStreamer buffer */
        gst_buffer_map(buffer, &map, GST_MAP_WRITE);
        if (data->cntl == 0) {
            testMatrix = greenMatrix;
            //memcpy(map.data, greenMatrix.data, greenMatrix.total() * greenMatrix.elemSize());
            data->cntl = 1;
        }
        else {
            testMatrix = redMatrix;
            data->cntl = 0;
        }
        memcpy(map.data, rgb.data, rgb.total() * rgb.elemSize());
        cv::imshow("Gst Matrix", rgb);
        //memcpy(map.data, greenMatrix.data, greenMatrix.total() * greenMatrix.elemSize());


        gst_buffer_unmap(buffer, &map);

        /* Push the buffer into the appsrc */
        g_signal_emit_by_name(data->app_source, "push-buffer", buffer, &ret);

        /* Free the buffer now that we are done with it */
        gst_buffer_unref(buffer);

        if (ret != GST_FLOW_OK) {
            /* We got some error, stop sending data */
            return FALSE;
        }
        // cout << "push ..." << endl;
        return TRUE;
    }

    static void
        start_feed(GstElement* source, guint size, CustomData* data)
    {
        if (data->sourceid == 0) {
            g_print("Start feeding\n");
            data->sourceid = g_idle_add((GSourceFunc)push_data, data);
        }
    }

    static void
        stop_feed(GstElement* source, CustomData* data)
    {
        if (data->sourceid != 0) {
            g_print("Stop feeding\n");
            g_source_remove(data->sourceid);
            data->sourceid = 0;
        }
    }

    static void
        error_cb(GstBus* bus, GstMessage* msg, CustomData* data)
    {
        GError* err;
        gchar* debug_info;

        /* Print error details on the screen */
        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Error received from element %s: %s\n",
            GST_OBJECT_NAME(msg->src), err->message);
        g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
        g_clear_error(&err);
        g_free(debug_info);

        g_main_loop_quit(data->main_loop);
    }

    bool setElements();
    void startMainLoop();
};


#endif