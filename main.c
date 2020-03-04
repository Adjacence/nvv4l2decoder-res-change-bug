#include <gst/gst.h>

#include <stdlib.h>


#define H264_ENCODER "x264enc"


int main(int argc, char** argv)
{
  gst_init(&argc, &argv);

  gboolean stop_on_change = FALSE;

  if (argc >= 2 && g_strcmp0(argv[1], "stop-on-change") == 0)
    stop_on_change = TRUE;

  const char* const pipeline_launch_str
    = "videotestsrc pattern=ball is-live=TRUE num-buffers=150 "
      "! capsfilter name=src_caps_filter "
      " caps=video/x-raw,format=I420,width=1280,height=720,framerate=15/1 "
      "! " H264_ENCODER " "
      "! h264parse "
      "! nvv4l2decoder "
      "! nvvidconv "
      "! " H264_ENCODER " "
      "! h264parse "
      "! splitmuxsink location=video%d.mkv muxer-factory=matroskamux ";

  GError* error = NULL;
  GstElement* const pipeline = gst_parse_launch(pipeline_launch_str, &error);

  if (error != NULL)
  {
    g_printerr("Error parsing pipeline launch string \"%s\": \"%s\"\n",
                pipeline_launch_str,
                error->message);
    g_error_free(error);
  }

  if (pipeline == NULL)
    return EXIT_FAILURE;

  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  GstBus* const bus = gst_element_get_bus(pipeline);

  gboolean done = FALSE, error_occurred = FALSE, fps_changed = FALSE;

  do
  {
    GstMessage* const message = gst_bus_timed_pop_filtered(
                                  bus,
                                  GST_CLOCK_TIME_NONE,
                                  GST_MESSAGE_EOS
                                  | GST_MESSAGE_ERROR
                                  | GST_MESSAGE_WARNING
                                  | GST_MESSAGE_INFO
                                  | GST_MESSAGE_STATE_CHANGED);

    if (message != NULL)
    {
      switch (GST_MESSAGE_TYPE(message))
      {
        case GST_MESSAGE_EOS:
          g_print("Received EOS\n");
          done = TRUE;
          break;
        case GST_MESSAGE_ERROR:
        {
          GError* error = NULL;
          gchar* debug_info = NULL;

          gst_message_parse_error(message, &error, &debug_info);

          g_printerr("ERROR from element %s: %s\n",
                     GST_OBJECT_NAME(message->src),
                     error->message);

          if (debug_info != NULL)
          {
            g_printerr("Debug info: %s\n", debug_info);
            g_free(debug_info);
          }

          g_error_free(error);

          done = TRUE;
          error_occurred = TRUE;

          break;
        }
        case GST_MESSAGE_WARNING:
        {
          GError* error = NULL;
          gchar* debug_info = NULL;

          gst_message_parse_warning(message, &error, &debug_info);

          g_printerr("WARNING from element %s: %s\n",
                     GST_OBJECT_NAME(message->src),
                     error->message);

          if (debug_info != NULL)
          {
            g_printerr("Debug info: %s\n", debug_info);
            g_free(debug_info);
          }

          g_error_free(error);

          break;
        }
        case GST_MESSAGE_INFO:
        {
          GError* error = NULL;
          gchar* debug_info = NULL;

          gst_message_parse_info(message, &error, &debug_info);

          g_printerr("INFO from element %s: %s\n",
                     GST_OBJECT_NAME(message->src),
                     error->message);

          if (debug_info != NULL)
          {
            g_printerr("Debug info: %s\n", debug_info);
            g_free(debug_info);
          }

          g_error_free(error);

          break;
        }
        case GST_MESSAGE_STATE_CHANGED:
        {
          GstState old_state, new_state;

          gst_message_parse_state_changed(
            message, &old_state, &new_state, NULL);

          g_print("Element %s changed state from %s to %s\n",
                  GST_OBJECT_NAME(message->src),
                  gst_element_state_get_name(old_state),
                  gst_element_state_get_name(new_state));

          if (! fps_changed
              && new_state == GST_STATE_PLAYING
              && GST_OBJECT(pipeline) == message->src)
          {
            g_print("Pipeline is playing - attempting to change resolution\n");

            if (stop_on_change)
            {
              g_print("Attempting to set pipeline to NULL before resolution "
                      "change\n");

              const GstStateChangeReturn state_change_ret
                = gst_element_set_state(pipeline, GST_STATE_NULL);

              if (state_change_ret == GST_STATE_CHANGE_FAILURE)
              {
                g_printerr("Failed to stop pipeline\n");
                break;
              }
            }

            GstElement* const src_caps_filter
              = gst_bin_get_by_name(GST_BIN_CAST(pipeline), "src_caps_filter");
            g_assert_nonnull(src_caps_filter);

            GstCaps* const new_caps
              = gst_caps_new_simple("video/x-raw",
                                    "format", G_TYPE_STRING, "I420",
                                    "width", G_TYPE_INT, 720,
                                    "height", G_TYPE_INT, 1280,
                                    "framerate", GST_TYPE_FRACTION, 15, 1,
                                    NULL);

            g_object_set(src_caps_filter, "caps", new_caps, NULL);

            gst_caps_unref(new_caps);
            gst_object_unref(src_caps_filter);

            if (stop_on_change)
            {
              g_print("Attempting to set pipeline to PLAYING after resolution "
                      "change\n");
              gst_element_set_state(pipeline, GST_STATE_PLAYING);
            }

            fps_changed = TRUE;
          }

          break;
        }
      }

      gst_message_unref(message);
    }
  }
  while (! done);

  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return (error_occurred) ? EXIT_FAILURE : EXIT_SUCCESS;
}
