#include <sstream>
#include <csignal>
#include <iostream>
#include <ios>
#include <cassert>

#include <fcntl.h>
#include <sys/stat.h>

#include "app.h"

App *App::global_app = nullptr;
using std::stringstream;

// reference: https://github.com/gavv/snippets/blob/master/pa/pa_play_async_poll.c
// That gives a pretty good way to structure. I'm making some changes to make it
// a little more cpp imo

App::App(std::string pf) : denoiser("/home/gabe/code/audo/audo-ml/denoiser/realtime-w-hidden-test.ts"), pipe_file_name(pf) {
  if (App::global_app) {
    throw std::string( "Only a single instance allowed!");
  }
  struct stat s;
  int err = stat(pipe_file_name.c_str(), &s);
  if (err) {
    throw std::string( "Can't access file");
  }
  pipe = ofstream(pipe_file_name, std::ios_base::out|std::ios_base::binary);
  if (!pipe.is_open()) {
    stringstream ss;
    ss << "Failed to open file \""<< pipe_file_name << "\"";
    throw ss.str();
  }
  App::global_app = this;
  signal(SIGINT, App::signal_handler);
  mainloop = shared_ptr<pa_mainloop>(pa_mainloop_new(), pa_mainloop_free);
  connect();

  should_run = true;
}
void App::connect() {
  ctx = shared_ptr<pa_context>(pa_context_new(pa_mainloop_get_api(mainloop.get()), client_name), free_pa_context);
  if (!ctx) {
    throw std::string( "pa_context_new failed");
  }

  int err = pa_context_connect(ctx.get(), nullptr, (pa_context_flags)0, nullptr);
  if (err) {
    stringstream ss;
    ss << "pa_context_connect failed: " << pa_strerror(err);
    throw ss.str();
  }
}

void App::run() {
  int err;

  while (should_run) {
    if ((err = pa_mainloop_iterate(mainloop.get(), 1, NULL)) < 0) {
      stringstream ss;
      ss << "pa_mainloop_iterate: " << pa_strerror(err);
      throw ss.str();
    }
    poll_context();
    if (rec_stream) {
      poll_recording_stream();
    }
    size_t spew_size;
    if ((spew_size = denoiser.willspew())) {
      float *arr = new float[spew_size];
      assert(spew_size == denoiser.spew(arr, spew_size));

      // std::cout<<"read " << spew_size << " bytes from denoiser" << std::endl;
      pipe.write((char *)arr, spew_size*4);

      delete[] arr;
    }
  }
}


void App::poll_context() {
  pa_context_state_t state = pa_context_get_state(ctx.get());

  switch  (state) {
  case PA_CONTEXT_READY:
    /* context connected to server, */
    if (!rec_stream) {
      start_recording_stream();
    }
    break;

  case PA_CONTEXT_FAILED:
  case PA_CONTEXT_TERMINATED:
    /* context connection failed */
    throw std::string( "failed to connect to pulseaudio context!");
  default:
    /* nothing interesting */
    break;
  }
}

void App::start_recording_stream() {
  // TODO: make this configurable
    pa_sample_spec sample_spec = {};
    sample_spec.format = PA_SAMPLE_FLOAT32LE;
    sample_spec.rate = 16384;
    sample_spec.channels = 1;

    rec_stream = shared_ptr<pa_stream>(pa_stream_new(ctx.get(), rec_stream_name, &sample_spec, nullptr), free_pa_stream);
    if (!rec_stream) {
      stringstream ss;
      ss << "pa_stream_new failed: " << pa_strerror(pa_context_errno(ctx.get()));
      throw ss.str();
    }

    int err = pa_stream_connect_record(rec_stream.get(), source, nullptr, (pa_stream_flags)0);
    if (err != 0) {
      stringstream ss;
      ss << "pa_stream_connect_record: " << pa_strerror(err);
      throw ss.str();
    }
}
void App::poll_recording_stream() {
  pa_stream_state_t state = pa_stream_get_state(rec_stream.get());

  switch  (state) {
  case PA_STREAM_READY:
    /* stream is writable, proceed now */
    if (pa_stream_readable_size(rec_stream.get()) > 0) {
      const void *data;
      size_t nbytes;
      int err = pa_stream_peek(rec_stream.get(), &data, &nbytes);
      if (err) {
	stringstream ss;
	ss << "pa_stream_peak: " << pa_strerror(err);
	throw ss.str();
      }
      denoiser.feed((float *)data, nbytes/4);
      pa_stream_drop(rec_stream.get());
    }
    break;

  case PA_STREAM_FAILED:
  case PA_STREAM_TERMINATED:
    /* stream is closed, exit */
    throw std::string("recording_stream closed unexpectedly");
  default:
    /* stream is not ready yet */
    return;
  }
}
void App::free_pa_context(pa_context *ctx) {
  pa_context_disconnect(ctx);
  pa_context_unref(ctx);
}
void App::free_pa_stream(pa_stream *s) {
  pa_stream_disconnect(s);
  pa_stream_unref(s);
}

void App::signal_handler(int signal) {
  switch (signal) {
  case SIGTERM:
  case SIGINT:
    std::cout << "Recieved signal, exiting";
    App::global_app->should_run = false;
    break;
  }
}
