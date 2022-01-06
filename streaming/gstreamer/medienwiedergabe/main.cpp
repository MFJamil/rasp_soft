/**
 * @file main.cpp
 * @brief demo to play and switch between videos
 */
#include <iostream>
#include <gst/gst.h>
#include <string>
#include <thread>
using namespace std;

struct Custom_Data{
     GstElement *pipeline;
     GstElement *source;
     GstElement *aconvert;
     GstElement *vconvert;
     GstElement *resample;
     GstElement *asink;
     GstElement *vsink;
};


class MediaPlayer{

public:
/**
 * @brief struct containing all important data
 * 
 */

 Custom_Data data {};
GstBus *bus = nullptr;
bool pipeline_created;
const char * current_path; 

int create_elements();
void handle_message(GstMessage *msg);

void listen();
void unref();
void handle();

public:

void pad_added_handler(GstElement *src,GstPad *new_pad);
void start_media();
void stop_media();
void set_media_src(const char* file_path);
MediaPlayer(int arg, char *argv[]);

};


static void pad_added(GstElement *src,GstPad *new_pad, MediaPlayer *mdeiaPlayer){
    mdeiaPlayer->pad_added_handler(src,new_pad);
}
int MediaPlayer::create_elements(){
    g_print("Starting to create the elements ... \n");
    data.source = gst_element_factory_make("uridecodebin","source");
    data.aconvert = gst_element_factory_make("audioconvert","aconvert");
    data.vconvert = gst_element_factory_make("videoconvert","vconvert");
    data.resample = gst_element_factory_make("audioresample","resample");
    data.asink = gst_element_factory_make("autoaudiosink","asink");
    data.vsink = gst_element_factory_make("kmssink","vsink");
    /* create the empty pipeline */
    data.pipeline = gst_pipeline_new("test-pipeline");  
    if(!data.pipeline || !data.source || !data.aconvert || !data.resample || !data.asink || !data.vsink || !data.vconvert){
        g_printerr("Not all elements could be created ! \n");
        return -1;
    }
    /*  Build the pipeline */
    gst_bin_add_many(GST_BIN(data.pipeline), data.source,data.aconvert, data.resample, data.asink,data.vconvert,  data.vsink,NULL);   

    if (!gst_element_link_many(data.aconvert,data.resample,data.asink,NULL)){
        gst_printerr("Audio Elements could not be linked \n");
        gst_object_unref(data.pipeline);
        return -1;
    }
    if (!gst_element_link_many(data.vconvert,data.vsink,NULL)){
        gst_printerr("Video Elements could not be linked \n");
        gst_object_unref(data.pipeline);
        return -1;
    }

    
    return 0;
}

void MediaPlayer::handle_message(GstMessage *msg){
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


void MediaPlayer::pad_added_handler(GstElement *src,GstPad *new_pad){
    GstPad *asink_pad  = gst_element_get_static_pad(data.aconvert,"sink");
    GstPad *vsink_pad  = gst_element_get_static_pad(data.vconvert,"sink");

    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = nullptr;
    GstStructure *new_pad_struct = nullptr;
    const gchar *new_pad_type = nullptr;
    g_print("Received new pad '%s' from '%s' :\n",GST_PAD_NAME(new_pad),GST_ELEMENT_NAME(src));

    /* if our converter is already linked, we have nothing to do here */
    if ((gst_pad_is_linked(asink_pad))&&(gst_pad_is_linked(vsink_pad))){
        g_print("We are already linked . Ignoring \n");
        goto exit;
    } 

    /* Check the new pad's type*/
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    new_pad_struct = gst_caps_get_structure(new_pad_caps,0);
    new_pad_type = gst_structure_get_name(new_pad_struct);
    g_print("The Pad Type is %s",new_pad_type);
    if (
        (!g_str_has_prefix(new_pad_type,"audio/x-raw"))&&
        (!g_str_has_prefix(new_pad_type,"video/x-raw"))
        ){
        g_print("It has type %s which is not raw audio or Video.Ignoring.\n",new_pad_type);
        goto exit;
    }

    /* Attempt the link */
    if (g_str_has_prefix(new_pad_type,"audio/x-raw"))
        ret = gst_pad_link(new_pad,asink_pad);
    else
        ret = gst_pad_link(new_pad,vsink_pad);
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
    gst_object_unref(asink_pad);
    gst_object_unref(vsink_pad);
}

void MediaPlayer::unref()
{
    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
}


void MediaPlayer::handle()
{
    GstMessage *msg = nullptr;
    bool terminate = false;
    do{
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                        static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (msg != nullptr)
            handle_message(msg);
    }while(!terminate);
    unref();
}




/**
 * @brief to continue the current paused video
 * 
 */
void MediaPlayer::start_media()
{
    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
}

/**
 * @brief to pause the current video
 * 
 */
void MediaPlayer::stop_media()
{
    gst_element_set_state(data.pipeline, GST_STATE_PAUSED);
}

/**
 * @brief set a video path to play 
 * 
 * @param file_path 
 */
void MediaPlayer::set_media_src(const char* file_path)
{
    if (strcmp(file_path,current_path)!=0)
    {
        g_print("Received request to set the media to %s  \n",file_path);
        if(!pipeline_created)
        {
            
            /* connect to the pad-added signal */ 
            g_object_set(data.source,"uri", file_path ,NULL);
            g_signal_connect(data.source,"pad-added",G_CALLBACK(pad_added),this);
            gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
            bus = gst_element_get_bus(data.pipeline);
            pipeline_created = true;
            current_path = file_path;
            handle();
        }else
        {
            g_print("Activating the new one\n");    
            gst_element_set_state(data.pipeline, GST_STATE_READY);
            g_object_set(data.source,"uri", file_path ,NULL);
            gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
            current_path = file_path;
        }
    }else
    {
        g_printerr("Cannot rerun the same URL ! %s",file_path);
    }
    

}


MediaPlayer::MediaPlayer(int arg, char *argv[]) 
{
    pipeline_created = false;
    current_path = "";
    // gstreamer initialization
    gst_init(&arg, &argv);
    /* Create the elements */
    create_elements();
    
   
}



int main (int arg, char *argv[]){
    g_print("Starting the program .... \n");
    MediaPlayer player(arg,argv);
    thread t1(&MediaPlayer::set_media_src,&player,"http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4");
    this_thread::sleep_for(10s);
    g_print("Will switch the video\n");
    player.set_media_src("https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm");
    this_thread::sleep_for(10s);
    g_print("Will switch the video\n");
    player.set_media_src("http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ElephantsDream.mp4");
    this_thread::sleep_for(15s);
    g_print("Will switch the video\n");
    player.set_media_src("http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ForBiggerBlazes.mp4");
    t1.join();
    return 0;
}
