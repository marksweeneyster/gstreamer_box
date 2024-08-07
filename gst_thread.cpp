#include <gst/gst.h>
#include "GstWrapper.hpp"
#include <thread>

int main(int argc, char* argv[]) {

  gst_init(&argc, &argv);

  auto gst_win_webcam = gst_box::GstWrapper::MakePipeline("mfvideosrc device-index=0 ! "
                                                  "video/x-raw,format=NV12,width=640,framerate=30/1 "
                                                  "! fpsdisplaysink");

  auto gst_trailer = gst_box::GstWrapper::MakePipeline("playbin "
                                                       "uri=https://gstreamer.freedesktop.org/data/"
                                                       "media/sintel_trailer-480p.webm");

  auto gst_vid_test = gst_box::GstWrapper::MakePipeline("videotestsrc ! autovideosink");

  auto gst_rtp_meerkat= gst_box::GstWrapper::MakePipeline("videotestsrc ! avenc_mpeg4 ! rtpmp4vpay "
                                                           "config-interval=1 ! udpsink host=192.168.0.14");

  std::thread t([&gst_win_webcam]() { gst_win_webcam(); });
  std::thread t2([&gst_trailer]() { gst_trailer(); });
  std::thread t3([&gst_vid_test]() { gst_vid_test(); });
  std::thread t4(&gst_box::GstWrapper::operator(), &gst_rtp_meerkat);

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(20s);

  gst_vid_test.stop();
  gst_trailer.stop();
  gst_win_webcam.stop();
  gst_rtp_meerkat.stop();

  t.join();
  t2.join();
  t3.join();
  t4.join();
}