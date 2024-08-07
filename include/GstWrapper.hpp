#ifndef GSTREAMER_BOX_INCLUDE_GSTWRAPPER_HPP
#define GSTREAMER_BOX_INCLUDE_GSTWRAPPER_HPP

#include <string>
#include <string_view>

class _GstElement;
class _GstBus;
class _GstMessage;

namespace gst_box {

  class GstWrapper {
  private:
    _GstElement* pipeline;
    _GstBus* bus;
    _GstMessage* msg;

    explicit GstWrapper(std::string_view pipeline_str);

  public:
    static GstWrapper MakePipeline(std::string_view pipeline_str) {
      return GstWrapper(pipeline_str);
    }

    void operator()();

    void stop() const;

    GstWrapper()                              = delete;
    GstWrapper(const GstWrapper&)             = delete;
    GstWrapper(GstWrapper&&)                  = delete;
    GstWrapper& operator=(const GstWrapper)   = delete;
    GstWrapper& operator=(GstWrapper&& other) = delete;

    ~GstWrapper();
  };
}// namespace gst_box
#endif//GSTREAMER_BOX_INCLUDE_GSTWRAPPER_HPP
