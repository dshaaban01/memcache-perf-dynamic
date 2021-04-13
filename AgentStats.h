/* -*- c++ -*- */
#ifndef AGENTSTATS_H
#define AGENTSTATS_H

#include <vector>
#include <array>
#include <iostream>

#include "LogHistogramSampler.h"

#ifdef STATIC_ALLOC_SAMPLER
// Static allocation

  class AgentStats {
  public:
    // Base stats
    uint64_t rx_bytes, tx_bytes;
    uint64_t gets, sets, get_misses;
    uint64_t skips;
    double start, stop;


    // Dynamic stats
    uint64_t gets_dyn[MAX_INTERVALS], sets_dyn[MAX_INTERVALS];

    // Sampler stats
    uint64_t get_bins[MAX_INTERVALS][LOGSAMPLER_BINS];
    double get_sum[MAX_INTERVALS];
    double get_sum_sq[MAX_INTERVALS];
    
  };

#else
// Dynamic allocation

  class BaseStats {
  public:
    // Base stats
    uint64_t rx_bytes, tx_bytes;
    uint64_t gets, sets, get_misses;
    uint64_t skips;
    double start, stop;
  };

  class AgentStats {
  public:
    int n_intervals;

    // Base stats
    BaseStats bs;

    // Dynamic stats
    uint64_t *gets_dyn, *sets_dyn;

    // Sampler stats
    uint64_t **get_bins;
    double *get_sum;
    double *get_sum_sq;
    
    // Constructor
    AgentStats(int n_intervals) {
      this->n_intervals = n_intervals;

      gets_dyn = new uint64_t[n_intervals];
      sets_dyn = new uint64_t[n_intervals];

      get_bins = new uint64_t*[n_intervals];
      for(int i = 0; i < n_intervals; i++) {
        get_bins[i] = new uint64_t[LOGSAMPLER_BINS];
      }

      get_sum = new double[n_intervals];
      get_sum_sq = new double[n_intervals];
    };

    // Destructor
    ~AgentStats() {
      delete[] gets_dyn;
      delete[] sets_dyn;

      for(int i = 0; i < n_intervals; i++) {
        delete[] get_bins[i];
      }
      delete[] get_bins;

      delete[] get_sum;
      delete[] get_sum_sq;
    }

    void print_base() {
      std::cout << "rx: " << bs.rx_bytes << ", tx: " << bs.tx_bytes << std::endl;
      std::cout << "gets: " << bs.gets << ", sets: " << bs.sets << std::endl;
    }

    void print_dyn() {
      for(int i = 0; i < n_intervals; i++) {
        std::cout << i << "- gets_dyn: " << gets_dyn[i] << ", sets_dyn: " << sets_dyn[i] << std::endl;
      }
    }

    void print_bins() {
      for(int i = 0; i < n_intervals; i++) {
        std::cout << "bin: " << i << std::endl;
        for(int j = 0; j < LOGSAMPLER_BINS; j++) {
          std::cout << get_bins[i][j] << std::endl;
        }
      }
    }

    void print_sum() {
      for(int i = 0; i < n_intervals; i++) {
        std::cout << i << "- sum: " << get_sum[i] << ", sum_sq: " << get_sum_sq[i] << std::endl;
      }
    }

  };

#endif

#endif // AGENTSTATS_H
