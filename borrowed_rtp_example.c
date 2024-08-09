// https://gist.github.com/integer-overflown/302ad2ac029fff5ee9897bb84be36d07

#include <gst/gst.h>
#include <getopt.h>

typedef struct {
    const char *peer_address;
    guint16 rtp_video_src_port, rtp_video_sink_port;
    guint16 rtp_audio_src_port, rtp_audio_sink_port;
    guint16 rtcp_src_port, rtcp_sink_port;
} PeerNetworkInfo;

typedef struct {
    GstElement *pipeline;
    GstElement *rtp_bin;
    GMainLoop *loop;
    GstElement *video_udp_sink, *rtcp_udp_sink;
    GstElement *video_udp_src, *rtcp_udp_src;
    GstElement *audio_udp_src, *audio_udp_sink;
} CustomData;

static void handle_bus_message(GstBus *, GstMessage *, CustomData *);

static void rtp_bin_pad_added(GstElement *self,
                              GstPad *new_pad,
                              CustomData *user_data);

// Links RTP video/audio pads
static gboolean link_media_pads(CustomData *);
// Links RTCP pads
static gboolean link_rtcp_pads(CustomData *);

// for more laconic error handling
static GstElement *create_bin_generic(GstElement *embed_into, const char *description);

// encoding/decoding element chains
static GstElement *create_video_send_pipeline(GstElement *embed_into);

static GstElement *create_video_receive_pipeline(GstElement *embed_into);

static GstElement *create_audio_send_pipeline(GstElement *embed_into);

static GstElement *create_audio_receive_pipeline(GstElement *embed_into);

enum {
    // error codes
    ERC_ERRONEOUS_PIPELINE = 1,
    ERC_PAD_RETRIEVAL_FAILURE,
    ERC_INVALID_USAGE,
    ERC_PLAYING_FAILURE
};

static gboolean init_pipeline(CustomData *data, const PeerNetworkInfo *info) {
    data->pipeline = gst_pipeline_new("main-pipeline");
    data->rtp_bin = gst_element_factory_make("rtpbin", "rtp-bin");
    data->loop = g_main_loop_new(NULL, FALSE);
    data->video_udp_sink = gst_element_factory_make("udpsink", "video_udp_sink");
    data->rtcp_udp_sink = gst_element_factory_make("udpsink", "rtcp_udp_sink");
    data->audio_udp_sink = gst_element_factory_make("udpsink", "audio_udp_sink");
    data->video_udp_src = gst_element_factory_make("udpsrc", "video_udp_src");
    data->rtcp_udp_src = gst_element_factory_make("udpsrc", "rtcp_udp_src");
    data->audio_udp_src = gst_element_factory_make("udpsrc", "audio_udp_src");

    if (!(data->pipeline &&
          data->video_udp_sink && data->video_udp_src &&
          data->rtcp_udp_src && data->rtcp_udp_sink &&
          data->rtp_bin)) {
        g_printerr("Failed to create a pipeline.");
        return FALSE;
    }

    GstCaps *video_udp_src_caps = gst_caps_from_string(
            "application/x-rtp,"
            "media=(string)video,"
            "clock-rate=(int)90000,"
            "encoding-name=(string)THEORA"
    );

    GstCaps *audio_udp_src_caps = gst_caps_from_string(
            "application/x-rtp,"
            "media=(string)audio,"
            "clock-rate=(int)48000,"
            "encoding-name=(string)OPUS"
    );

    g_object_set(data->video_udp_src,
                 "port", info->rtp_video_src_port,
                 // caps MUST be set, otherwise the data won't further the pipeline, just being dropped
                 "caps", video_udp_src_caps,
                 NULL);
    g_object_set(data->video_udp_sink,
                 "host", info->peer_address,
                 "port", info->rtp_video_sink_port,
                 NULL);
    g_object_set(data->rtcp_udp_src,
                 "port", info->rtcp_src_port,
                 NULL);
    g_object_set(data->rtcp_udp_sink,
                 "host", info->peer_address,
                 "port", info->rtcp_sink_port,
                 NULL);
    g_object_set(data->audio_udp_src,
                 "caps", audio_udp_src_caps,
                 "port", info->rtp_audio_src_port,
                 NULL);
    g_object_set(data->audio_udp_sink,
                 "host", info->peer_address,
                 "port", info->rtp_audio_sink_port,
                 NULL);
    gst_bin_add_many(GST_BIN(data->pipeline),
                     data->rtp_bin,
                     data->video_udp_sink, data->video_udp_src,
                     data->rtcp_udp_sink, data->rtcp_udp_src,
                     data->audio_udp_sink, data->audio_udp_src,
                     NULL);

    g_print(
            "BEGIN UDP ports map:\nVideo Source: %d\nVideo Sink: %d\n"
            "Audio Source: %d\nAudio Sink: %d\n"
            "RTCP Source: %d\nRTCP Sink: %d\nEND\n\n",
            info->rtp_video_src_port, info->rtp_video_sink_port,
            info->rtp_audio_src_port, info->rtp_audio_sink_port,
            info->rtcp_src_port, info->rtcp_sink_port);

    gst_caps_unref(audio_udp_src_caps);
    gst_caps_unref(video_udp_src_caps);

    return TRUE;
}

static int app_try_init(int argc, char *argv[], CustomData *data) {
    const struct option long_opts[] = {
            // host address
            {"address",          required_argument, NULL, 'a'},
            // receive UDP datagrams on port numbers starting with base-source-port
            {"base-source-port", required_argument, NULL, 's'},
            // send datagrams to ports starting with base-sink-port
            {"base-sink-port",   required_argument, NULL, 'd'}
    };

    char *peer_address = NULL;
    guint16 base_source_port = 5000, base_sink_port = 5000;

    for (;;) {
        int opt_index;
        int result = getopt_long(argc, argv, "a:", long_opts, &opt_index);
        if (result < 0) break;
        switch (result) {
            case 'a':
                peer_address = optarg;
                break;
            case 's':
            case 'd': {
                long port = strtol(optarg, NULL, 10);
                if (port == 0) {
                    g_printerr("Invalid port number: %s.\n", optarg);
                    return ERC_INVALID_USAGE;
                }
                if (port < 0 || port > 65535) {
                    g_printerr("Port number exceeds the valid range [0-65535].\n");
                    return ERC_INVALID_USAGE;
                }
                result == 's' ? (base_source_port = port) : (base_sink_port = port);
                break;
            }
            case '?':
            default:
                g_printerr("Unknown option: %c\n.", result);
                return ERC_INVALID_USAGE;
        }
    }

    PeerNetworkInfo peerNetworkInfo = {
            .peer_address = peer_address ? peer_address : "0.0.0.0",
            .rtp_video_src_port = base_source_port,
            .rtp_video_sink_port = base_sink_port,
            .rtcp_src_port = base_source_port + 1,
            .rtcp_sink_port = base_sink_port + 1,
            .rtp_audio_src_port = base_source_port + 2,
            .rtp_audio_sink_port = base_sink_port + 2
    };

    memset(data, 0, sizeof(CustomData));
    if (!init_pipeline(data, &peerNetworkInfo)) return ERC_ERRONEOUS_PIPELINE;

    return 0;
}

// To run on one machine
// ./gstreamer_basics --base-source-port 12345
// ./gstreamer-basics --base-sink-port 12345
// To run on local network
// Computer A: ./gstreamer_basic --address <B's address>
// Computer B: ./gstreamer_basic --address <A's address>
int main(int argc, char *argv[]) {
    CustomData data;

    {
        gst_init(&argc, &argv);
        int code = app_try_init(argc, argv, &data);
        if (code != 0) return code; // error has been already reported, just exit
    }

    g_signal_connect(data.rtp_bin, "pad-added", G_CALLBACK(rtp_bin_pad_added), &data);

    // RTP
    g_return_val_if_fail(link_media_pads(&data), ERC_ERRONEOUS_PIPELINE);
    // RTCP
    g_return_val_if_fail(link_rtcp_pads(&data), ERC_ERRONEOUS_PIPELINE);

    GstBus *bus = gst_element_get_bus(data.pipeline);
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message", G_CALLBACK(handle_bus_message), &data);

    GstStateChangeReturn ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Couldn't set pipeline's state to PLAYING.\n");
        return ERC_PLAYING_FAILURE;
    }

    g_main_loop_run(data.loop);

    g_main_loop_unref(data.loop);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    return 0;
}

void handle_bus_message(GstBus *bus, GstMessage *msg, CustomData *data) {
    (void) bus;
    GError *err = NULL;
    gchar *debug_info = NULL;
    GstMessageType messageType = GST_MESSAGE_TYPE(msg);

    switch (messageType) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debugging information: code %i, %s\n", err->code, debug_info ? debug_info : "none");
            g_clear_error(&err);
            g_free(debug_info);
            g_main_loop_quit(data->loop);
            break;
        case GST_MESSAGE_EOS:
            g_print("End-Of-Stream reached.\n");
            break;
        default:;
    }
}

gboolean link_rtcp_pads(CustomData *data) {
    GstPad *rtcp_send_src = gst_element_get_request_pad(data->rtp_bin, "send_rtcp_src_%u");
    GstPad *rtcp_send_sink = gst_element_get_request_pad(data->rtp_bin, "recv_rtcp_sink_%u");

    if (!rtcp_send_src || !rtcp_send_sink) return FALSE;

    GstPad *rtcp_udp_sink = gst_element_get_static_pad(data->rtcp_udp_sink, "sink"),
            *rtcp_udp_src = gst_element_get_static_pad(data->rtcp_udp_src, "src");

    g_assert(rtcp_udp_sink && rtcp_udp_src);

    g_object_set(data->rtcp_udp_sink, "sync", FALSE, "async", FALSE, NULL);
    gst_pad_link(rtcp_send_src, rtcp_udp_sink);
    gst_pad_link(rtcp_udp_src, rtcp_send_sink);

    gst_object_unref(rtcp_udp_sink);
    gst_object_unref(rtcp_udp_src);

    return TRUE;
}

gboolean link_media_pads(CustomData *data) {
    GstPad *video_sink = gst_element_get_request_pad(data->rtp_bin, "send_rtp_sink_%u"),
            *audio_sink = gst_element_get_request_pad(data->rtp_bin, "send_rtp_sink_%u");
    GstPad *video_receiver_sink = gst_element_get_request_pad(data->rtp_bin, "recv_rtp_sink_%u"),
            *audio_receiver_sink = gst_element_get_request_pad(data->rtp_bin, "recv_rtp_sink_%u");

    if (!(video_sink && audio_sink && video_receiver_sink && audio_receiver_sink)) {
        g_printerr("rtpbin: failed to allocate media pads by request.");
        return ERC_ERRONEOUS_PIPELINE;
    }

    GstElement *video_send_pipeline = create_video_send_pipeline(data->pipeline);
    GstElement *audio_send_pipeline = create_audio_send_pipeline(data->pipeline);

    if (!(video_send_pipeline && audio_send_pipeline)) return FALSE;

    // Linking pads
    // RTP - Sending video
    GstPad *video_send_src_pad = gst_element_get_static_pad(video_send_pipeline, "src");
    g_return_val_if_fail(video_send_src_pad != NULL, ERC_PAD_RETRIEVAL_FAILURE);
    // this will trigger creation of send_rtp_source pads and will be linked in the callback
    gst_pad_link(video_send_src_pad, video_sink);

    // RTP - Receiving video
    GstPad *video_udp_src_pad = gst_element_get_static_pad(data->video_udp_src, "src");
    g_return_val_if_fail(video_udp_src_pad != NULL, ERC_PAD_RETRIEVAL_FAILURE);
    gst_pad_link(video_udp_src_pad, video_receiver_sink);

    // RTP - Sending audio
    GstPad *audio_send_src_pad = gst_element_get_static_pad(audio_send_pipeline, "src");
    g_return_val_if_fail(audio_send_src_pad != NULL, ERC_PAD_RETRIEVAL_FAILURE);
    gst_pad_link(audio_send_src_pad, audio_sink);

    // RTP - Sending audio
    GstPad *audio_udp_src_pad = gst_element_get_static_pad(data->audio_udp_src, "src");
    g_return_val_if_fail(video_udp_src_pad != NULL, ERC_PAD_RETRIEVAL_FAILURE);
    gst_pad_link(audio_udp_src_pad, audio_receiver_sink);

    gst_object_unref(audio_send_src_pad);
    gst_object_unref(audio_udp_src_pad);
    gst_object_unref(video_send_src_pad);
    gst_object_unref(video_udp_src_pad);
    gst_object_unref(audio_receiver_sink);
    gst_object_unref(video_receiver_sink);
    gst_object_unref(audio_sink);
    gst_object_unref(video_sink);
    return TRUE;
}

void rtp_bin_pad_added(GstElement *self,
                       GstPad *new_pad,
                       CustomData *user_data) {
    (void) self;
    gchar *new_pad_name = gst_pad_get_name(new_pad);

    const char *send_prefix = "send_rtp_src_";
    const char *receive_prefix = "recv_rtp_src_";
    enum {
        VIDEO_STREAM_ID = 0,
        AUDIO_STREAM_ID = 1
    };

    GstPad *sink_pad = NULL;
    GstElement *play_element = NULL;

    gboolean is_send = g_str_has_prefix(new_pad_name, send_prefix);
    gboolean is_receive = !is_send && g_str_has_prefix(new_pad_name, receive_prefix);

    if (!(is_send || is_receive)) {
        g_print("Got a pad '%s'\n", new_pad_name);
        goto exit;
    }

    long sess_id = strtol(new_pad_name + strlen(is_send ? send_prefix : receive_prefix), NULL, 10);

    if (sess_id > AUDIO_STREAM_ID) {
        g_printerr("Unknown session ID: %ld\n", sess_id);
        goto exit;
    }

    if (is_send) {
        g_print("RTP SENDER: got a src pad '%s' with id %ld\n", new_pad_name, sess_id);
        if (sess_id == VIDEO_STREAM_ID) {
            sink_pad = gst_element_get_static_pad(play_element = user_data->video_udp_sink, "sink");
        } else if (sess_id == AUDIO_STREAM_ID) {
            sink_pad = gst_element_get_static_pad(play_element = user_data->audio_udp_sink, "sink");
        }
    } else {
        g_print("RTP RECEIVER: got a src pad '%s' with id %ld\n", new_pad_name, sess_id);
        if (sess_id == VIDEO_STREAM_ID) {
            GstElement *video_receiver = create_video_receive_pipeline(user_data->pipeline);
            sink_pad = gst_element_get_static_pad(video_receiver, "sink");
        } else if (sess_id == AUDIO_STREAM_ID) {
            GstElement *audio_receiver = create_audio_receive_pipeline(user_data->pipeline);
            sink_pad = gst_element_get_static_pad(audio_receiver, "sink");
        }
    }

    GstPadLinkReturn pad_link_ret = gst_pad_link(new_pad, sink_pad);

    if (GST_PAD_LINK_FAILED(pad_link_ret)) {
        g_printerr("rtpbin: on-pad-added: linking source pad '%s' failed with code: %d\n", new_pad_name, pad_link_ret);
        if (pad_link_ret == GST_PAD_LINK_NOFORMAT) {
            GstCaps *src_caps = gst_pad_query_caps(new_pad, NULL);
            GstCaps *sink_caps = gst_pad_query_caps(sink_pad, NULL);
            gchar *src_caps_str = gst_caps_to_string(src_caps), *sink_caps_str = gst_caps_to_string(sink_caps);
            g_printerr("Src pad format:\n%s\nSink pad format:\n%s\n", src_caps_str, sink_caps_str);
            g_free(sink_caps_str);
            g_free(src_caps_str);
            gst_caps_unref(sink_caps);
            gst_caps_unref(src_caps);
        }
        g_main_loop_quit(user_data->loop);
    }

    if (play_element && gst_element_set_state(play_element, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        gchar *name = gst_element_get_name(play_element);
        g_printerr("Failed to change the state of the element '%s' to PLAYING.\n", name);
        g_free(name);
    }

    exit:
    if (sink_pad) g_object_unref(sink_pad);
    g_free(new_pad_name);
}

static GstElement *create_bin_generic(GstElement *embed_into, const char *description) {
    GError *error = NULL;
    // by setting TRUE we request creating ghost pads, thus our bin can be used as an element
    // (since having 'src' and 'sink' pads)
    GstElement *bin = gst_parse_bin_from_description(description, TRUE,
                                                     &error);
    if (error) {
        char *name = gst_element_get_name(bin);
        g_printerr("An error occurred while creating a bin [name='%s']: %s\n", name, error->message);
        g_free(name);
        g_error_free(error);
        return NULL;
    }

    gst_bin_add(GST_BIN(embed_into), bin);
    g_assert(gst_element_sync_state_with_parent(bin));
    return bin;
}

GstElement *create_video_send_pipeline(GstElement *embed_into) {
    // for theora, config sending is disabled by default
    // but before receiving a config packet the video won't be decoded
    return create_bin_generic(embed_into,
                              "videotestsrc is-live=true ! videoconvert ! theoraenc ! "
                              // request sending config packets every one second
                              "rtptheorapay config-interval=1");
}

GstElement *create_video_receive_pipeline(GstElement *embed_into) {
    return create_bin_generic(embed_into, "rtptheoradepay ! theoradec ! videoconvert ! ximagesink");
}

GstElement *create_audio_send_pipeline(GstElement *embed_into) {
    return create_bin_generic(embed_into, "audiotestsrc is-live=true ! audioconvert ! opusenc ! rtpopuspay");
}

GstElement *create_audio_receive_pipeline(GstElement *embed_into) {
    return create_bin_generic(embed_into, "rtpopusdepay ! opusdec ! audioconvert ! autoaudiosink");
}
