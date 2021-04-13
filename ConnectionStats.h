/* -*- c++ -*- */
#ifndef CONNECTIONSTATS_H
#define CONNECTIONSTATS_H

#include <algorithm>
#include <inttypes.h>
#include <vector>
#include <iostream>

#include "AgentStats.h"
#include "Operation.h"

#include "LogHistogramSampler.h"

using namespace std;

#ifdef STATIC_ALLOC_SAMPLER
// Static allocation

    class ConnectionStats {
    public:
        static int details[];
        static int ndetails;

        LogHistogramSampler get_sampler;
        LogHistogramSampler set_sampler;
        LogHistogramSampler op_sampler;

        uint64_t rx_bytes, tx_bytes;
        uint64_t gets, sets, get_misses;
        uint64_t skips;
        uint64_t gets_dyn[MAX_INTERVALS], sets_dyn[MAX_INTERVALS];

        double start, stop;

        bool sampling;
        bool plotall;

        int n_intervals;

        // Constructor
        ConnectionStats(bool _sampling = true, int n_intervals = 1) :
            get_sampler(LOGSAMPLER_BINS, n_intervals), set_sampler(LOGSAMPLER_BINS, n_intervals), op_sampler(LOGSAMPLER_BINS, n_intervals),
            rx_bytes(0), tx_bytes(0), gets(0), sets(0), start(0), stop(0), plotall(false),
            get_misses(0), skips(0), sampling(_sampling) {
                
                this->n_intervals = n_intervals;
                for(int i = 0; i < MAX_INTERVALS; i++) {
                    gets_dyn[i] = 0;
                    sets_dyn[i] = 0;
                }
        }

        // Logging functions
        void log_get(Operation& op) { if (sampling) get_sampler.sample(op); gets++; gets_dyn[op.interval]++;}
        void log_set(Operation& op) { if (sampling) set_sampler.sample(op); sets++; sets_dyn[op.interval]++;}
        void log_op (double op)     { if (sampling)  op_sampler.sample(op); }
    
        // Get overall qps
        double get_qps() {
            return (gets + sets) / (stop - start);
        }
    
        double get_nth(double nth, int interval = 0) {
            double get_val=get_sampler.get_nth(nth, interval);
            double set_val=set_sampler.total(interval)>0 ? set_sampler.get_nth(nth, interval):0.0;
            double ret_val=get_val>set_val?get_val:set_val;
            return ret_val;
        }

        double get_avg(int interval = 0) {
            double get_val=get_sampler.average(interval);
            double set_val=set_sampler.total(interval)>0 ? set_sampler.average(interval):0.0;
            double ret_val=get_val>set_val?get_val:set_val;
            return ret_val;
        }

        // Accumulate 
        void accumulate(const ConnectionStats &cs) {
            get_sampler.accumulate(cs.get_sampler);
            set_sampler.accumulate(cs.set_sampler);
            op_sampler.accumulate(cs.op_sampler);

            rx_bytes += cs.rx_bytes;
            tx_bytes += cs.tx_bytes;
            gets += cs.gets;
            sets += cs.sets;

            for(int i = 0; i < n_intervals; i++) {
                gets_dyn[i] += cs.gets_dyn[i];
                sets_dyn[i] += cs.sets_dyn[i];
            }

            get_misses += cs.get_misses;
            skips += cs.skips;

            start = cs.start;
            stop = cs.stop;
        }

        // Accumulate - send from agent to master
        void accumulate(const AgentStats &as) {
            rx_bytes += as.rx_bytes;
            tx_bytes += as.tx_bytes;
            gets += as.gets;
            sets += as.sets;
            get_misses += as.get_misses;
            skips += as.skips;

            start = as.start;
            stop = as.stop;

            for(int i = 0; i < n_intervals; i++) {
                gets_dyn[i] += as.gets_dyn[i];
                sets_dyn[i] += as.sets_dyn[i];
            }

        #ifdef LOGSAMPLER_BINS
            for (int i = 0; i < n_intervals; i++) {
                for (int j=0; j<LOGSAMPLER_BINS; j++) 
                    get_sampler.bins[i][j]	+=	as.get_bins[i][j];
                get_sampler.sum[i] 	+=	as.get_sum[i];
                get_sampler.sum_sq[i]	+=	as.get_sum_sq[i];
            }
        #endif
        }

        static void print_header(bool newline=true) {
            int i;
            printf("%-7s %7s %7s %7s",
                "#type", "avg", "std", "min");
            for (i=0; i<ndetails; i++) {
                char buf[8];
                sprintf(buf,"p%d",details[i]); 
                printf(" %7s",buf);
            }
            if (newline) 
                    printf("\n");
        }

        void print_stats(const char *tag, LogHistogramSampler &sampler,
                        bool newline = true, bool plotit=false, int interval = 0) {
            int i;
            
            if (sampler.total(interval) == 0) {
            printf("%-7s %7.1f %7.1f %7.1f",
                    tag, 0.0, 0.0, 0.0);
                for (i=0; i<ndetails; i++) {
                    printf(" %7.1f",0.0);
                }
                    
            if (newline) printf("\n");
            return;
            }

            printf("%-7s %7.1f %7.1f %7.1f",
                tag, sampler.average(interval), sampler.stddev(interval),
                sampler.get_nth(0, interval));

                for (i=0; i<ndetails; i++) {
                    printf(" %7.1f",sampler.get_nth(details[i], interval));
                }

            if (newline) 
                printf("\n");

            if (plotit || plotall)
                sampler.plot(tag,get_qps());
        }

    };

#else
// Dynamic allocation

    class ConnectionStats {
    public:
        static int details[];
        static int ndetails;

        LogHistogramSampler get_sampler;
        LogHistogramSampler set_sampler;
        LogHistogramSampler op_sampler;

        uint64_t rx_bytes, tx_bytes;
        uint64_t gets, sets, get_misses;
        uint64_t skips;
        uint64_t *gets_dyn, *sets_dyn;

        double start, stop;

        bool sampling;
        bool plotall;

        int n_intervals;

        // Constructor
        ConnectionStats(bool _sampling = true, int n_intervals = 1) :
            get_sampler(LOGSAMPLER_BINS, n_intervals), set_sampler(LOGSAMPLER_BINS, n_intervals), op_sampler(LOGSAMPLER_BINS, n_intervals),
            rx_bytes(0), tx_bytes(0), gets(0), sets(0), start(0), stop(0), plotall(false),
            get_misses(0), skips(0), sampling(_sampling) {
                
                this->n_intervals = n_intervals;
                gets_dyn = new uint64_t[n_intervals] ();
                sets_dyn = new uint64_t[n_intervals] ();
        }

        // Destructor
        ~ConnectionStats() {
            delete[] gets_dyn;
            delete[] sets_dyn;
        }

        // Logging functions
        void log_get(Operation& op) { if (sampling) get_sampler.sample(op); gets++; gets_dyn[op.interval]++;}
        void log_set(Operation& op) { if (sampling) set_sampler.sample(op); sets++; sets_dyn[op.interval]++;}
        void log_op (double op)     { if (sampling)  op_sampler.sample(op); }
    
        // Get overall qps
        double get_qps() {
            return (gets + sets) / (stop - start);
        }
    
        double get_nth(double nth, int interval = 0) {
            double get_val=get_sampler.get_nth(nth, interval);
            double set_val=set_sampler.total(interval)>0 ? set_sampler.get_nth(nth, interval):0.0;
            double ret_val=get_val>set_val?get_val:set_val;
            return ret_val;
        }

        double get_avg(int interval = 0) {
            double get_val=get_sampler.average(interval);
            double set_val=set_sampler.total(interval)>0 ? set_sampler.average(interval):0.0;
            double ret_val=get_val>set_val?get_val:set_val;
            return ret_val;
        }

        // Accumulate 
        void accumulate(const ConnectionStats &cs) {
            get_sampler.accumulate(cs.get_sampler);
            set_sampler.accumulate(cs.set_sampler);
            op_sampler.accumulate(cs.op_sampler);

            rx_bytes += cs.rx_bytes;
            tx_bytes += cs.tx_bytes;
            gets += cs.gets;
            sets += cs.sets;

            for(int i = 0; i < n_intervals; i++) {
                gets_dyn[i] += cs.gets_dyn[i];
                sets_dyn[i] += cs.sets_dyn[i];
            }

            get_misses += cs.get_misses;
            skips += cs.skips;

            start = cs.start;
            stop = cs.stop;
        }

        // Accumulate - send from agent to master
        void accumulate(const AgentStats &as) {
            rx_bytes += as.bs.rx_bytes;
            tx_bytes += as.bs.tx_bytes;
            gets += as.bs.gets;
            sets += as.bs.sets;
            get_misses += as.bs.get_misses;
            skips += as.bs.skips;

            start = as.bs.start;
            stop = as.bs.stop;

            for(int i = 0; i < n_intervals; i++) {
                gets_dyn[i] += as.gets_dyn[i];
                sets_dyn[i] += as.sets_dyn[i];
            }

        #ifdef LOGSAMPLER_BINS
            for (int i = 0; i < n_intervals; i++) {
                for (int j=0; j<LOGSAMPLER_BINS; j++) 
                    get_sampler.bins[i][j]	+=	as.get_bins[i][j];
                get_sampler.sum[i] 	+=	as.get_sum[i];
                get_sampler.sum_sq[i]	+=	as.get_sum_sq[i];
            }
        #endif
        }

        static void print_header(bool newline=true) {
            int i;
            printf("%-7s %7s %7s %7s",
                "#type", "avg", "std", "min");
            for (i=0; i<ndetails; i++) {
                char buf[8];
                sprintf(buf,"p%d",details[i]); 
                printf(" %7s",buf);
            }
            if (newline) 
                    printf("\n");
        }

        void print_stats(const char *tag, LogHistogramSampler &sampler,
                        bool newline = true, bool plotit=false, int interval = 0) {
            int i;
            
            if (sampler.total(interval) == 0) {
            printf("%-7s %7.1f %7.1f %7.1f",
                    tag, 0.0, 0.0, 0.0);
                for (i=0; i<ndetails; i++) {
                    printf(" %7.1f",0.0);
                }
                    
            if (newline) printf("\n");
            return;
            }

            printf("%-7s %7.1f %7.1f %7.1f",
                tag, sampler.average(interval), sampler.stddev(interval),
                sampler.get_nth(0, interval));

                for (i=0; i<ndetails; i++) {
                    printf(" %7.1f",sampler.get_nth(details[i], interval));
                }

            if (newline) 
                printf("\n");

            if (plotit || plotall)
                sampler.plot(tag,get_qps());
        }

    };

#endif

#endif // CONNECTIONSTATS_H
