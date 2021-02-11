#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include <pulse/pulseaudio.h>

#include <denoiser.h>

using std::shared_ptr;
using std::string;
using std::ofstream;
class App;
// Only supports a single instance
class App {
 public:
  App(string pf);

  void run();
 private:
  const char *rec_stream_name = "Recording Stream";
  const char *source = nullptr;
  const char *client_name = "Client Name";
  const int target_latency = 0;

  string pipe_file_name;
  ofstream pipe;

  


  // TODO: I don't think we should allow mainloop to be freed before ctx and
  // rec_stream but, I can't figure out a good way to fix that
  shared_ptr<pa_mainloop> mainloop;
  shared_ptr<pa_context> ctx;
  shared_ptr<pa_stream> rec_stream;
  volatile bool should_run;

  Denoiser denoiser;

  void connect();
  void run_mainloop();
  void poll_context();
  void start_recording_stream();
  void poll_recording_stream();

  // Handle ctrl-c
  static App *global_app;
  static void signal_handler(int signal);

  // utils
  static void free_pa_context(pa_context *ctx);
  static void free_pa_stream(pa_stream *s);
};
