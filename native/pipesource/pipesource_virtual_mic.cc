#include <sstream>
#include <csignal>
#include <iostream>
#include <ios>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "pipesource_virtual_mic.h"

using std::stringstream;

// reference: https://github.com/gavv/snippets/blob/master/pa/pa_play_async_poll.c
// That gives a pretty good way to structure. I'm making some changes to make it
// a little more cpp imo

PipeSourceVirtualMic::PipeSourceVirtualMic()
    : denoiser(
	  "/home/gabe/code/audo/audo-ml/denoiser/realtime-w-hidden-test.ts"),
      pipesource_module_idx(-1), module_load_operation(nullptr),
      state(InitContext), action(NoAction), action_state(NoActionState) {

  buffer = new float[buffer_length];
  write_idx = 0;
  read_idx = 0;

  should_run = true;

  mainloop = shared_ptr<pa_mainloop>(pa_mainloop_new(), pa_mainloop_free);
  connect();
  
  async_thread = thread(&PipeSourceVirtualMic::run, this);
}
void PipeSourceVirtualMic::changeState(State s) {
  std::cout << "Changin state from " << state << " to " << s << std::endl;
  state = s;
}

void PipeSourceVirtualMic::connect() {
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
  changeState(WaitContextReady);
}

void PipeSourceVirtualMic::run() {
  int err;
  bool swapped_file = false;

  while (should_run) {
    lock_guard<mutex> lock(mainloop_mutex);
    if ((err = pa_mainloop_iterate(mainloop.get(), 0, NULL)) < 0) {
      stringstream ss;
      ss << "pa_mainloop_iterate: " << pa_strerror(err);
      throw ss.str();
    }
    // TODO figure out a way to respond to state changes. maybe do this along
    // side of switching over to a state bitfield
    switch (state) {
    case InitContext:
      throw std::string("Context should have been inited already");
    case WaitContextReady:
      poll_context();
      break;
    case InitModule:
      load_pipesource_module();
      break;
    case WaitModuleReady:
      poll_operation();
      break;
    case InitRecStream:
      start_recording_stream();
      break;
    case WaitRecStreamReady:
      poll_recording_stream();
      break;
    case Denoise:
      poll_recording_stream();
      size_t spew_size;
      if ((spew_size = denoiser.willspew())) {
	size_t spewed = denoiser.spew(buffer + write_idx, buffer_length - write_idx);
	write_idx = (write_idx + spewed) % buffer_length;


	size_t to_write = write_idx - read_idx;
	if (read_idx > write_idx) {
	  to_write = buffer_length - read_idx;
	}
	ssize_t written = write(pipe_fd, buffer + read_idx, sizeof(float)*to_write);
	if (written == -1) {
	  switch (errno) {
	  case EINTR:
	  case EAGAIN:
	    break;
	  default:
	    stringstream ss;
	    ss << "pa_mainloop_iterate, write: " << strerror(errno);
	    throw ss.str();
          }
	} else {
	  written /= sizeof(float);
	  read_idx = (written + read_idx) % buffer_length;
        }
      }
      break;
    }
  }
}

void PipeSourceVirtualMic::poll_context() {
  pa_context_state_t state = pa_context_get_state(ctx.get());

  switch  (state) {
  case PA_CONTEXT_READY:
    /* context connected to server, */
    if (this->state == WaitContextReady) {
      changeState(InitModule);
    } else {
      // TODO when I get logging setup log something here because it shouldn't
      // really happen
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

void PipeSourceVirtualMic::poll_operation() {
  if (!module_load_operation) {
    return;
  }
  switch(pa_operation_get_state(module_load_operation)) {
  case PA_OPERATION_CANCELLED:
    throw std::string("module_load_operation cancelled!");
  case PA_OPERATION_DONE:
    if (-1 == pipesource_module_idx) {
      throw std::string("Failed to load module-pipesource");
    } else {
      pa_operation_unref(module_load_operation);

      changeState(InitRecStream);

      // Not sure how O_APPEND works on pipes, but shouldn't hurt
      pipe_fd = open(pipe_file_name.c_str(), O_WRONLY|O_APPEND|O_NONBLOCK);
      if (-1 == pipe_fd) {
	stringstream ss;
	ss << "Failed to open file \"" << pipe_file_name << "\" (" << strerror(errno) << ")";
        throw ss.str();
      }
    }
  default:
    break;
  }
}

void PipeSourceVirtualMic::load_pipesource_module() {
  pipe_file_name = "/tmp/virtmic";
  module_load_operation = pa_context_load_module(ctx.get(), "module-pipe-source", "source_name=virtmic file=/tmp/virtmic format=float32le rate=16000 channels=1", PipeSourceVirtualMic::index_cb, this);
  changeState(WaitModuleReady);
}

void PipeSourceVirtualMic::index_cb(pa_context *c, unsigned int idx, void *u) {
  PipeSourceVirtualMic *m = (PipeSourceVirtualMic *)u;
  m->pipesource_module_idx = idx;
}

void PipeSourceVirtualMic::start_recording_stream() {
  // TODO: make this configurable
    pa_sample_spec sample_spec = {};
    sample_spec.format = PA_SAMPLE_FLOAT32LE;
    sample_spec.rate = 16000;
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
    changeState(WaitRecStreamReady);
}
void PipeSourceVirtualMic::poll_recording_stream() {
  pa_stream_state_t state = pa_stream_get_state(rec_stream.get());

  switch  (state) {
  case PA_STREAM_READY:
    /* stream is writable, proceed now */
    if (this->state == WaitRecStreamReady) {
      changeState(Denoise);
    }
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
void PipeSourceVirtualMic::free_pa_context(pa_context *ctx) {
  pa_context_disconnect(ctx);
  pa_context_unref(ctx);
}
void PipeSourceVirtualMic::free_pa_stream(pa_stream *s) {
  pa_stream_disconnect(s);
  pa_stream_unref(s);
}
PipeSourceVirtualMic::~PipeSourceVirtualMic() {
  // TODO err is pretty ugly in here. shoudl fix that
  int err = 0;

  if (pipe_fd) {
    err = close(pipe_fd);
  }
  if (err) {
    std::cerr << "Error closing pipe_fd: " << strerror(err) << std::endl;
  }
  err = 0;
  delete buffer;
  if (-1 != pipesource_module_idx && ctx && PA_CONTEXT_READY == pa_context_get_state(ctx.get())) {
    // TODO make this use logger when I get that
    std::cerr<< std::endl << "Unloading module" << std::endl;
    pa_operation *op = pa_context_unload_module(ctx.get(), pipesource_module_idx, nullptr, nullptr);
    int i;
    for (i = 0; !err && i < 1000; i++) {
      if ((err = pa_mainloop_iterate(mainloop.get(), 0, NULL)) < 0) {
	std::cerr << "Error: " << pa_strerror(err) << std::endl;
	break;
      }
      err = 0;
      switch (pa_operation_get_state(op)) {
      case PA_OPERATION_RUNNING:
	continue;
      case PA_OPERATION_DONE:
	i = 1000;
	break;
      case PA_OPERATION_CANCELLED:
	err = 1;
	std::cerr << "Unload operation cancelled" << std::endl;
	break;
      }
    }
    if (err) {
      std::cerr << err << std::endl;
      std::cerr << "Error unloading module" << std::endl;
    }
  }
}
void PipeSourceVirtualMic::stop() {
  //   lock_guard<mutex> lock(mainloop_mutex);
  should_run = false;
  async_thread.join();
}
void PipeSourceVirtualMic::getStatus(promise<bool> promise) {
  lock_guard<mutex> lock(mainloop_mutex);
  promise.set_value(state >= InitRecStream);
}

void PipeSourceVirtualMic::getMicrophones(promise<vector<pair<int, string>>> p) {
  lock_guard<mutex> lock(mainloop_mutex);
  if (state < InitRecStream) {
    throw std::runtime_error("Not ready to get Microphones");
  }
  // TODO: we don't support multiple actions at a time yet
  if (action != NoAction) {
    throw std::runtime_error("Can only handle single action at a time");
  }
  action = GetMicrophones;
  get_mics_promise = std::move(p);
}
void PipeSourceVirtualMic::setMicrophone(promise<void> p, int ind) {
  return;
}
void PipeSourceVirtualMic::setRemoveNoise(promise<void> p, bool b) {
  return;
}
