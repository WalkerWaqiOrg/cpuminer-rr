// C includes
//#include <ctime>
//#include <csignal>
//#include <cstdlib>

#include <curl/curl.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>

// C++ includes
//#include <string>
//#include <vector>
//#include <random>
//#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

// CAF
#include "caf/all.hpp"
#include "caf/io/all.hpp"

#include "stratum_actor.h"
#include "user_params.h"
#include "miner_actors.h"
#include "rrhash.h"

using namespace caf;

void caf_main(actor_system& system, const UserParams& user_params) {
  curl_global_init(CURL_GLOBAL_DEFAULT);

  scoped_actor self{system};
  auto master_actor = self->spawn<detached>(miner_master, user_params);
  actor stratum_actor = self->spawn<detached>(stratum_handler, user_params, master_actor);
  self->send(master_actor, stratum_actor_atom::value, stratum_actor);

  self->await_all_other_actors_done();

  curl_global_cleanup();
}

int main(int argc, char** argv) {
  rrhash_init();

  UserParams* user_params = new UserParams();
  user_params->parse_cmdline(argc, argv);
  actor_system_config cfg;
  cfg.scheduler_max_threads = user_params->actors;
  actor_system system{cfg};
  caf_main(system, *user_params);

  delete user_params;
  rrhash_release();

  return 0;
}
