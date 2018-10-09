#ifndef USER_PARAMS_H
#define USER_PARAMS_H

#include <stdlib.h>
#include <string.h>

class UserParams {
public:
    char *user;
    char *url;
    int actors;
    bool request_token;
    int retry_pause;

    UserParams(): user(NULL), url(NULL), actors(1), request_token(false), retry_pause(10) {}

    UserParams(const UserParams &user_params): actors(user_params.actors),
        request_token(user_params.request_token), retry_pause(user_params.retry_pause) {
        if (user_params.user) {
            user = strdup(user_params.user);
        } else {
            user = NULL;
        }
        if (user_params.url) {
            url = strdup(user_params.url);
        } else {
            url = NULL;
        }
    }

    virtual ~UserParams() {
        if (user) {
            free(user);
        }
        if (url) {
            free(url);
        }
    }
    void parse_arg(int key, char *arg);
    void parse_cmdline(int argc, char *argv[]);
};

#endif
