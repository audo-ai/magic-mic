#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <cstdio>

#include <pulse/pulseaudio.h>

#include <denoiser.h>

using std::shared_ptr;
using std::string;
using std::ofstream;
class App;
// Only supports a single instance
class App {
 public:
  App();
  ~App();

  void run();
 private:
   enum State {
     InitContext = 1,
     WaitContextReady,
     InitModule,
     WaitModuleReady,
     InitRecStream,
     WaitRecStreamReady,
     Denoise
   };
   // TODO for now I guess a single state thing is ok, but I think it would make more sense to have some sort of state bitfield. It would allow parallel initialization and would overall be more elegant. It would be more resillient and more accuratly reflect the fact that whats relevant isn't really a line of execution but the statse of each resource and operation.
   State state;
   void changeState(State s);

   const char *rec_stream_name = "Recording Stream";
   const char *source = nullptr;
   const char *client_name = "Client Name";
   const int target_latency = 0;
   const size_t buffer_length = 32000;

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
   volatile bool should_run;

   int pipesource_module_idx;
   pa_operation *module_load_operation;

   Denoiser denoiser;

   void connect();
   void run_mainloop();
   void poll_context();
   void start_recording_stream();
   void poll_recording_stream();
   void poll_operation();
   void load_pipesource_module();
   static void index_cb(pa_context *c, unsigned int idx, void *u);

   // Handle ctrl-c
   static App *global_app;
   static void signal_handler(int signal);

   // utils
   static void free_pa_context(pa_context *ctx);
   static void free_pa_stream(pa_stream *s);
};
