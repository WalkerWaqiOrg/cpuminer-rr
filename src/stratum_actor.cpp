#include <unistd.h>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "stratum_actor.h"
#include "util.h"
#include "miner_actors.h"
#include "stratum_json.h"
#include "rrhash.h"

using connect_atom = atom_constant<atom("connect")>;
using login_atom = atom_constant<atom("login")>;
using receive_atom = atom_constant<atom("receive")>;
using disconnect_atom = atom_constant<atom("disconnect")>;
using update_info_atom = atom_constant<atom("updateinfo")>;

using std::chrono::milliseconds;

#define JSON_BUF_LEN 345
#define RBUFSIZE 2048
#define RECVSIZE (RBUFSIZE - 4)

#if LIBCURL_VERSION_NUM >= 0x071101
static curl_socket_t opensocket_grab_cb(void *clientp, curlsocktype purpose,
	struct curl_sockaddr *addr)
{
	curl_socket_t *sock = (curl_socket_t *)clientp;
	*sock = socket(addr->family, addr->socktype, addr->protocol);
	return *sock;
}
#endif

int stratum_state::connect_to_miner_pool(stateful_actor<stratum_state>* self, const UserParams& user_params) {
    if (self->state.curl) {
        curl_easy_cleanup(self->state.curl);
    }
    self->state.curl = curl_easy_init();
    if (!self->state.curl) {
        // todo: error handle
    }
	/*if (!sockbuf) {
		sockbuf = (char *)calloc(RBUFSIZE, 1);
		sockbuf_size = RBUFSIZE;
	}
	sockbuf[0] = '\0';*/
    //char* url = (char*)"http://114.115.135.123:3333";
    curl_easy_setopt(self->state.curl, CURLOPT_URL, user_params.url);
    curl_easy_setopt(self->state.curl, CURLOPT_FRESH_CONNECT, 1);
    curl_easy_setopt(self->state.curl, CURLOPT_CONNECTTIMEOUT, 30);
    curl_easy_setopt(self->state.curl, CURLOPT_ERRORBUFFER, self->state.curl_err_buf);
    curl_easy_setopt(self->state.curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(self->state.curl, CURLOPT_TCP_NODELAY, 1);
    curl_easy_setopt(self->state.curl, CURLOPT_HTTPPROXYTUNNEL, 1);
#if LIBCURL_VERSION_NUM >= 0x070f06
    //curl_easy_setopt(self->state.curl, CURLOPT_SOCKOPTFUNCTION, sockopt_keepalive_cb);
#endif
#if LIBCURL_VERSION_NUM >= 0x071101
    curl_easy_setopt(self->state.curl, CURLOPT_OPENSOCKETFUNCTION, opensocket_grab_cb);
    curl_easy_setopt(self->state.curl, CURLOPT_OPENSOCKETDATA, &(self->state.sock));
#else
    /* CURLINFO_LASTSOCKET is broken on Win64; only use it as a last resort */
    curl_easy_getinfo(curl, CURLINFO_LASTSOCKET, (long *)&sctx->sock);
#endif
    curl_easy_setopt(self->state.curl, CURLOPT_CONNECT_ONLY, 1);

    int ret = curl_easy_perform(self->state.curl);
    if (ret) {
        curl_easy_cleanup(self->state.curl);
        self->state.curl = NULL;
    }
    return ret;
}

#ifdef WIN32
#define socket_blocks() (WSAGetLastError() == WSAEWOULDBLOCK)
#else
#define socket_blocks() (errno == EAGAIN || errno == EWOULDBLOCK)
#endif

static bool send_line(curl_socket_t sock, char *s)
{
	ssize_t len, sent = 0;
	
	len = strlen(s);
	s[len++] = '\n';

	while (len > 0) {
		struct timeval timeout = {0, 0};
		ssize_t n;
		fd_set wd;

		FD_ZERO(&wd);
		FD_SET(sock, &wd);
		if (select(sock + 1, NULL, &wd, NULL, &timeout) < 1)
			return false;
		n = send(sock, s + sent, len, 0);
		if (n < 0) {
			if (!socket_blocks())
				return false;
			n = 0;
		}
		sent += n;
		len -= n;
	}

	return true;
}

bool stratum_state::send_login(stateful_actor<stratum_state>* self, const UserParams& user_params) {
    char* str = (char*)malloc(200);
    if (user_params.request_token) {
        sprintf(str, "{\"method\": \"login\", \"params\": {\"hc\": %d, \"token\": %s, \"uid\": \"%s\"}}",
            util::get_num_processors(), "true", user_params.user);
    } else {
    	sprintf(str, "{\"method\": \"login\", \"params\": {\"hc\": %d, \"uid\": \"%s\"}}",
			util::get_num_processors(), user_params.user);
    }
    fflush(stdout);
    bool ret = send_line(self->state.sock, str);
    free(str);
    return ret;
}

bool stratum_state::send_submit(stateful_actor<stratum_state>* self) {
    char str[JSON_BUF_LEN];
    char* noncestr_buf = (char*)malloc(9);
    noncestr_buf[8] = 0;
    char hash[32];
    char hash_buf[32];
    char *noncestr = util::bin2hex(((const unsigned char*)self->state.job.data) + 84, 4);
    rrhash((char *)job.data, 88, (unsigned char *)hash);
    for (int i = 0; i < sizeof(noncestr)/2; ++i) {
        for (int j = 0; j < 2; ++j) {
            noncestr_buf[(sizeof(noncestr)/2 - 1 - i) * 2 + j] = noncestr[i * 2 + j];
        }
    }

    for (int i = 0; i < sizeof(hash); ++i) {
        hash_buf[sizeof(hash) - 1 - i] = hash[i];
    }
    char *hashhex = util::bin2hex((unsigned char *)hash_buf, 32);
    memset(str, 0, JSON_BUF_LEN);
    snprintf(str, JSON_BUF_LEN,
        "{\"method\": \"submit\", \"params\": {\"id\": \"%s\", \"hash\": \"%s\", \"nonce\": \"%s\"}}",
        self->state.job.job_id, hashhex, noncestr_buf);
    printf("----------Found mine! nonce: %s\n", noncestr_buf);
    fflush(stdout);

    bool ret = send_line(self->state.sock, str);
    free(hashhex);
    free(noncestr_buf);
    free(noncestr);
    return ret;
}

void stratum_state::disconnect(stateful_actor<stratum_state>* self) {
    curl_easy_cleanup(self->state.curl);
    self->state.curl = NULL;
}

static bool socket_full(curl_socket_t sock, int timeout)
{
	struct timeval tv;
	fd_set rd;

	FD_ZERO(&rd);
	FD_SET(sock, &rd);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	if (select(sock + 1, &rd, NULL, NULL, &tv) > 0)
		return true;
	return false;
}

static bool stratum_socket_full(char *sockbuf, curl_socket_t sock, int timeout)
{
	return strlen(sockbuf) || socket_full(sock, timeout);
}

static void stratum_buffer_append(stateful_actor<stratum_receive_state>* self, const char *s)
{
	size_t old_size, new_size;

	old_size = strlen(self->state.sockbuf);
	new_size = old_size + strlen(s) + 1;
	if (new_size >= self->state.sockbuf_size) {
		self->state.sockbuf_size = new_size + (RBUFSIZE - (new_size % RBUFSIZE));
		self->state.sockbuf = (char *)realloc(self->state.sockbuf, self->state.sockbuf_size);
	}
	strcpy(self->state.sockbuf + old_size, s);
}

static char* stratum_recv_line(stateful_actor<stratum_receive_state>* self) {
	ssize_t len, buflen;
	char *tok, *sret = NULL;

	if (!strstr(self->state.sockbuf, "\n")) {
		bool ret = true;
		time_t rstart;

		time(&rstart);
		if (!socket_full(self->state.sock, 60)) {
			printf("stratum_recv_line timed out");
      fflush(stdout);
			goto out;
		}
		do {
			char s[RBUFSIZE];
			ssize_t n;

			memset(s, 0, RBUFSIZE);
			n = recv(self->state.sock, s, RECVSIZE, 0);
			if (!n) {
				printf("Socket error: %s\n", strerror(errno));
        fflush(stdout);
				ret = false;
				break;
			}
			if (n < 0) {
				if (!socket_blocks() || !socket_full(self->state.sock, 1)) {
					ret = false;
					break;
				}
			} else
				stratum_buffer_append(self, s);
		} while (time(NULL) - rstart < 60 && !strstr(self->state.sockbuf, "\n"));

		if (!ret) {
			printf("stratum_recv_line failed\n");
      fflush(stdout);
			goto out;
		}
	}

	buflen = strlen(self->state.sockbuf);
	tok = strtok(self->state.sockbuf, "\n");
	if (!tok) {
		printf("stratum_recv_line failed to parse a newline-terminated string");
    fflush(stdout);
		goto out;
	}
	sret = strdup(tok);
	len = strlen(sret);

	if (buflen > len + 1)
		memmove(self->state.sockbuf, self->state.sockbuf + len + 1, buflen - len + 1);
	else
		self->state.sockbuf[0] = '\0';

out:
	//if (sret && opt_protocol)
	//	applog(LOG_DEBUG, "< %s", sret);
	return sret;
}

behavior stratum_receive_handler(stateful_actor<stratum_receive_state>* self, const UserParams& user_params,
  const actor& miner_master, const actor &parent) {
  if (!self->state.sockbuf) {
    self->state.sockbuf = (char *)calloc(RBUFSIZE, 1);
	self->state.sockbuf_size = RBUFSIZE;
  }
  self->state.sockbuf[0] = '\0';
  return {
    [=](update_info_atom, int sock) {
      self->state.sock = sock;
    },
    [=](receive_atom) {
      char *str;
      if (!stratum_socket_full(self->state.sockbuf, self->state.sock, 120)) {
        printf("Stratum connection timed out\n");
        fflush(stdout);
        str = NULL;
      } else {
        str = stratum_recv_line(self);
      }
      if (str) {
        // parse Json-RPC and set to state object
        Job job;
        Global_state_info global_state_info;
        stratum_json::stratum_handle_method(job, global_state_info, str);
        if (global_state_info.state == STATE_CONNECTED) {
          // send to miner master
          self->send(miner_master, new_job_atom::value, job);
        }

        // send to parent
        self->send(parent, update_info_atom::value, global_state_info);

        // continue to receive job
        self->send(self, receive_atom::value);
      } else {
        printf("Stratum connection interrupted\n");
        self->send(parent, disconnect_atom::value);
        printf("...retry after %d seconds\n", user_params.retry_pause);
        fflush(stdout);
        self->delayed_send(parent, milliseconds(1000 * user_params.retry_pause), connect_atom::value);
      }
      free(str);
    }
  };
}

behavior stratum_handler(stateful_actor<stratum_state>* self, const UserParams& user_params,
  const actor& miner_master) {
  self->send(self, connect_atom::value);
  self->state.stratum_receive_actor = self->spawn<detached+linked>(stratum_receive_handler, user_params, miner_master, self);
  return {
    [=](connect_atom) {
      int ret = self->state.connect_to_miner_pool(self, user_params);
      if (ret) {
        printf("Connect failed.\n");
        printf("...retry after %d seconds\n", user_params.retry_pause);
        fflush(stdout);
        self->delayed_send(self, milliseconds(1000 * user_params.retry_pause), connect_atom::value);
      } else {
        printf("Connect successfully.\n");

        fflush(stdout); // todo

        self->send(self->state.stratum_receive_actor, update_info_atom::value, self->state.sock);
        self->send(self, login_atom::value);
        }
    },
    [=](login_atom) {
      bool is_successful = self->state.send_login(self, user_params);
      if (!is_successful) {
        printf("Send login failed.\n");
        printf("...retry after %d seconds\n", user_params.retry_pause);
        fflush(stdout);
        self->state.disconnect(self);
        self->delayed_send(self, milliseconds(1000 * user_params.retry_pause), connect_atom::value); // todo: handle multi connect message in queue
      } else {
        printf("Send login successfully, start to receive job.\n");
        fflush(stdout);
        self->send(self->state.stratum_receive_actor, receive_atom::value);
      }
    },
    [=](disconnect_atom) {
      self->state.disconnect(self);
    },
    [=](update_info_atom, const Global_state_info &global_state_info) {
      if (global_state_info.token[0] > 0) {
        for (int i = 0; i < TOKEN_MAX_LEN; ++i) {
          self->state.token[i] = global_state_info.token[i];
        }
      }
      if (!((self->state.state == STATE_GOT_TOKEN) && (global_state_info.state == STATE_CONNECTED))) {
        self->state.state = global_state_info.state;
      }
    },
    [=](get_info_atom) -> const Global_state_info & {
      Global_state_info ret;
      for (int i = 0; i < TOKEN_MAX_LEN; ++i) {
        ret.token[i] = self->state.token[i];
      }
      ret.state = self->state.state;
      return ret;
    },
    [=](submit_atom, Job &job) { // todo: become and error handle
      self->state.job = job;
      self->state.send_submit(self);
    }
  };
}
