#include <iostream>
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <string>

#define CHUNK_SIZE 1024         /* Amount of bytes we are sending in each buffer */
#define SAMPLE_RATE 44100       /* Samples per second we are sending */

using namespace std;

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct CustomData {
  GstElement *pipeline, *app_source, *tee, *audio_queue, *audio_convert1, *audio_resample, *audio_sink;
  GstElement *video_queue, *audio_convert2, *visual, *video_convert, *video_sink;
  GstElement *app_queue, *app_sink;

  guint64 num_samples;   /* Number of samples generated so far (for timestamp generation) */
  gfloat a, b, c, d;     /* For waveform generation */
  guint sourceid;        /* To control the GSource */

  GMainLoop *main_loop;  /* GLib's Main Loop */
};

int main(int arg, char *argv[]) {
    GstElement *pipeline, *audio_source, *tee, *audio_queue, *audio_convert,
      *audio_resample, *audio_sink;
    GstElement *video_queue, *visual, *video_convert, *video_sink;
    GstBus *bus = nullptr;
    GstMessage *msg = nullptr;
    GstPad *tee_audio_pad, *tee_video_pad;
    GstPad *queue_audio_pad, *queue_video_pad;


    // gstreamer initialization
    gst_init(&arg, &argv);

  
  
    /* Create the elements */
    audio_source = gst_element_factory_make ("audiotestsrc", "audio_source");
    tee = gst_element_factory_make ("tee", "tee");
    audio_queue = gst_element_factory_make ("queue", "audio_queue");
    audio_convert = gst_element_factory_make ("audioconvert", "audio_convert");
    audio_resample = gst_element_factory_make ("audioresample", "audio_resample");
    audio_sink = gst_element_factory_make ("autoaudiosink", "audio_sink");
    video_queue = gst_element_factory_make ("queue", "video_queue");
    visual = gst_element_factory_make ("wavescope", "visual");
    video_convert = gst_element_factory_make ("videoconvert", "video_convert");
    video_sink = gst_element_factory_make ("autovideosink", "video_sink");

    /* create the empty pipeline */
    pipeline = gst_pipeline_new("test-pipeline");
    
  if (!pipeline || !audio_source || !tee || !audio_queue || !audio_convert
      || !audio_resample || !audio_sink || !video_queue || !visual
      || !video_convert || !video_sink) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

    /* Configure elements */
    g_object_set (audio_source, "freq", 215.0f, NULL);
    g_object_set (visual, "shader", 0, "style", 1, NULL);



  /* Link all elements that can be automatically linked because they have "Always" pads */
    gst_bin_add_many (GST_BIN (pipeline), audio_source, tee, audio_queue, audio_convert, audio_sink,
        video_queue, visual, video_convert, video_sink, NULL);
    if (gst_element_link_many (audio_source, tee, NULL) != TRUE ||
        gst_element_link_many (audio_queue, audio_convert, audio_sink, NULL) != TRUE ||
        gst_element_link_many (video_queue, visual, video_convert, video_sink, NULL) != TRUE) {
            g_printerr ("Elements could not be linked.\n");
            gst_object_unref (pipeline);
            return -1;
    }
    
      /* Manually link the Tee, which has "Request" pads */
    tee_audio_pad = gst_element_get_request_pad(tee,"src_%u");
    g_print ("Obtained request pad %s for audio branch.\n", gst_pad_get_name (tee_audio_pad));
    queue_audio_pad = gst_element_get_static_pad(audio_queue, "sink");
    tee_video_pad = gst_element_get_request_pad(tee,"src_%u");
    g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
    queue_video_pad = gst_element_get_static_pad(video_queue, "sink");
    if (
        (gst_pad_link(tee_audio_pad,queue_audio_pad) != GST_PAD_LINK_OK)||
        (gst_pad_link(tee_video_pad,queue_video_pad) != GST_PAD_LINK_OK)
        ){
        g_printerr("Tee could not be linked \n");
        gst_object_unref(pipeline);
        return -1;    
    }
    gst_object_unref(queue_audio_pad);
    gst_object_unref(queue_video_pad);
    // start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
   
    //wait until error or EOS ( End Of Stream )
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                        static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  /* Release the request pads from the Tee, and unref them */
  gst_element_release_request_pad (tee, tee_audio_pad);
  gst_element_release_request_pad (tee, tee_video_pad);
  gst_object_unref (tee_audio_pad);
  gst_object_unref (tee_video_pad);

  /* Free resources */
  if (msg != NULL)
    gst_message_unref (msg);
  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);
  return 0;
}

