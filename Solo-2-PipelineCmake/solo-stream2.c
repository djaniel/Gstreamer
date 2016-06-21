
#include <gst/gst.h>
#include <glib.h>
#include "SoloUDPConfig.h"
/*
 * Video feed from 3DR Solo
 * 
 * Author: 
 * Daniel Soto Guerrero
 * 
 * Description:
 * This code creates the complete pipeline from a string. 
 * Does not verify the creation of every component on the pipeline.
 * It will only work with Gstreamer 1.0
 * 
 * COMPILATION:
 * gcc -Wall solo-stream2.c -o solo-stream2 $(pkg-config --cflags --libs gstreamer-1.0)
 * 
 * to quit Ctrl+C
 */
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


int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GMainLoop *loop;
    
    GstBus *bus;
    guint bus_watch_id;
    g_print ("Solo UDP video reception. Version %d.%d\n", SoloUDP_VERSION_MAJOR, SoloUDP_VERSION_MINOR);
    
    /* Initialize GStreamer */
    gst_init (&argc, &argv);
    loop = g_main_loop_new (NULL, FALSE);
    
    /* Build the pipeline */
    
    pipeline = gst_parse_launch ("udpsrc port=5600 ! application/x-rtp,encoding-name=H264,payload=96 ! rtpjitterbuffer ! rtph264depay ! avdec_h264 ! videoconvert ! autovideosink",NULL);
    
    if ( !pipeline ) {
        g_printerr ("Pipeline could not be built.\n");
        return -1;
    }
    
    /* we add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);
    
    /* Start playing */
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    
    /* Iterate */
    g_print ("Running...\n");
    g_main_loop_run (loop);
    
    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping playback\n");
    gst_element_set_state (pipeline, GST_STATE_NULL);
    
    g_print ("Deleting pipeline\n");
    gst_object_unref (GST_OBJECT (pipeline));
    g_source_remove (bus_watch_id);
    g_main_loop_unref (loop);
    
    return 0;
}
