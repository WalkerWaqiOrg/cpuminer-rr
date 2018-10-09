#include <curl/curl.h>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

// CAF
#include "caf/all.hpp"
#include "caf/io/all.hpp"

#include "cpuminer_lib.h"
#include "stratum_actor.h"
#include "user_params.h"
#include "miner_actors.h"
#include "rrhash.h"
#include "util.h"

using namespace caf;

actor* g_master_actor = NULL;
actor* g_stratum_actor = NULL;

int get_num_processors() {
  return util::get_num_processors();
}

int get_state() {
  if (!g_stratum_actor) {
    return STATE_DISCONNECTED;
  }
  actor_system_config cfg;
  actor_system system{cfg};
  scoped_actor self{system};
  Global_state_info ret;
  self->request(*g_stratum_actor, std::chrono::seconds(5), get_info_atom::value).receive (
    [&](const Global_state_info &global_state_info){
      ret.state = global_state_info.state;
    },
    [&](error& err) {
      ret.state = -1;
    }
  );
  return ret.state;
}

char *get_token() {
  if (!g_stratum_actor) {
    return STATE_DISCONNECTED;
  }
  actor_system_config cfg;
  actor_system system{cfg};
  scoped_actor self{system};
  Global_state_info ret;
  self->request(*g_stratum_actor, std::chrono::seconds(5), get_info_atom::value).receive (
    [&](const Global_state_info &global_state_info){
      for (int i = 0; i < TOKEN_MAX_LEN; ++i) {
        ret.token[i] = global_state_info.token[i];
      }
    },
    [&](error& err) {
      memset(ret.token, 0, TOKEN_MAX_LEN);
    }
  );
  return ret.token;
}

// todo: get_rate

void caf_main(actor_system& system, const UserParams& user_params) {
  curl_global_init(CURL_GLOBAL_DEFAULT);

  scoped_actor self{system};
  g_master_actor = new actor(self->spawn<detached>(miner_master, user_params));
  g_stratum_actor = new actor(self->spawn<detached>(stratum_handler, user_params, *g_master_actor));
  self->send(*g_master_actor, stratum_actor_atom::value, *g_stratum_actor);
}

int start_miner_thread_func(int argc, char** argv) {
  rrhash_init();

  UserParams* user_params = new UserParams();
  user_params->parse_cmdline(argc, argv);
  actor_system_config cfg;
  cfg.scheduler_max_threads = user_params->actors;
  actor_system system{cfg};
  caf_main(system, *user_params);

  scoped_actor self{system};
  self->await_all_other_actors_done();

  delete user_params;
  curl_global_cleanup();
  rrhash_release();

  return 0;
}

int start_miner(int argc, char** argv) {
  if (!g_master_actor) {
      boost::thread start_thread(boost::bind(start_miner_thread_func, argc, argv));
  }
}

void stop_miner() {
  actor_system_config cfg;
  actor_system system{cfg};
  scoped_actor self{system};
  anon_send_exit(*g_master_actor, exit_reason::user_shutdown);
  anon_send_exit(*g_stratum_actor, exit_reason::user_shutdown);
  self->await_all_other_actors_done();

  delete g_master_actor;
  delete g_stratum_actor;
  g_master_actor = NULL;
  g_stratum_actor = NULL;
}
