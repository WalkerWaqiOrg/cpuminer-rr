#ifndef __CPUMINER_LIB_H__
#define __CPUMINER_LIB_H__

#ifdef __cplusplus
extern "C" {
#endif

int start_miner(int argc, char *argv[]);
void stop_miner();
int get_num_processors();
int get_state();
char *get_token();

#ifdef __cplusplus
}
#endif

#endif /* __CPUMINER_LIB_H__ */
