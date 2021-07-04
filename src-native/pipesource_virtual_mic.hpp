#pragma once

#include <atomic>
#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

#include <pulse/pulseaudio.h>

#include <boost/circular_buffer.hpp>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

#include "audio_processor.hpp"
#include "virtual_mic.hpp"

using boost::circular_buffer;
using std::atomic;
using std::future;
using std::lock_guard;
using std::mutex;
using std::optional;
using std::pair;
using std::promise;
using std::queue;
using std::shared_ptr;
using std::string;
using std::thread;
using std::vector;

class PipeSourceVirtualMic;
// Only supports a single instance
class PipeSourceVirtualMic : public VirtualMic {
public:
  // TODO Need to implement copy constructors
  PipeSourceVirtualMic(AudioProcessor *denoise,
                       std::shared_ptr<spdlog::logger> logger);
  // threads aren't copyable, so neiter is this
  PipeSourceVirtualMic(PipeSourceVirtualMic &) = delete;
  ~PipeSourceVirtualMic();

  void stop() override;
  void abortLastRequest() override;
  future<bool> getStatus() override;
  future<pair<int, vector<pair<int, string>>>> getMicrophones() override;
  future<void> setMicrophone(int) override;
  future<void> setRemoveNoise(bool) override;
  future<bool> getRemoveNoise() override;
  future<bool> setLoopback(bool) override;
  future<bool> getLoopback() override;
  void setAudioProcessor(AudioProcessor *ap) override;
  future<std::exception_ptr> get_exception_future() override {
    return exception_promise.get_future();
  };
  optional<VirtualMicUpdate> get_update() override;

private:
  std::shared_ptr<spdlog::logger> logger;

  mutex updates_mutex;
  queue<VirtualMicUpdate> updates;

  promise<std::exception_ptr> exception_promise;
  thread async_thread;
  struct GetMicrophones {
    enum GetMicrophonesActionState { Init, Wait, Done };
    enum GetMicrophonesActionState state;
    pa_operation *op;
    vector<pair<int, string>> list;
    promise<pair<int, vector<pair<int, string>>>> p;
  };
  struct SetMicrophone {
    enum SetMicrophoneActionState {
      InitGettingSource,
      WaitGettingSource,
      UpdatingStream
    };
    enum SetMicrophoneActionState state;
    pa_operation *op;
    int ind;
    string name;
    promise<void> p;
  };
  struct CurrentAction {
    enum { NoAction, GetMicrophones, SetMicrophone, Loopback } action;
    // I'm a little worried about the memory management here
    // union {
    struct GetMicrophones get_mics;
    struct SetMicrophone set_mic;
    // };
    CurrentAction() { this->action = NoAction; }
    // ~CurrentAction() {
    //   switch (action) {
    //   case GetMicrophones:
    // 	get_mics.~GetMicrophones();
    // 	break;
    //   case SetMicrophone:
    // 	set_mic.~SetMicrophone();
    // 	break;
    //   case NoAction:
    // 	break;
    //   }
    // }
  };
  enum State {
    InitContext = 1,
    WaitContextReady,
    InitCheckModuleLoaded,
    WaitCheckModuleLoaded,
    InitModule,
    WaitModuleReady,
    GetDefaultSource,
    InitRecStream,
    WaitRecStreamReady,
    Denoise,
    UnloadModule
  };
  friend std::ostream &operator<<(std::ostream &out,
                                  const PipeSourceVirtualMic::State value);
  CurrentAction cur_act;
  State state;
  void changeState(State s);

  // TODO for now I guess a single state thing is ok, but I think it would make
  // more sense to have some sort of state bitfield. It would allow parallel
  // initialization and would overall be more elegant. It would be more
  // resillient and more accuratly reflect the fact that whats relevant isn't
  // really a line of execution but the statse of each resource and operation.
  const char *rec_stream_name = "Magic Mic Recording Stream";
  const char *pb_stream_name = "Magic Mic Playback Stream";
  const char *source = nullptr;
  const char *client_name = "Client Name";
  const int target_latency = 0;
  const size_t buffer_length = 3200;
  const size_t max_denoiser_buffer = 16000;
  const size_t max_read_stream_buffer = 16000;
  pa_sample_spec shared_sample_spec;
  string format_module;
  string pipe_file_name;
  // It appears c++ doesn't do non blocking writes which is a problem for
  // signal handling, so we need to go old school (sort of, still have
  // shared_ptr which is sort of cool I guess)
  int pipe_fd;

  circular_buffer<uint8_t> ring_buf;

  // TODO: I don't think we should allow mainloop to be freed before ctx
  // and rec_stream but, I can't figure out a good way to fix that
  shared_ptr<pa_mainloop> mainloop;
  shared_ptr<pa_context> ctx;
  shared_ptr<pa_stream> rec_stream;
  atomic<bool> should_run;

  int pipesource_module_idx;
  int pipesource_idx;
  pa_operation *module_operation;

  // The playback stream is sort of unrelated to all the other stuff. It should
  // only be inited after everything else is inited, but It shouldn't be inited
  // if it isn't requested. That could cause problems if someone doesn't wan't
  // playback, and just wants recording. Probably a very small number of people
  // (if any) that playback would break something, but whatever.
  // TODO: This should probably be setup simillarly to the other commands, but
  // I'll refactor that later.
  // TODO: failures on playback are not fatal. They shouldn't be treated fatally
  enum PlaybackState {
    StreamEmpty,
    InitStream,
    WaitingOnStream,
    StreamReady,
    Loopback,
    StreamBroken,
  };
  PlaybackState pb_state;
  shared_ptr<pa_stream> pb_stream;
  promise<bool> pb_promise;

  // For regularly checking whether the virtmic is active
  pa_source_state_t virtmic_source_state = PA_SOURCE_INVALID_STATE;
  pa_usec_t mic_active_last_check = 0;
  pa_usec_t mic_active_interval = 10000;
  pa_operation *mic_active_op = nullptr;

  pa_operation *get_default_source_op = nullptr;

  bool requested_should_denoise = false;

  mutex mainloop_mutex;
  void unload_module();
  void update_sample_spec();
  void run();
  void connect();
  void run_mainloop();
  void poll_context();
  void start_recording_stream();
  void poll_recording_stream();
  void start_pb_stream();
  void poll_pb_stream();
  void poll_operation();
  void check_module_loaded();
  void load_pipesource_module();
  void write_to_outputs();
  void check_mic_active();
  static void index_cb(pa_context *c, unsigned int idx, void *u);

  void set_microphone();
  void get_microphones();
  static void serv_info_cb(pa_context *c, const pa_server_info *i, void *u);
  static void source_info_cb(pa_context *c, const pa_source_info *i, int eol,
                             void *u);
  static void mic_active_source_info_cb(pa_context *c, const pa_source_info *i,
                                        int eol, void *u);
  static void module_info_cb(pa_context *c, const pa_module_info *i, int eol,
                             void *u);
  static void get_mics_cb();

  // utils
  static void free_pa_context(pa_context *ctx);
  static void free_pa_stream(pa_stream *s);
};
