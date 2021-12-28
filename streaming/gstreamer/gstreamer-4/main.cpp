#include <iostream>
#include <gst/gst.h>
#include <string>

using namespace std;

/* Structure to contain all our information, so we can pass it around */
struct Custom_Data{
    GstElement *playbin;  /* Our one and only element */
    gboolean playing;     /* Are we in the playing state? */
    gboolean terminate;    /* Should we terminate execution? */
    gboolean seek_enabled; /* Is seeking enabled for this media? */
    gboolean seek_done;    /* Have we performed the seek already ? */
    gint64 duration;       /* How long does this media last, in nanoseconds */   
};

/* Forward definition of the message processing funtion */
static void handle_message(Custom_Data *data, GstMessage *msg);


int main(int arg, char *argv[]) {
    Custom_Data data{};
    GstBus *bus = nullptr;
    GstMessage *msg = nullptr;
    GstStateChangeReturn ret;
    data.playing = FALSE;
    data.terminate = FALSE;
    data.seek_enabled= FALSE;
    data.seek_done = FALSE;
    data.duration = GST_CLOCK_TIME_NONE;


    // gstreamer initialization
    gst_init(&arg, &argv);

  
  
    /* Create the elements */
    data.playbin = gst_element_factory_make("playbin","playbin");
    
    if(!data.playbin){
        g_printerr("Not all elements could be created ! \n");
        return -1;
    }
    
    /* Set the URI to play*/
    /* Below can be replaced with the command line argument to dynamically change the source*/
    //string uriSrc = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";
    string uriSrc = "http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4";

    g_object_set(data.playbin,"uri", uriSrc.c_str() ,NULL);
    
    // start playing
    ret = gst_element_set_state(data.playbin, GST_STATE_PLAYING);
    if (ret==GST_STATE_CHANGE_FAILURE){
        gst_printerr("Unable to set the playbin to playing state \n");
        gst_object_unref(data.playbin);
        return -1;

    }

    //wait until error or EOS ( End Of Stream )
    bus = gst_element_get_bus(data.playbin);
    do{
        msg = gst_bus_timed_pop_filtered(bus, 100 * GST_MSECOND,
                                        static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_DURATION));

        // free memory
        if (msg != nullptr){
            handle_message(&data,msg);
        }else{
           /* We got no message therefore this means that the timeout expired*/
           if (data.playing){
               gint64 current = -1;
               /* Query the current position of the stream  */
               if (!gst_element_query_position(data.playbin,GST_FORMAT_TIME,&current)){
                    g_printerr("Cound not query current position. \n");
               }
               /* If we did not know it yet, query the stream duration */
               if (!GST_CLOCK_TIME_IS_VALID(data.duration)){
                   if (!gst_element_query_duration(data.playbin,GST_FORMAT_TIME,&data.duration)){
                       g_printerr("Cound not query current duration. \n");
                   }
               }
               /* Print current position and total duration  */
               g_print("Position %" GST_TIME_FORMAT " /%" GST_TIME_FORMAT "\r", GST_TIME_ARGS(current),GST_TIME_ARGS(data.duration));
               /*  If seeking is enabled, we have not done it yet, and the time is right, seek */
               if (data.seek_enabled && !data.seek_done && current>10*GST_SECOND){
                   g_print("\n Reached 10S, performing seek ....\n");
                   gst_element_seek_simple(data.playbin,GST_FORMAT_TIME,static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT),(30*GST_SECOND));
                   data.seek_done = TRUE;
               }
           } 
        }
    }while(!data.terminate);
    gst_object_unref(bus);
    gst_element_set_state(data.playbin, GST_STATE_NULL);
    gst_object_unref(data.playbin);

    return 0;
}


static void handle_message(Custom_Data *data, GstMessage *msg){
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
        if(GST_MESSAGE_SRC(msg)==GST_OBJECT(data->playbin)){
            GstState old_state,new_state,pending_state;
            gst_message_parse_state_changed(msg,&old_state,&new_state,&pending_state);
            g_print("Pipeline state changed from %s to %s :\n",gst_element_state_get_name(old_state),gst_element_state_get_name(new_state));
            data->playing = (new_state == GST_STATE_PLAYING);
            if (data->playing){
                /* We just moved to playing, Check if seeking is possible */
                GstQuery *query;
                gint64 start,end;
                query = gst_query_new_seeking(GST_FORMAT_TIME);
                if (gst_element_query(data->playbin,query)){
                    gst_query_parse_seeking(query,NULL,&data->seek_enabled,&start,&end);
                    if(data->seek_enabled){
                        g_print("Seeking is enabled from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(start),GST_TIME_ARGS(end));
                    }else{
                        g_print("Seeking is disabled for this stream\n");
                    }
                    
                }else{
                    g_print("Seeking query failed \n");
                }
                gst_query_unref(query);
            }
        }
        break;
    case GST_MESSAGE_DURATION:
        /* The duration has changed, mark the current one is invalid */
        data->duration = GST_CLOCK_TIME_NONE; 
        break;
    default:
        g_printerr("Unexpected message received .\n");
        break;
    }
    gst_message_unref(msg);

}



