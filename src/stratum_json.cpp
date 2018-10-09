#include <string.h>
#include <jansson.h>

#include "stratum_json.h"
#include "util.h"

namespace stratum_json {

#if JANSSON_MAJOR_VERSION >= 2
#define JSON_LOADS(str, err_ptr) json_loads((str), 0, (err_ptr))
#else
#define JSON_LOADS(str, err_ptr) json_loads((str), (err_ptr))
#endif

static char *rpc2_blob = NULL;
static int rpc2_bloblen = 0;
static uint32_t rpc2_target = 0;
static char *rpc2_job_id = NULL;

static bool jobj_binary(const json_t *obj, const char *key, void *buf,
        size_t buflen) {
    const char *hexstr;
    json_t *tmp;

    tmp = json_object_get(obj, key);
    if (!tmp) {
        printf("JSON key '%s' not found\n", key);
        fflush(stdout);
        return false;
    }
    hexstr = json_string_value(tmp);
    if (!hexstr) {
        printf("JSON key '%s' is not a string\n", key);
        fflush(stdout);
        return false;
    }
    if (!util::hex2bin((unsigned char *)buf, hexstr, buflen))
        return false;

    return true;
}

bool stratum_job_decode(Job& job, const json_t *job_json) {
    json_t *tmp;
    tmp = json_object_get(job_json, "id");
    if (!tmp) {
        printf("JSON inval job id\n");
        fflush(stdout);
        return false;
    }
    const char *job_id = json_string_value(tmp);
    tmp = json_object_get(job_json, "job");
    if (!tmp) {
        printf("JSON inval blob\n");
        fflush(stdout);
        return false;
    }
    const char *hexblob = json_string_value(tmp);
    int blobLen = strlen(hexblob);
    if (blobLen % 2 != 0 || ((blobLen / 2) < 40 && blobLen != 0) || (blobLen / 2) > 128) {
        printf("JSON invalid blob length\n");
        fflush(stdout);
        return false;
    }
    if (blobLen != 0) {
        unsigned char *blob = (unsigned char *)malloc(blobLen / 2);
        if (!util::hex2bin(blob, hexblob, blobLen / 2)) {
            printf("JSON inval blob\n");
            fflush(stdout);
            return false;
        }
        //memcpy(rpc2_hex, hexblob, 88); // for debug
        if (rpc2_blob) {
            free(rpc2_blob);
        }
        rpc2_bloblen = blobLen / 2;
        rpc2_blob = (char *)malloc(rpc2_bloblen);
        memcpy(rpc2_blob, blob, blobLen / 2);

        free(blob);

        uint32_t target;
        jobj_binary(job_json, "target", &target, 4);
        if(rpc2_target != target) {
            // todo
            //float hashrate = 0.;
            //for (size_t i = 0; i < opt_n_threads; i++)
              //  hashrate += thr_hashrates[i];
            double difficulty = (((double) 0xffffffff) / target);
            printf("Pool set diff to %g\n", difficulty); // todo
            fflush(stdout);
            rpc2_target = target;
        }

        if (rpc2_job_id) {
            free(rpc2_job_id);
        }
        rpc2_job_id = strdup(job_id);
    }

    if (!rpc2_blob) {
        printf("Error: received empty blob data!\n");
        fflush(stdout);
        return false;
    }
    memcpy(job.data, rpc2_blob, rpc2_bloblen);
    memset(job.target, 0xff, sizeof(job.target));
    job.target[7] = rpc2_target;
    if (job.job_id) {
        free(job.job_id);
    }
    job.job_id = strdup(rpc2_job_id);
    return true;
}

bool stratum_handle_method(Job &job, Global_state_info &global_state_info, const char *s) {
	json_t *val, *id, *params;
	json_error_t err;
	const char *method;
	bool ret = false;

	val = JSON_LOADS(s, &err);
	if (!val) {
		printf("JSON decode failed(%d): %s\n", err.line, err.text);
        fflush(stdout);
		goto out;
	}

	method = json_string_value(json_object_get(val, "method"));
	if (!method)
		goto out;
	id = json_object_get(val, "id");
	params = json_object_get(val, "params");

    if (!strcasecmp(method, "job")) {
        global_state_info.state = STATE_CONNECTED;
        ret = stratum_job_decode(job, params);
        goto out;
    }
    if (!strcasecmp(method, "token")) {
        memset(global_state_info.token, 0, TOKEN_MAX_LEN);
        const char *data = json_string_value(json_object_get(val, "data"));
        memcpy(global_state_info.token, data, strlen(data));
	    global_state_info.state = STATE_GOT_TOKEN;
        goto out;
    }
    if (!strcasecmp(method, "banned")) {
        printf("Received miner pool error: banned.\n");
        fflush(stdout);
        global_state_info.state = STATE_POOL_BANNED;
		ret = true;
        goto out;
    }
    if (!strcasecmp(method, "error")) {
        global_state_info.state = STATE_POOL_GENERIC_ERROR;
        const char *msg = json_string_value(json_object_get(val, "msg"));
        printf("Received miner pool error: %s\n", msg);
        fflush(stdout);
		ret = true;
        goto out;
    }

out:
	if (val)
		json_decref(val);

	return ret;
}

}
