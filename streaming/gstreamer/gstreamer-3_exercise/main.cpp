#include <iostream>
#include <gst/gst.h>
#include <string>

using namespace std;

/* Structure to contain all our information, so we can pass it to the callbacks */
struct Custom_Data{
    GstElement *pipeline;
    GstElement *source;
    GstElement *convert;
    GstElement *resample;
    GstElement *sink;

};
/* Handler for the pad added signal */
static void pad_added_handler(GstElement *src,GstPad *pad,Custom_Data *data);


int main(int arg, char *argv[]) {
    Custom_Data data{};
    GstBus *bus = nullptr;
    GstMessage *msg = nullptr;
    GstStateChangeReturn ret;
    gboolean terminate = FALSE;


    // gstreamer initialization
    gst_init(&arg, &argv);

  
  
    /* Create the elements */
    data.source = gst_element_factory_make("uridecodebin","source");
    data.convert = gst_element_factory_make("videoconvert","convert");
    //data.resample = gst_element_factory_make("videoresample","resample");
    data.sink = gst_element_factory_make("autovideosink","sink");

    
  
    
    /* create the empty pipeline */
    data.pipeline = gst_pipeline_new("test-pipeline");
    
    if(!data.pipeline || !data.source || !data.convert ||  !data.sink ){
        g_printerr("Not all elements could be created ! \n");
        return -1;
    }
    
    /*  Build the pipeline */
    gst_bin_add_many(GST_BIN(data.pipeline), data.source,data.convert,  data.sink,NULL);   

    if (!gst_element_link_many(data.convert,data.sink,NULL)){
        gst_printerr("Elements could not be linked \n");
        gst_object_unref(data.pipeline);
        return -1;

    }
    /* Set the URI to play*/
    /* Below can be replaced with the command line argument to dynamically change the source*/
    //string uriSrc = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";
    
    string uriSrc = "http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4";

    g_object_set(data.source,"uri", uriSrc.c_str() ,NULL);
    
    /* connect to the pad-added signal */ 
    g_signal_connect(data.source,"pad-added",G_CALLBACK(pad_added_handler),&data);


    // start playing
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret==GST_STATE_CHANGE_FAILURE){
        gst_printerr("Unable to set the pipeline to playing state \n");
        gst_object_unref(data.pipeline);
        return -1;

    }


    //wait until error or EOS ( End Of Stream )
    bus = gst_element_get_bus(data.pipeline);
    do{
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
            case GST_MESSAGE_STATE_CHANGED:
                /* we are only interested in state-changed messages from the pipeline */
                if(GST_MESSAGE_SRC(msg)==GST_OBJECT(data.pipeline)){
                    GstState old_state,new_state,pending_state;
                    gst_message_parse_state_changed(msg,&old_state,&new_state,&pending_state);
                    g_print("Pipeline state changed from %s to %s :\n",gst_element_state_get_name(old_state),gst_element_state_get_name(new_state));
                }
                break;
            
            default:
                g_printerr("Unexpected message received .\n");
                break;
            }
            gst_message_unref(msg);

        }
    }while(!terminate);
    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    return 0;
}

static void pad_added_handler(GstElement *src,GstPad *new_pad,Custom_Data *data){
    GstPad *sink_pad  = gst_element_get_static_pad(data->convert,"sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = nullptr;
    GstStructure *new_pad_struct = nullptr;
    const gchar *new_pad_type = nullptr;

    g_print("Received new pad '%s' from '%s' :\n",GST_PAD_NAME(new_pad),GST_ELEMENT_NAME(src));

    /* if our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked(sink_pad)){
        g_print("We are already linked . Ignoring \n");
        goto exit;
    } 

    /* Check the new pad's type*/
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    new_pad_struct = gst_caps_get_structure(new_pad_caps,0);
    new_pad_type = gst_structure_get_name(new_pad_struct);
    if (!g_str_has_prefix(new_pad_type,"video/x-raw")){
        g_print("It has type %s which is not raw audio.Ignoring.\n",new_pad_type);
        goto exit;

    }

    /* Attempt the link */
    ret = gst_pad_link(new_pad,sink_pad);
    if (GST_PAD_LINK_FAILED(ret)){
        g_print("Type is %s but link failed \n.",new_pad_type);
    }else{
        g_print("Link succeeded (type %s).\n",new_pad_type);
    }
    exit:
    /* Unreference the new pad's cap, if we got them*/
    if (new_pad_caps!=nullptr){
        gst_caps_unref(new_pad_caps);
    }
    /* Unreference the sink pad */
    gst_object_unref(sink_pad);

}