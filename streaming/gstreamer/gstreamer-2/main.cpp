#include <iostream>
#include <gst/gst.h>
#include <string>

using namespace std;
int main(int arg, char *argv[]) {
    GstElement *pipeline = nullptr;
    GstBus *bus = nullptr;
    GstMessage *msg = nullptr;
    GstElement *source = nullptr;
    GstElement *sink = nullptr;
    GstStateChangeReturn ret;

    // gstreamer initialization
    gst_init(&arg, &argv);

    // building pipeline
    /*
    pipeline = gst_parse_launch(
            "playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
            nullptr);
    string playBin = "playbin uri=";
    string urlFile = argv[1];
    string cmd = playBin + urlFile;
    pipeline = gst_parse_launch(
            cmd.c_str(),
            nullptr);

     /*/       
    //*
    /* Create the elements */
    source = gst_element_factory_make("videotestsrc","source");
    sink = gst_element_factory_make("autovideosink","sink");
    /* create the empty pipeline */
    pipeline = gst_pipeline_new("test-pipeline");
    /*  Build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline),source,sink,NULL);   
    if (gst_element_link(source,sink)!=TRUE){
        gst_printerr("Elements could not be linked \n");
        gst_object_unref(pipeline);
        return -1;

    }
    /* Modify the source property */
    g_object_set(source,"pattern", 11,NULL);


    // start playing
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret==GST_STATE_CHANGE_FAILURE){
        gst_printerr("Unable to set the pipeline to playing state \n");
        gst_object_unref(pipeline);
        return -1;

    }


    //wait until error or EOS ( End Of Stream )
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                     static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // free memory
    if (msg != nullptr){
        GError *err;
        gchar *debug_info;
        switch (GST_MESSAGE_TYPE(msg))
        {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg,&err,&debug_info);
            g_printerr("Error received from elment %s:%s\n",GST_OBJECT_NAME(msg->src),err->message );
            g_printerr("Debugging Information : %s\n",debug_info?debug_info:"None");
            g_clear_error(&err);
            g_free(debug_info);
            break;
        case GST_MESSAGE_EOS:
            g_print("End-Of-Stream reached. \n");
            break;
        
        default:
            g_printerr("Unexpected message received .\n");
            break;
        }
        gst_message_unref(msg);

    }
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}