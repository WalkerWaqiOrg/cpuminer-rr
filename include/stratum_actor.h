#ifndef STRATUM_PROTOCOL_H
#define STRATUM_PROTOCOL_H

// CAF
#include "caf/all.hpp"
#include "caf/io/all.hpp"

#include <curl/curl.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>

#include "user_params.h"
#include "miner_actors.h"

using namespace caf;

#define STATE_DISCONNECTED 0
#define STATE_CONNECTED 1
#define STATE_GOT_TOKEN 2
#define STATE_POOL_BANNED 3
#define STATE_POOL_GENERIC_ERROR 4

#define TOKEN_MAX_LEN 256

using submit_atom = atom_constant<atom("submit")>;
using get_info_atom = atom_constant<atom("getinfo")>;

class stratum_receive_state {
public:
  curl_socket_t sock;
  char *sockbuf;
  size_t sockbuf_size;

  ~stratum_receive_state() {
    if (sockbuf) {
      free(sockbuf);
    }
  }
};

class Global_state_info {
public:
  char token[TOKEN_MAX_LEN];
  int state;

  Global_state_info(): state(STATE_DISCONNECTED) {
    memset(token, 0, TOKEN_MAX_LEN);
  }

  Global_state_info(const Global_state_info &global_state_info) {
    for (int i = 0; i < TOKEN_MAX_LEN; ++i) {
      token[i] = global_state_info.token[i];
    }
    state = global_state_info.state;
  }
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f, Global_state_info& x) {
  return f(meta::type_name("Global_state_info"), x.token, x.state);
}

class stratum_state {
public:
  Job job;

  char token[TOKEN_MAX_LEN];
  int state;

  actor stratum_receive_actor;

  virtual ~stratum_state() {
    curl_easy_cleanup(curl);
    //if (sockbuf) {
      //free(sockbuf);
    //}
  }
  int connect_to_miner_pool(stateful_actor<stratum_state>* self, const UserParams& user_params);
  bool send_login(stateful_actor<stratum_state>* self, const UserParams& user_params);
  void disconnect(stateful_actor<stratum_state>* self);
  bool send_submit(stateful_actor<stratum_state>* self);
  //char *stratum_recv_line(stateful_actor<stratum_state>* self);
  //bool socket_full(curl_socket_t sock, int timeout);
  //bool stratum_socket_full(stateful_actor<stratum_state>* self, int timeout);
  //void stratum_buffer_append(stateful_actor<stratum_state>* self, const char *s);

public:
  CURL *curl;
  char curl_err_buf[256];
  curl_socket_t sock;
  //char *sockbuf;
  //size_t sockbuf_size;
};

behavior stratum_handler(stateful_actor<stratum_state>* self, const UserParams& user_params,
  const actor& miner_master);

#endif
