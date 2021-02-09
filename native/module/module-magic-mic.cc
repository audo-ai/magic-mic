/***
    This file is part of PulseAudio.

    Copyright 2010 Intel Corporation
    Contributor: Pierre-Louis Bossart <pierre-louis.bossart@intel.com>

    PulseAudio is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    PulseAudio is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with PulseAudio; if not, see <http://www.gnu.org/licenses/>.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include "module_class.h"

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


  PA_MODULE_AUTHOR("Audo AI");
  PA_MODULE_DESCRIPTION("Magic Mic");
  PA_MODULE_VERSION(PACKAGE_VERSION);// oh shit
  PA_MODULE_LOAD_ONCE(false);
  PA_MODULE_USAGE(_("source_name=<name for the source> "
		    "source_properties=<properties for the source> "
		    "master=<name of source to filter> "
		    "uplink_sink=<name> (optional)"
		    "format=<sample format> "
		    "rate=<sample rate> "
		    "channels=<number of channels> "
		    "channel_map=<channel map> "
		    "use_volume_sharing=<yes or no> "
		    "force_flat_volume=<yes or no> "));

  int pa__init(pa_module *m) {
    try {
      void *module_data = pa_xnew0(Module, 1);
      Module *u = new (module_data) Module(m);
    } catch (const std::string &e) {
      pa_log(e.c_str());
      //pa__done(m);
      return -1;
    }
    return 0;
  }

  int pa__get_n_used(pa_module *m) {
    Module *u;

    pa_assert(m);
    pa_assert_se(u = (Module*)m->userdata);

    return pa_source_linked_by(u->source);
  }

  void pa__done(pa_module *m) {
    Module *u;

    pa_assert(m);

    if (!(u = (Module*)m->userdata))
      return;

    u->~Module();
  
    pa_xfree(u);
  }

}
