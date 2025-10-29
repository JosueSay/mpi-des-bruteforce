#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>

// tiempo simple para reportes
typedef struct
{
  double secs;
  long nsec;
} time_span_t;

// header csv único para todo el proyecto
#define CSV_HEADER "implementation,mode,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text,out_bin"

#endif
