#include "poGst.hpp"

poGst::poGst(portal::Zed* zed, portal::RTC* portalRTC)
{
    /* Initialize cumstom data structure */
    memset(&data, 0, sizeof(data));

    /* Initialize GStreamer */
    gst_init(NULL, NULL);
    data.rgb = 0;
    data.zed = zed;
    data.portalRTC = portalRTC;
};

bool poGst::setElements()
{
    /* Create the elements */
    // appsrc -> videoconvert -> ximage or videosink
    data.app_source = gst_element_factory_make("appsrc", "video_source");
    data.video_convert = gst_element_factory_make("videoconvert", "video_convert");
    data.video_sink = gst_element_factory_make("autovideosink", "video_sink");

    //streaming element factory
    data.queue = gst_element_factory_make("queue", "queue");
    data.x264enc = gst_element_factory_make("x264enc", "x264enc");
    data.rtph264pay = gst_element_factory_make("rtph264pay", "rtph264pay");
    data.udpsink = gst_element_factory_make("udpsink", "udpsink");
    data.rtpvrawpay = gst_element_factory_make("rtpvrawpay", "rtpvrawpay");

    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new("test-pipeline");

    if (!data.app_source || !data.video_convert || !data.video_sink ||
        !data.queue || !data.x264enc || !data.rtph264pay || !data.udpsink || !data.rtpvrawpay)
    {
        g_printerr("Not all elements could be created.\n");
        return false;
    }

    /* Set appsrc & udpsink properties */
    // g_object_set(data.app_source, "format", GST_FORMAT_TIME, NULL);
    // Set appsrc properties
    g_object_set(data.app_source, "format", GST_FORMAT_TIME, "is-live", TRUE, "block", TRUE, NULL);
    //g_object_set(encoder, "framerate", "30/1", NULL);
    // g_object_set(data.x264enc, "bitrate", 1000, "key-int-max", 30, NULL);
    // g_object_set(encoder, "qp-min", 20, NULL); // Set a minimum QP value
    // g_object_set(data.x264enc, "key-int-max", 30, NULL); //zero-latency
    g_object_set(data.rtph264pay, "pt", 96, "mtu", 1200, NULL);
    g_object_set(data.udpsink, "host", "127.0.0.1", "port", 4444, NULL);
    g_object_set(data.x264enc, "tune", 0x00000004, NULL);

    // g_object_set(data.rtph264pay, "aggregate-mode", 1, NULL);

    // g_object_set(data.x264enc, "tune", 0x00000004, NULL);
    /*
    g_object_set(data.x264enc, "bitrate", 3000, NULL);
    g_object_set(data.x264enc, "key-int-max", 30, NULL);
    g_object_set(data.x264enc, "pass", 5, NULL);
    g_object_set(data.x264enc, "qp-min", 0, NULL);
    g_object_set(data.x264enc, "quantizer", 0, NULL);
    g_object_set(data.rtph264pay, "pt", 96, NULL);
    g_object_set(data.rtph264pay, "mtu", 2000, NULL);
    */

    /* Set appsrc caps(based on your input format) */
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "BGR",
        //"format", G_TYPE_STRING, "GRAY16_BE",
        "width", G_TYPE_INT, 1280,
        "height", G_TYPE_INT, 720,
        "framerate", GST_TYPE_FRACTION, 30, 1,
        NULL);

    //GstCaps* caps_rawpay = gst_caps_new_simple("video/x-rtp", NULL);

    g_object_set(data.app_source, "caps", caps, NULL);
    //g_object_set(data.rtpvrawpay, "caps", caps_rawpay, NULL);

    gst_caps_unref(caps);
    //gst_caps_unref(caps_rawpay);


    /* Configure appsrc */
    g_signal_connect(data.app_source, "need-data", G_CALLBACK(start_feed),
        &data);
    g_signal_connect(data.app_source, "enough-data", G_CALLBACK(stop_feed),
        &data);


    /* Link all elements that can be automatically linked because they have "Always" pads */
    gst_bin_add_many(GST_BIN(data.pipeline), data.app_source,
        data.video_convert, data.x264enc, data.rtph264pay, data.udpsink, NULL);
    if (gst_element_link_many(data.app_source, data.video_convert
        , data.x264enc, data.rtph264pay, data.udpsink, NULL) != TRUE) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(data.pipeline);
        return false;
    }

    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    bus = gst_element_get_bus(data.pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb,
        &data);
    gst_object_unref(bus);

    /* Start playing the pipeline */
    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

    /* Create a GLib Main Loop and set it to run */
    data.main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data.main_loop);

    /* Free resources */
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    // gst.data.rgb = &rgb;
    // 
    // portalRTC.sendDataToChannel("RGB", &rgb);

    return true;
}
void poGst::startMainLoop() {


}
