#ifndef MINER_ACTORS_H
#define MINER_ACTORS_H

#include <boost/shared_ptr.hpp>

// CAF
#include "caf/all.hpp"
#include "caf/io/all.hpp"

#include "user_params.h"

using namespace caf;

using stratum_actor_atom = atom_constant<atom("stra_actor")>;
using new_job_atom = atom_constant<atom("new_job")>;
using next_hash_atom = atom_constant<atom("next_hash")>;
using get_statistics_atom = atom_constant<atom("statistics")>;

#define JOB_DATA_LEN 32
#define JOB_TARGET_LEN 8

class Job {
public:
  uint32_t data[JOB_DATA_LEN];
  uint32_t target[JOB_TARGET_LEN];
  char *job_id;

  Job(): job_id(NULL) {}

  Job(const Job &job) {
    for (int i = 0; i < JOB_DATA_LEN; ++i) {
      data[i] = job.data[i];
    }
    for (int i = 0; i < JOB_TARGET_LEN; ++i) {
      target[i] = job.target[i];
    }
    if (job.job_id) {
      job_id = strdup(job.job_id);
    } else {
      job_id = NULL;
    }
  }

  virtual ~Job() {
    if (job_id) {
      //free(job_id);
    }
  }
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f, Job& x) {
  return f(meta::type_name("Job"), x.data, x.target);
}

class miner_master_state {
public:
  std::vector<actor> idle;
  struct timeval last_time;
};

behavior miner_master(stateful_actor<miner_master_state>* self, const UserParams& user_params);

#endif
