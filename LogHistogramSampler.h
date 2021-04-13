/* -*- c++ -*- */
#ifndef LOGHISTOGRAMSAMPLER_H
#define LOGHISTOGRAMSAMPLER_H

#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include <vector>

#include "mcperf.h"
#include "Operation.h"

#define MAX_INTERVALS 32
//#define STATIC_ALLOC_SAMPLER

//increase resolution slightly for the range of ~10ms
#define _POW 1.08
//at bin 200 the latency is >4s
#define LOGSAMPLER_BINS 200
#ifdef GNUPLOT
#include "gnuplot_i.h"
static int nm=0;
#endif

#ifdef STATIC_ALLOC_SAMPLER
// Static allocation

  class LogHistogramSampler {
  public:
    int n_bins;
    int n_intervals;

    std::vector<Operation> samples;

    uint64_t bins[MAX_INTERVALS][LOGSAMPLER_BINS+1];

    double sum[MAX_INTERVALS];
    double sum_sq[MAX_INTERVALS];

    // Constructor
    LogHistogramSampler(int n_bins, int n_intervals = 1) {
      assert(n_bins > 0);
      assert(n_intervals >= 1);
      
      this->n_bins = n_bins;
      this->n_intervals = n_intervals;

      for(int i = 0; i < n_intervals; i++) {
        for(int j = 0; j < LOGSAMPLER_BINS+1; j++) {
          bins[i][j] = 0;
        }
        sum[i] = 0;
        sum_sq[i] = 0;
      }
    }
    LogHistogramSampler() = delete;

    // Log
    void sample(const Operation &op) {
      sample(op.time(), op.interval);
      if (args.save_given) samples.push_back(op);
    }

    // Sample
    void sample(double s, int interval = 0) {
      assert(s >= 0);
      size_t bin = log(s)/log(_POW);

      sum[interval] += s;
      sum_sq[interval] += s*s;

      if ((int64_t) bin < 0) {
        bin = 0;
      } else if (bin >= n_bins) {
        bin = n_bins - 1;
      }

      bins[interval][bin]++;
    }

    // Statistics
    uint64_t total(int interval = 0) {
      uint64_t sum = 0.0;

      for (int i = 0; i < n_bins; i++) sum += bins[interval][i];

      return sum;
    }

    double average(int interval = 0) {
      return sum[interval] / total(interval);
    }

    double stddev(int interval = 0) {
      return sqrt(sum_sq[interval] / total(interval) - pow(sum[interval] / total(interval), 2.0));
    }

    double minimum(int interval = 0) {
      for (size_t i = 0; i < n_intervals; i++)
        if (bins[interval][i] > 0) return pow(_POW, (double) i + 0.5);
      DIE("Not implemented");
    }

    double get_nth(double nth, int interval = 0) {
      uint64_t count = total(interval);
      uint64_t n = 0;
      double target = count * nth/100;

      if (nth>100.0) {
        target = count * nth/1000;
      }
      if (nth>1000.0) {
        target = count * nth/10000;
      }

      for (size_t i = 0; i < n_bins; i++) {
        n += bins[interval][i];

        if (n > target) { // The nth is inside bins[i].
          double left = target - (n - bins[interval][i]);
          return pow(_POW, (double) i) +
            left / bins[interval][i] * (pow(_POW, (double) (i+1)) - pow(_POW, (double) i));
        }
      }

      return pow(_POW, n_bins);
    } 

    // Accumulation 
    void accumulate(const LogHistogramSampler &h) {
      for(int i = 0; i < n_intervals; i++) {
          //assert(bins[i].size() == h.bins[i].size());
          for (size_t j = 0; j < n_bins; j++) bins[i][j] += h.bins[i][j];

          sum[i] += h.sum[i];
          sum_sq[i] += h.sum_sq[i];
      }

      std::vector<Operation>::const_iterator hi;
      for (hi=h.samples.begin();  hi!=h.samples.end(); hi++) samples.push_back(*hi);
    }
    
    // TODO: Re-enable
    void plot(const char *tag, double QPS) { }
  };

#else
// Dynamic allocation

    class LogHistogramSampler {
  public:
    int n_bins;
    int n_intervals;

    std::vector<Operation> samples;

    // Dynamic allocation
    uint64_t **bins;

    double *sum;
    double *sum_sq;

    // Constructor
    LogHistogramSampler(int n_bins, int n_intervals = 1) {
      assert(n_bins > 0);
      assert(n_intervals >= 1);
      
      this->n_bins = n_bins;
      this->n_intervals = n_intervals;

      bins = new uint64_t*[n_intervals];
      for(int i = 0; i < n_intervals; i++) {
        bins[i] = new uint64_t[LOGSAMPLER_BINS+1] ();
      }

      sum = new double[n_intervals] ();
      sum_sq = new double[n_intervals] ();
    }
    LogHistogramSampler() = delete;

    // Destructor
    ~LogHistogramSampler() {
      for(int i = 0; i < n_intervals; i++) {
        delete[] bins[i];
      }
      delete[] bins;

      delete[] sum;
      delete[] sum_sq;
    }

    // Log
    void sample(const Operation &op) {
      sample(op.time(), op.interval);
      if (args.save_given) samples.push_back(op);
    }

    // Sample
    void sample(double s, int interval = 0) {
      assert(s >= 0);
      size_t bin = log(s)/log(_POW);

      sum[interval] += s;
      sum_sq[interval] += s*s;

      if ((int64_t) bin < 0) {
        bin = 0;
      } else if (bin >= n_bins) {
        bin = n_bins - 1;
      }

      bins[interval][bin]++;
    }

    // Statistics
    uint64_t total(int interval = 0) {
      uint64_t sum = 0.0;

      for (int i = 0; i < n_bins; i++) sum += bins[interval][i];

      return sum;
    }

    double average(int interval = 0) {
      return sum[interval] / total(interval);
    }

    double stddev(int interval = 0) {
      return sqrt(sum_sq[interval] / total(interval) - pow(sum[interval] / total(interval), 2.0));
    }

    double minimum(int interval = 0) {
      for (size_t i = 0; i < n_intervals; i++)
        if (bins[interval][i] > 0) return pow(_POW, (double) i + 0.5);
      DIE("Not implemented");
    }

    double get_nth(double nth, int interval = 0) {
      uint64_t count = total(interval);
      uint64_t n = 0;
      double target = count * nth/100;

      if (nth>100.0) {
        target = count * nth/1000;
      }
      if (nth>1000.0) {
        target = count * nth/10000;
      }

      for (size_t i = 0; i < n_bins; i++) {
        n += bins[interval][i];

        if (n > target) { // The nth is inside bins[i].
          double left = target - (n - bins[interval][i]);
          return pow(_POW, (double) i) +
            left / bins[interval][i] * (pow(_POW, (double) (i+1)) - pow(_POW, (double) i));
        }
      }

      return pow(_POW, n_bins);
    } 

    // Accumulation 
    void accumulate(const LogHistogramSampler &h) {
      for(int i = 0; i < n_intervals; i++) {
          for (size_t j = 0; j < n_bins; j++) bins[i][j] += h.bins[i][j];

          sum[i] += h.sum[i];
          sum_sq[i] += h.sum_sq[i];
      }

      std::vector<Operation>::const_iterator hi;
      for (hi=h.samples.begin();  hi!=h.samples.end(); hi++) samples.push_back(*hi);
    }
    
    // TODO: Re-enable
    void plot(const char *tag, double QPS) { }
  };

#endif

#endif // LOGHISTOGRAMSAMPLER_H
