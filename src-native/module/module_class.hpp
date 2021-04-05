#pragma once

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <functional>
#include <memory>
#include <new>
#include <string>
#include <type_traits>

// pulseaudio wasn't expecting a module to be written in c++, huh?
extern "C" {
#include <pulse/def.h>
#include <pulse/xmalloc.h>

#include <pulsecore/core-util.h>
#include <pulsecore/i18n.h>
#include <pulsecore/log.h>
#include <pulsecore/ltdl-helper.h>
#include <pulsecore/macro.h>
#include <pulsecore/mix.h>
#include <pulsecore/modargs.h>
#include <pulsecore/module.h>
#include <pulsecore/namereg.h>
#include <pulsecore/rtpoll.h>
#include <pulsecore/sample-util.h>
#include <pulsecore/sink.h>
#include <pulsecore/thread.h>
}

#include <denoiser.h>

constexpr int MEMBLOCKQ_MAXLENGTH = (16 * 1024 * 1024);
constexpr int BLOCK_USEC = 1000; /* FIXME */

class Module {
public:
  Module(pa_module *m);
  ~Module();

  pa_source *source;

private:
  const char *valid_modargs[11] = {"source_properties",
                                   "master",
                                   "uplink_sink",
                                   "format",
                                   "rate",
                                   "channels",
                                   "channel_map",
                                   "use_volume_sharing",
                                   "force_flat_volume",
                                   NULL};
  pa_module *module;

  /* FIXME: Uncomment this and take "autoloaded" as a modarg if this is a filter
   */
  /* bool autoloaded; */

  pa_source_output *source_output;

  bool auto_desc;
  unsigned channels;

  pa_source *master = nullptr;
  pa_sample_spec ss = {
      .format = PA_SAMPLE_FLOAT32LE, .rate = 16384, .channels = 1};
  pa_channel_map map;
  std::shared_ptr<pa_thread> thread;
  bool use_volume_sharing = true, force_flat_volume = false;
  char source_name[64] = "Magic Mic";
  pa_atomic_t run_thread;
  size_t max_q_len = 1048576, prebuf = 0,
         maxrewind = 1048576; // TODO consider this
  std::shared_ptr<pa_memblockq> memblockq_in, memblockq_out;
  std::shared_ptr<pa_rtpoll> rtpoll;

  Denoiser denoiser;

  void store_modargs(std::shared_ptr<pa_modargs> ma);
  void create_source(std::shared_ptr<pa_modargs> ma);
  void create_source_output();

  // Callback functions:
  // =================================================================
  static void thread_func(void *userdata);

  /* Called from I/O thread context */
  static int source_process_msg_cb(pa_msgobject *o, int code, void *data,
                                   int64_t offset, pa_memchunk *chunk);
  /* Called from main context */
  static int
  source_set_state_in_main_thread_cb(pa_source *s, pa_source_state_t state,
                                     pa_suspend_cause_t suspend_cause);

  /* Called from I/O thread context */
  static void source_update_requested_latency_cb(pa_source *s);
  /* Called from main context */
  static void source_set_volume_cb(pa_source *s);
  /* Called from main context */
  static void source_set_mute_cb(pa_source *s);
  /* Called from input thread context */
  static void source_output_push_cb(pa_source_output *o,
                                    const pa_memchunk *chunk);
  /* Called from input thread context */
  static void source_output_process_rewind_cb(pa_source_output *o,
                                              size_t nbytes);
  /* Called from output thread context */
  static void source_output_update_max_rewind_cb(pa_source_output *o,
                                                 size_t nbytes);
  /* Called from output thread context */
  static void source_output_attach_cb(pa_source_output *o);
  /* Called from output thread context */
  static void source_output_detach_cb(pa_source_output *o);
  /* Called from output thread context except when cork() is called without
   * valid source.*/
  static void source_output_state_change_cb(pa_source_output *o,
                                            pa_source_output_state_t state);

  /* Called from main thread */
  static void source_output_kill_cb(pa_source_output *o);

  /* Called from main thread */
  static void source_output_moving_cb(pa_source_output *o, pa_source *dest);
  // =================================================================
};
