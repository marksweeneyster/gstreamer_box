#include "GstWrapper.hpp"
#include <gst/gst.h>

namespace gst_box {

  GstWrapper::GstWrapper(std::string_view pipeline_str)
      : bus(nullptr), msg(nullptr) {
    pipeline = gst_parse_launch(std::string(pipeline_str).c_str(), nullptr);
  }

  GstWrapper::~GstWrapper() {
    if (bus) {
      gst_object_unref(bus);
    }
    if (msg) {
      gst_message_unref(msg);
    }
    if (pipeline) {
      gst_element_set_state(pipeline, GST_STATE_NULL);
      gst_object_unref(pipeline);
    }
  }

  void GstWrapper::operator()() {
    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    auto msgType =
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, msgType);

    /* See next tutorial for proper error message handling/parsing */
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
      g_printerr("An error occurred! Re-run with the GST_DEBUG=*:WARN "
                 "environment variable set for more details.\n");
    }
  }
  void GstWrapper::stop() const {
    if (pipeline) {
      gst_element_send_event(pipeline, gst_event_new_eos());
    }
  }

}// namespace gst_box
