#include <stdio.h>

#include "user_params.h"
#include "util.h"

#define HAVE_GETOPT_LONG 1 // todo: config

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
struct option {
    const char *name;
    int has_arg;
    int *flag;
    int val;
};
#endif

#define PROGRAM_NAME		"minerd"

static char const usage[] =
        "\
Usage: " PROGRAM_NAME " [OPTIONS]\n\
Options:\n\
  -o, --url=URL         URL of mining server\n\
  -u, --user=USERNAME   username for mining server\n\
  -t, --threads=N       number of miner threads (default: number of processors)\n\
  -R, --retry-pause=N   time to pause between retries, in seconds (default: 30)\n\
";

static char const short_options[] =
        "ahorRu";

static struct option const options[] = {
        { "help", 0, NULL, 'h' },
        { "retry-pause", 1, NULL, 'r' },
        { "actors", 1, NULL, 'a' },
        { "url", 1, NULL, 'o' },
        { "user", 1, NULL, 'u' },
        { "request-token", 0, NULL, 'R' },
        { 0, 0, 0, 0 }
};

static void show_usage_and_exit(int status) {
    if (status)
        fprintf(stderr,
                "Try `" PROGRAM_NAME " --help' for more information.\n");
    else
        printf(usage);
    exit(status);
}

void UserParams::parse_arg(int key, char *arg) {
    char *p;
    int v, i;

    switch (key) {
    case 'h':
        show_usage_and_exit(0);
        break;
    case 'a':
        v = atoi(arg);
        if ((v < 0) || (v > 1000)) {
            show_usage_and_exit(1);
        }
        actors = v;
        break;
    case 'o':
        p = strstr(arg, "://");
        if (p) {
            if (strncasecmp(arg, "http://", 7)
                    && strncasecmp(arg, "https://", 8)
                    && strncasecmp(arg, "stratum+tcp://", 14))
                show_usage_and_exit(1);
            if (this->url) {
                free(this->url);
            }
            this->url = strdup(arg);
        } else {
            if (!strlen(arg) || *arg == '/')
                show_usage_and_exit(1);
            if (this->url) {
                free(this->url);
            }
            this->url = (char *)malloc(strlen(arg) + 8);
            sprintf(this->url, "http://%s", arg);
        }
        break;
    case 'r':
        v = atoi(arg);
        if ((v < 1) || (v > 9999)) {
            show_usage_and_exit(1);
        }
        this->retry_pause = v;
        break;
    case 'R':
        this->request_token = true;
        break;
    default:
        break;
    }
}

void UserParams::parse_cmdline(int argc, char *argv[]) {
    this->url = strdup("http://114.115.135.123:3333");
    this->user = strdup("1");
    this->request_token = false;
    this->retry_pause = 1; // todo: ms
    this->actors = 1;//util::get_num_processors() - 1;

    int key;

    while (1) {
#if HAVE_GETOPT_LONG
        key = getopt_long(argc, argv, short_options, options, NULL );
#else
        key = getopt(argc, argv, short_options);
#endif
        if (key < 0)
            break;

        parse_arg(key, optarg);
    }

}
