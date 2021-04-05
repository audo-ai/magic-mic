#include "module_class.hpp"

void Module::thread_func(void *userdata) {
  Module *u = (Module *)userdata;

  pa_log("Entering Magic %s thread", u->source_name);
  while (pa_atomic_load(&u->run_thread)) {
    pa_memchunk chunk;
    // if (pa_memblockq_get_length(u->memblockq_in.get())) {
    //   pa_log_info("in length: %d",
    //   pa_memblockq_get_length(u->memblockq_in.get()));
    // }
    while (0 == pa_memblockq_peek(u->memblockq_in.get(), &chunk)) {
      // Might have a problem if this becomes zero. Not
      auto len_to_take = u->denoiser.get_size_multiple() *
                         (chunk.length / u->denoiser.get_size_multiple());
      if (chunk.memblock) {
        pa_memblock_acquire(chunk.memblock);
        u->denoiser.feed((float *)chunk.memblock, len_to_take);
        pa_memblock_release(chunk.memblock);
        // pa_memblockq_push(u->memblockq_out.get(), &chunk);
      }
      pa_memblockq_drop(u->memblockq_in.get(), len_to_take);
      if (chunk.memblock)
        pa_memblock_unref(chunk.memblock);
    }
    std::size_t size;
    if (0 != (size = u->denoiser.willspew())) {
      pa_memchunk chunk;
      pa_memblock *block =
          pa_memblock_new(u->module->core->mempool, size * sizeof(float));
      // TODO this might fail, and if so its not actually a problem, we just
      // need to split up allocation to smaller blocks
      assert(block);

      u->denoiser.spew((float *)pa_memblock_acquire(block), size);

      chunk.length = size * sizeof(float);
      chunk.memblock = block;
      pa_memblockq_push(u->memblockq_out.get(), &chunk);
      pa_memblock_release(block);
      pa_memblock_unref(block);
    }
  }
}
int Module::source_process_msg_cb(pa_msgobject *o, int code, void *data,
                                  int64_t offset, pa_memchunk *chunk) {
  Module *u = (Module *)PA_SOURCE(o)->userdata;

  switch (code) {

  case PA_SOURCE_MESSAGE_GET_LATENCY:

    /* The source is _put() before the source output is, so let's
     * make sure we don't access it in that time. Also, the
     * source output is first shut down, the source second. */
    if (!PA_SOURCE_IS_LINKED(u->source->thread_info.state) ||
        !PA_SOURCE_OUTPUT_IS_LINKED(u->source_output->thread_info.state)) {
      *((pa_usec_t *)data) = 0;
      return 0;
    }

    *((pa_usec_t *)data) =

        /* Get the latency of the master source */
        pa_source_get_latency_within_thread(u->source_output->source, true) +

        /* Add the latency internal to our source output on top */
        /* FIXME, no idea what I am doing here */
        pa_bytes_to_usec(pa_memblockq_get_length(
                             u->source_output->thread_info.delay_memblockq),
                         &u->source_output->source->sample_spec);

    return 0;
  }

  return pa_source_process_msg(o, code, data, offset, chunk);
}

int Module::source_set_state_in_main_thread_cb(
    pa_source *s, pa_source_state_t state, pa_suspend_cause_t suspend_cause) {
  Module *u;

  pa_source_assert_ref(s);
  pa_assert_se(u = (Module *)s->userdata);

  if (!PA_SOURCE_IS_LINKED(state) ||
      !PA_SOURCE_OUTPUT_IS_LINKED(u->source_output->state))
    return 0;

  pa_source_output_cork(u->source_output, state == PA_SOURCE_SUSPENDED);
  return 0;
}
void Module::source_update_requested_latency_cb(pa_source *s) {
  Module *u;

  pa_source_assert_ref(s);
  pa_assert_se(u = (Module *)s->userdata);

  if (!PA_SOURCE_IS_LINKED(u->source->thread_info.state) ||
      !PA_SOURCE_OUTPUT_IS_LINKED(u->source_output->thread_info.state))
    return;

  /* Just hand this one over to the master source */
  pa_source_output_set_requested_latency_within_thread(
      u->source_output, pa_source_get_requested_latency_within_thread(s));
}
void Module::source_set_volume_cb(pa_source *s) {
  Module *u;

  pa_source_assert_ref(s);
  pa_assert_se(u = (Module *)s->userdata);

  if (!PA_SOURCE_IS_LINKED(s->state) ||
      !PA_SOURCE_OUTPUT_IS_LINKED(u->source_output->state))
    return;

  pa_source_output_set_volume(u->source_output, &s->real_volume, s->save_volume,
                              true);
}
void Module::source_set_mute_cb(pa_source *s) {
  Module *u;

  pa_source_assert_ref(s);
  pa_assert_se(u = (Module *)s->userdata);

  if (!PA_SOURCE_IS_LINKED(s->state) ||
      !PA_SOURCE_OUTPUT_IS_LINKED(u->source_output->state))
    return;

  pa_source_output_set_mute(u->source_output, s->muted, s->save_muted);
}
void Module::source_output_push_cb(pa_source_output *o,
                                   const pa_memchunk *chunk) {
  Module *u;

  pa_source_output_assert_ref(o);
  pa_source_output_assert_io_context(o);
  pa_assert_se(u = (Module *)o->userdata);

  if (!PA_SOURCE_IS_LINKED(u->source->thread_info.state))
    return;

  if (!PA_SOURCE_OUTPUT_IS_LINKED(u->source_output->thread_info.state)) {
    pa_log("push when no link?");
    return;
  }

  /* PUT YOUR CODE HERE TO DO SOMETHING WITH THE SOURCE DATA */
  /* forward the data to the virtual source */
  // pa_log("Chunk length: %zu", chunk->length);
  // pa_source_post(u->source, chunk);
  pa_memblock_acquire_chunk(chunk);
  pa_memblockq_push(u->memblockq_in.get(), chunk);
  pa_memblock_release(chunk->memblock);
  pa_memchunk qchunk;
  // pa_log_info("qchunk: %p", qchunk.memblock);
  int posted = 0;
  while (pa_memblockq_peek(u->memblockq_out.get(), &qchunk) == 0) {
    posted += qchunk.length;
    // sometimes peek returns just length, for a hole I believe
    if (qchunk.memblock)
      pa_source_post(u->source, &qchunk);
    pa_memblockq_drop(u->memblockq_out.get(), qchunk.length);
    if (qchunk.memblock)
      pa_memblock_unref(qchunk.memblock);
  }
  if (posted < chunk->length) {
    pa_log_info("Not posted enough: %d out of %d", posted, chunk->length);
  }
}
void Module::source_output_process_rewind_cb(pa_source_output *o,
                                             size_t nbytes) {
  Module *u;
  pa_log("Rewind");
  pa_source_output_assert_ref(o);
  pa_source_output_assert_io_context(o);
  pa_assert_se(u = (Module *)o->userdata);

  /* If the source is not yet linked, there is nothing to rewind */
  if (PA_SOURCE_IS_LINKED(u->source->thread_info.state))
    pa_source_process_rewind(u->source, nbytes);

    /* FIXME, no idea what I am doing here */
#if 0
    pa_asyncmsgq_post(u->asyncmsgq, PA_MSGOBJECT(u->sink_input), SINK_INPUT_MESSAGE_REWIND, NULL, (int64_t) nbytes, NULL, NULL);
    u->send_counter -= (int64_t) nbytes;
#endif
}
void Module::source_output_update_max_rewind_cb(pa_source_output *o,
                                                size_t nbytes) {
  pa_log("update Rwind");
  Module *u;

  pa_source_output_assert_ref(o);
  pa_source_output_assert_io_context(o);
  pa_assert_se(u = (Module *)o->userdata);

  pa_source_set_max_rewind_within_thread(u->source, nbytes);
}
void Module::source_output_attach_cb(pa_source_output *o) {
  Module *u;
  pa_log("Attach %p", o->source->thread_info.rtpoll);

  pa_source_output_assert_ref(o);
  pa_source_output_assert_io_context(o);
  pa_assert_se(u = (Module *)o->userdata);

  pa_source_set_rtpoll(u->source, o->source->thread_info.rtpoll);
  pa_source_set_latency_range_within_thread(u->source,
                                            o->source->thread_info.min_latency,
                                            o->source->thread_info.max_latency);
  pa_source_set_fixed_latency_within_thread(
      u->source, o->source->thread_info.fixed_latency);
  pa_source_set_max_rewind_within_thread(u->source,
                                         pa_source_output_get_max_rewind(o));

  if (PA_SOURCE_IS_LINKED(u->source->thread_info.state))
    pa_source_attach_within_thread(u->source);
}
void Module::source_output_detach_cb(pa_source_output *o) {
  Module *u;

  pa_source_output_assert_ref(o);
  pa_source_output_assert_io_context(o);
  pa_assert_se(u = (Module *)o->userdata);

  if (PA_SOURCE_IS_LINKED(u->source->thread_info.state))
    pa_source_detach_within_thread(u->source);
  pa_source_set_rtpoll(u->source, NULL);
}
void Module::source_output_state_change_cb(pa_source_output *o,
                                           pa_source_output_state_t state) {
  Module *u;

  pa_source_output_assert_ref(o);
  pa_assert_se(u = (Module *)o->userdata);

  /* FIXME */
#if 0
    if (PA_SOURCE_OUTPUT_IS_LINKED(state) && o->thread_info.state == PA_SOURCE_OUTPUT_INIT && o->source) {

	u->skip = pa_usec_to_bytes(PA_CLIP_SUB(pa_source_get_latency_within_thread(o->source, false),
					       u->latency),
				   &o->sample_spec);

	pa_log_info("Skipping %lu bytes", (unsigned long) u->skip);
    }
#endif
}
void Module::source_output_kill_cb(pa_source_output *o) {
  Module *u;

  pa_source_output_assert_ref(o);
  pa_assert_ctl_context();
  pa_assert_se(u = (Module *)o->userdata);

  /* The order here matters! We first kill the source so that streams
   * can properly be moved away while the source output is still connected
   * to the master. */
  pa_source_output_cork(u->source_output, true);
  pa_source_unlink(u->source);
  pa_source_output_unlink(u->source_output);

  pa_source_output_unref(u->source_output);
  u->source_output = NULL;

  pa_source_unref(u->source);
  u->source = NULL;

  pa_module_unload_request(u->module, true);
}
void Module::source_output_moving_cb(pa_source_output *o, pa_source *dest) {
  Module *u;
  uint32_t idx;
  pa_source_output *output;

  pa_source_output_assert_ref(o);
  pa_assert_ctl_context();
  pa_assert_se(u = (Module *)o->userdata);

  if (dest) {
    pa_source_set_asyncmsgq(u->source, dest->asyncmsgq);
    pa_source_update_flags(
        u->source,
        (pa_source_flags)(PA_SOURCE_LATENCY | PA_SOURCE_DYNAMIC_LATENCY),
        dest->flags);
  } else
    pa_source_set_asyncmsgq(u->source, NULL);

  /* Propagate asyncmsq change to attached virtual sources */
  PA_IDXSET_FOREACH(output, u->source->outputs, idx) {
    if (output->destination_source && output->moving)
      output->moving(output, u->source);
  }

  if (u->auto_desc && dest) {
    const char *z;
    pa_proplist *pl;

    pl = pa_proplist_new();
    z = pa_proplist_gets(dest->proplist, PA_PROP_DEVICE_DESCRIPTION);
    pa_proplist_setf(
        pl, PA_PROP_DEVICE_DESCRIPTION, "Virtual Source %s on %s",
        pa_proplist_gets(u->source->proplist, "device.vsource.name"),
        z ? z : dest->name);

    pa_source_update_proplist(u->source, PA_UPDATE_REPLACE, pl);
    pa_proplist_free(pl);
  }
}
void Module::store_modargs(std::shared_ptr<pa_modargs> ma) {
  if (!(master = (pa_source *)pa_namereg_get(
            module->core, pa_modargs_get_value(ma.get(), "master", NULL),
            PA_NAMEREG_SOURCE))) {
    throw "Master source not found";
  }
  // seems unnecessary to me, but can't hurt I suppose
  pa_assert(master);

  ss = master->sample_spec;
  ss.format = PA_SAMPLE_FLOAT32;
  map = master->channel_map;
  if (pa_modargs_get_sample_spec_and_channel_map(ma.get(), &ss, &map,
                                                 PA_CHANNEL_MAP_DEFAULT) < 0) {
    throw "Invalid sample format specification or channel map";
  }

  if (pa_modargs_get_value_boolean(ma.get(), "use_volume_sharing",
                                   &use_volume_sharing) < 0) {
    throw "use_volume_sharing= expects a boolean argument";
  }
  if (pa_modargs_get_value_boolean(ma.get(), "force_flat_volume",
                                   &force_flat_volume) < 0) {
    throw "force_flat_volume= expects a boolean argument";
  }

  if (use_volume_sharing && force_flat_volume) {
    throw "Flat volume can't be forced when using volume sharing.";
  }
}
// modargs are just to get proplist
void Module::create_source(std::shared_ptr<pa_modargs> ma) {
  /* Create source */
  pa_source_new_data source_data;
  pa_source_new_data_init(&source_data);
  source_data.driver = __FILE__;
  source_data.module = module;
  char *source_name_copy = new char[sizeof(source_name)];
  std::copy(source_name, source_name + sizeof(source_name), source_name_copy);
  source_data.name = source_name_copy;
  pa_source_new_data_set_sample_spec(&source_data, &ss);
  pa_source_new_data_set_channel_map(&source_data, &map);
  pa_proplist_sets(source_data.proplist, PA_PROP_DEVICE_MASTER_DEVICE,
                   master->name);
  pa_proplist_sets(source_data.proplist, PA_PROP_DEVICE_CLASS, "filter");
  pa_proplist_sets(source_data.proplist, "device.vsource.name",
                   source_data.name);

  if (pa_modargs_get_proplist(ma.get(), "source_properties",
                              source_data.proplist, PA_UPDATE_REPLACE) < 0) {
    pa_source_new_data_done(&source_data);
    throw "Invalid properties";
  }

  if ((auto_desc = !pa_proplist_contains(source_data.proplist,
                                         PA_PROP_DEVICE_DESCRIPTION))) {
    const char *z;

    z = pa_proplist_gets(master->proplist, PA_PROP_DEVICE_DESCRIPTION);
    pa_proplist_setf(source_data.proplist, PA_PROP_DEVICE_DESCRIPTION,
                     "Virtual Source %s on %s", source_data.name,
                     z ? z : master->name);
  }

  source = pa_source_new(
      module->core, &source_data,
      (pa_source_flags_t)(master->flags &
                          (PA_SOURCE_LATENCY | PA_SOURCE_DYNAMIC_LATENCY)) |
          (use_volume_sharing ? PA_SOURCE_SHARE_VOLUME_WITH_MASTER : 0));

  pa_source_new_data_done(&source_data);

  if (!source) {
    throw "Failed to create source.";
  }

  source->userdata = this;

  // highly unser about this. ostensible purpose is to stop loopback from
  // crashing on unload
  source->thread_info.rtpoll = master->thread_info.rtpoll;

  source->parent.process_msg = Module::source_process_msg_cb;
  source->set_state_in_main_thread = Module::source_set_state_in_main_thread_cb;
  source->update_requested_latency = Module::source_update_requested_latency_cb;
  pa_source_set_set_mute_callback(source, Module::source_set_mute_cb);
  if (!use_volume_sharing) {
    pa_source_set_set_volume_callback(source, Module::source_set_volume_cb);
    pa_source_enable_decibel_volume(source, true);
  }
  /* Normally this flag would be enabled automatically be we can force it. */
  if (force_flat_volume)
    source->flags |= PA_SOURCE_FLAT_VOLUME;

  pa_source_set_asyncmsgq(source, master->asyncmsgq);
}
void Module::create_source_output() {
  /* Create source output */
  pa_source_output_new_data source_output_data;
  pa_source_output_new_data_init(&source_output_data);
  source_output_data.driver = __FILE__;
  source_output_data.module = module;
  pa_source_output_new_data_set_source(&source_output_data, master, false,
                                       true);
  source_output_data.destination_source = source;

  pa_proplist_setf(
      source_output_data.proplist, PA_PROP_MEDIA_NAME,
      "Virtual Source Stream of %s",
      pa_proplist_gets(source->proplist, PA_PROP_DEVICE_DESCRIPTION));
  pa_proplist_sets(source_output_data.proplist, PA_PROP_MEDIA_ROLE, "filter");
  pa_source_output_new_data_set_sample_spec(&source_output_data, &ss);
  pa_source_output_new_data_set_channel_map(&source_output_data, &map);
  source_output_data.flags |= PA_SOURCE_OUTPUT_START_CORKED;

  pa_source_output_new(&source_output, module->core, &source_output_data);
  pa_source_output_new_data_done(&source_output_data);

  if (!source_output)
    throw "source_output creation failed";

  source_output->push = Module::source_output_push_cb;
  source_output->process_rewind = Module::source_output_process_rewind_cb;
  source_output->update_max_rewind = Module::source_output_update_max_rewind_cb;
  source_output->kill = Module::source_output_kill_cb;
  source_output->attach = Module::source_output_attach_cb;
  source_output->detach = Module::source_output_detach_cb;
  source_output->state_change = Module::source_output_state_change_cb;
  source_output->moving = Module::source_output_moving_cb;
  source_output->userdata = this;

  // This is what makes it a filter source
  source->output_from_master = source_output;
}
Module::Module(pa_module *m) : denoiser() {
  pa_log_debug("loading magic");

  pa_assert(m);

  module = m;

  // obviously, parse arguments
  std::shared_ptr<pa_modargs> ma(
      pa_modargs_new(module->argument, valid_modargs), pa_modargs_free);
  if (!ma) {
    throw "Failed to parse module arguments.";
  }
  store_modargs(ma);
  create_source(ma);

  module->userdata = this;
  channels = ss.channels;

  create_source_output();

  char q_in_name[sizeof(source_name) + 64];
  std::sprintf(q_in_name, "memblockq in: %s", source_name);
  memblockq_in = std::shared_ptr<pa_memblockq>(
      pa_memblockq_new(q_in_name, 0, max_q_len, 0, &ss, prebuf, 0, maxrewind,
                       nullptr),
      pa_memblockq_free);
  if (!memblockq_in) {
    throw "Failed to create source memblockq_in";
  }
  char q_out_name[sizeof(source_name) + 64];
  // pa_memblock_ref(source_output->source->silence.memblock);
  std::sprintf(q_in_name, "memblockq out: %s", source_name);
  memblockq_out = std::shared_ptr<pa_memblockq>(
      pa_memblockq_new(q_out_name, 0, max_q_len, 0, &ss, prebuf, 0, maxrewind,
                       nullptr),
      pa_memblockq_free);
  if (!memblockq_out) {
    throw "Failed to create source memblockq_out";
  }
  rtpoll = std::shared_ptr<pa_rtpoll>(pa_rtpoll_new(), pa_rtpoll_free);
  if (!rtpoll) {
    throw "Failed to create rtpoll (?!)";
  }

  /* The order here is important. The output must be put first,
   * otherwise streams might attach to the source before the
   * source output is attached to the master. */
  pa_source_output_put(source_output);
  pa_source_put(source);
  pa_source_output_cork(source_output, false);

  pa_atomic_store(&run_thread, 1);
  thread = std::shared_ptr<pa_thread>(
      pa_thread_new("magic mic thread", Module::thread_func, this),
      [this](pa_thread *thread) {
        pa_log("stopping thread");
        pa_atomic_store(&run_thread, 0);
        pa_thread_free(thread);
      });
  if (!thread) {
    throw "Failed to initialize thread";
  }
}
Module::~Module() {
  pa_log_debug("unloading magic");

  /* See comments in source_output_kill_cb() above regarding
   * destruction order! */
  if (source_output)
    pa_source_output_cork(source_output, true);

  if (source)
    pa_source_unlink(source);

  if (source_output) {
    pa_source_output_unlink(source_output);
    pa_source_output_unref(source_output);
  }

  if (source)
    pa_source_unref(source);
}
