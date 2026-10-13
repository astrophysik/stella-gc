#ifndef STELLA_GC_STATS_H
#define STELLA_GC_STATS_H
#include <stddef.h>

typedef struct StellaGcStats {
  size_t requested_bytes;
  size_t rounded_bytes;
  size_t allocated_objects;
  size_t collections;
  size_t occupied_bytes;
  size_t maximum_occupied_bytes;
  size_t write_barrier_activations;
  size_t write_barrier_activations_on_marking;
  size_t allocations_over_budget;
  size_t reclaimed_objects;
  size_t reclaimed_bytes;
  size_t grey_object_count;
  size_t maximum_grey_object_count;
  size_t scanned_objects;
} StellaGcStats;

#endif
