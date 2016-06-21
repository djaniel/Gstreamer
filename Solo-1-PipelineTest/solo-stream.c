
/*
 * 
 * Author: Daniel Soto
 * Date: June 21st, 2016
 * 
 * 
 */


#include <gst/gst.h>
#include <glib.h>


/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    GstElement *pipeline;
  
    GstElement *udpsrc;
    GstElement *capsfilter;
    GstElement *rtpjitterbuffer;
    GstElement *rtph264depay;
    GstElement *dec_h264;
    GstElement *video_convert;
    GstElement *video_sink;

} CustomData;

static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *) data;

    switch (GST_MESSAGE_TYPE (msg)) {

        case GST_MESSAGE_EOS:
            g_print ("End of stream\n");
            g_main_loop_quit (loop);
            break;

        case GST_MESSAGE_ERROR: {
            gchar  *debug;
            GError *error;

            gst_message_parse_error (msg, &error, &debug);
            g_free (debug);

            g_printerr ("Error: %s\n", error->message);
            g_error_free (error);

            g_main_loop_quit (loop);
            break;
        }
        default:
            break;
    }

    return TRUE;
}

static gboolean print_field (GQuark field, const GValue * value, gpointer pfx) {
    gchar *str = gst_value_serialize (value);
   
    g_print ("%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string (field), str);
    g_free (str);
    return TRUE;
}

static void print_caps (const GstCaps * caps, const gchar * pfx) {
    guint i;
   
    g_return_if_fail (caps != NULL);
   
    if (gst_caps_is_any (caps)) {
        g_print ("%sANY\n", pfx);
        return;
    }
    if (gst_caps_is_empty (caps)) {
        g_print ("%sEMPTY\n", pfx);
        return;
    }
   
    for (i = 0; i < gst_caps_get_size (caps); i++) {
        GstStructure *structure = gst_caps_get_structure (caps, i);
     
        g_print ("%s%s\n", pfx, gst_structure_get_name (structure));
        gst_structure_foreach (structure, print_field, (gpointer) pfx);
    }
}
/* Shows the CURRENT capabilities of the requested pad in the given element */
static void print_pad_capabilities (GstElement *element, gchar *pad_name) {
    GstPad *pad = NULL;
    GstCaps *caps = NULL;
   
    /* Retrieve pad */
    pad = gst_element_get_static_pad (element, pad_name);
    if (!pad) {
        g_printerr ("Could not retrieve pad '%s'\n", pad_name);
        return;
    }
   
    /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
    caps = gst_pad_get_current_caps (pad);
    if (!caps)
        caps = gst_pad_get_allowed_caps (pad);
   
    /* Print and free */
    g_print ("Caps for the %s pad:\n", pad_name);
    print_caps (caps, "      ");
    gst_caps_unref (caps);
    gst_object_unref (pad);
}

int main(int argc, char *argv[]) {
    GMainLoop *loop;
    
    CustomData data;
    GstBus *bus;
    guint bus_watch_id;
    
    //gboolean link_ok;
    guint gudpPort;
    
    /* Initialize GStreamer */
    gst_init (&argc, &argv);
    loop = g_main_loop_new (NULL, FALSE);
    
    /* Create gstreamer elements */
    data.pipeline       = gst_pipeline_new ("solo-video");
    data.udpsrc         = gst_element_factory_make("udpsrc",        "source");
    data.capsfilter     = gst_element_factory_make("capsfilter",    "capfilter");
    data.rtpjitterbuffer= gst_element_factory_make("rtpjitterbuffer","jbuffer");
    data.rtph264depay   = gst_element_factory_make("rtph264depay",  "depayloader");
    data.dec_h264       = gst_element_factory_make("avdec_h264",    "dec");    
    data.video_convert  = gst_element_factory_make("videoconvert",  "vconvert");    
    data.video_sink     = gst_element_factory_make("autovideosink", "sync");
    
    if (!data.pipeline || !data.udpsrc || !data.capsfilter || !data.rtpjitterbuffer || !data.rtph264depay || !data.video_sink || !data.video_convert ||!data.dec_h264) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }
    
    /* we add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (data.pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);
        
    /* Build the pipeline 
     * 
     * we add all elements into the bin 
     */
    
    gst_bin_add_many (GST_BIN (data.pipeline), data.udpsrc, data.capsfilter, data.rtpjitterbuffer, data.rtph264depay, data.dec_h264, data.video_convert, data.video_sink, NULL);
    
    g_print ("Pipeline is built. \nLinking...\n");
    
    /* Set up the pipeline, first the UDP port to receive the video feed */
    g_object_set (data.udpsrc, "port", 5600, NULL);
    
    {
         /* Set up the capabilities filter to match our application */
        GstCaps *udp_caps;
        
        udp_caps = gst_caps_from_string ("application/x-rtp, media=(string)=video, clock-rate=(int)90000, encoding-name=(int)H264, payload=(int)96");
        g_object_set (data.capsfilter, "caps", udp_caps, NULL);

        gst_caps_unref (udp_caps);
    }
    
    
    /* we link the elements together */
    if (!gst_element_link_many (data.udpsrc, data.capsfilter, data.rtpjitterbuffer, data.rtph264depay, data.dec_h264, data.video_convert, data.video_sink,NULL)) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }
    g_print("UDP settings:\n");
    print_pad_capabilities(data.udpsrc,"src");
    g_object_get (data.udpsrc, "port", &gudpPort, NULL);
    g_print ("               UDP port: %d\n\nReceiving...\n", gudpPort);
    
    g_print("CapsFilter settings:\n");
    print_pad_capabilities(data.capsfilter,"caps");
    
    /* Start playing */
    gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
    
    /* Iterate */
    g_print ("Running...\n");
    g_main_loop_run (loop);
    
    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping playback\n");
    gst_element_set_state (data.pipeline, GST_STATE_NULL);
    
    g_print ("Deleting pipeline\n");
    gst_object_unref (GST_OBJECT (data.pipeline));
    g_source_remove (bus_watch_id);
    g_main_loop_unref (loop);
    
    return 0;
}
