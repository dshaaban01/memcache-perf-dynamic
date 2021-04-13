#ifndef CONNECTIONOPTIONS_H
#define CONNECTIONOPTIONS_H

#include "distributions.h"

#define MAX_DYN   32

typedef struct {
  int connections;
  bool blocking;
  double lambda;
  int qps;
  int records;

  bool binary;
  bool sasl;
  char username[32];
  char password[32];

  char keysize[32];
  char valuesize[32];
  char keyorder[32];
  // int keysize;
  //  int valuesize;
  char ia[32];

  // qps_per_connection
  // iadist

  double update;
  int time;
  bool loadonly;
  int depth;
  bool no_nodelay;
  bool noload;
  int threads;
  enum distribution_t iadist;
  int warmup;
  bool skip;

  bool roundrobin;
  int server_given;
  int lambda_denom;

  bool oob_thread;

  bool moderate;
  double getq_freq;
  int getq_size;

  int dyn_agent;
  int dyn_en;
  int trace_en;
  int qps_min;
  int qps_max;
  double qps_interval;
  int n_intervals;
  int qps_measure;
  int qps_seed;

  int *qps_dyn;
  double *lambda_dyn; 

} options_t;

#endif // CONNECTIONOPTIONS_H
