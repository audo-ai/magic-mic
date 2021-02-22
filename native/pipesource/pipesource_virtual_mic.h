#pragma once

#include <atomic>
#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <optional>


#include <pulse/pulseaudio.h>

#include <denoiser.h>
#include <virtual_mic.h>

using std::atomic;
using std::future;
using std::lock_guard;
using std::mutex;
using std::pair;
using std::promise;
using std::shared_ptr;
using std::string;
using std::thread;
using std::vector;
class PipeSourceVirtualMic;
// Only supports a single instance
class PipeSourceVirtualMic : public VirtualMic {
public:
  // TODO Need to implement copy constructors
  PipeSourceVirtualMic();
  // threads aren't copyable, so neiter is this
  PipeSourceVirtualMic(PipeSourceVirtualMic &) = delete;
  ~PipeSourceVirtualMic();

  void stop() override;
  void abortLastRequest() override;
  future<bool> getStatus() override;
  future<vector<pair<int, string>>> getMicrophones() override;
  future<void> setMicrophone(int) override;
  future<void> setRemoveNoise(bool) override;

private:
  thread async_thread;
  struct GetMicrophones {
    enum GetMicrophonesActionState { Init, Wait, Done };
    enum GetMicrophonesActionState state;
    pa_operation *op;
    vector<pair<int, string>> list;
    promise<vector<pair<int, string>>> p;
  };
  struct SetMicrophone {
    enum SetMicrophoneActionState { InitGettingSource, WaitGettingSource, UpdatingStream };
    enum SetMicrophoneActionState state;
    pa_operation *op;
    int ind;
    string name;
    promise<void> p;
  };
  struct CurrentAction {
    enum { NoAction, GetMicrophones, SetMicrophone } action;
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
    InitModule,
    WaitModuleReady,
    InitRecStream,
    WaitRecStreamReady,
    Denoise
  };
  CurrentAction cur_act;
  State state;
  void changeState(State s);

  // TODO for now I guess a single state thing is ok, but I think it would make
  // more sense to have some sort of state bitfield. It would allow parallel
  // initialization and would overall be more elegant. It would be more
  // resillient and more accuratly reflect the fact that whats relevant isn't
  // really a line of execution but the statse of each resource and operation.
  const char *rec_stream_name = "Recording Stream";
  const char *source = nullptr;
  const char *client_name = "Client Name";
  const int target_latency = 0;
  const size_t buffer_length = 2048;

  string pipe_file_name;
  // It appears c++ doesn't do non blocking writes which is a problem for
  // signal handling, so we need to go old school (sort of, still have
  // shared_ptr which is sort of cool I guess)
  int pipe_fd;

  // TODO: factor this out into a tiny ring buffer
  float *buffer;
  size_t write_idx;
  size_t read_idx;

  // TODO: I don't think we should allow mainloop to be freed before ctx
  // and rec_stream but, I can't figure out a good way to fix that
  shared_ptr<pa_mainloop> mainloop;
  shared_ptr<pa_context> ctx;
  shared_ptr<pa_stream> rec_stream;
  atomic<bool> should_run;

  int pipesource_module_idx;
  pa_operation *module_load_operation;

  Denoiser denoiser;

  mutex mainloop_mutex;
  void run();
  void connect();
  void run_mainloop();
  void poll_context();
  void start_recording_stream();
  void poll_recording_stream();
  void poll_operation();
  void load_pipesource_module();
  void write_to_pipe();
  static void index_cb(pa_context *c, unsigned int idx, void *u);

  void set_microphone();
  void get_microphones();
  static void source_info_cb(pa_context *c, const pa_source_info *i, int eol, void *u);
  static void get_mics_cb();

  // utils
  static void free_pa_context(pa_context *ctx);
  static void free_pa_stream(pa_stream *s);
};
