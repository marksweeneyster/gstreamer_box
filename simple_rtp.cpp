#include <iostream>

#include <gst/gst.h>

int main(int argc, char* argv[]) {
  std::cout << "Hola, Gstreamer!\n";

  GstElement* pipeline;
  GstBus* bus;
  GstMessage* msg;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  pipeline = gst_parse_launch("videotestsrc ! avenc_mpeg4 ! rtpmp4vpay "
                              "config-interval=1 ! udpsink host=192.168.0.14",
                              NULL);

  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  bus = gst_element_get_bus(pipeline);
  GstMessageType msgType =
          static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, msgType);

  if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
    g_printerr("An error occurred! Re-run with the GST_DEBUG=*:WARN "
               "environment variable set for more details.\n");
  }

  /* Free resources */
  gst_message_unref(msg);
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}
