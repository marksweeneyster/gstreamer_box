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
                              "config-interval=1 ! udpsink host=224.1.1.1 port=5004 auto-multicast=true",
                              nullptr);
  /**
   * Running from windows, clients can then receive this via:
   *   gst-launch-1.0 udpsrc caps="application/x-rtp, media=(string)video, \
   *     payload=(int)96, encoding-name=(string)MP4V-ES, ssrc=(uint)3209630580, \
   *     timestamp-offset=(uint)1684226888, seqnum-offset=(uint)5028, \
   *     a-framerate=(string)30" \
   *     multicast-group=224.1.1.1 auto-multicast=true port=5004 ! \
   *     rtpmp4vdepay ! avdec_mpeg4 ! autovideosink
   */

  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  bus = gst_element_get_bus(pipeline);
  auto msgType =
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
