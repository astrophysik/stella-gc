#include <stella/gc.h>
#include "check.h"

static const StellaObjectDescriptor leaf_descriptor = {
  .abi_version = STELLA_C_GC_ABI_VERSION,
  .kind = STELLA_OBJECT_DATA,
  .primitive_count = 1, .slot_capacity = 1,
  .debug_name = "test leaf"
};

static const StellaObjectDescriptor parent_descriptor = {
  .abi_version = STELLA_C_GC_ABI_VERSION,
  .kind = STELLA_OBJECT_DATA,
  .managed_count = 1, .primitive_count = 1, .slot_capacity = 2,
  .debug_name = "test parent"
};

int main(void) {
  const StellaGcConfig config = {STELLA_C_GC_ABI_VERSION, 1024 * 1024};
  stella_gc_init(&config);
  StellaValue leaf = NULL, parent = NULL, replacement = NULL, observed = NULL;
  stella_gc_push_root(&leaf);
  stella_gc_push_root(&parent);
  stella_gc_push_root(&replacement);
  stella_gc_push_root(&observed);

  leaf = stella_gc_alloc(&leaf_descriptor);
  stella_object_init_primitive(leaf, 0, 42);
  parent = stella_gc_alloc(&parent_descriptor);
  stella_object_init_managed(parent, 0, leaf);
  stella_object_init_primitive(parent, 0, 7);
  observed = stella_gc_read_managed(&parent, 0);
  CHECK(observed == leaf);
  CHECK(stella_object_read_primitive(observed, 0) == 42);
  CHECK(stella_object_read_primitive(parent, 0) == 7);
  stella_object_write_primitive(parent, 0, 99);
  CHECK(stella_object_read_primitive(parent, 0) == 99);

  replacement = stella_gc_alloc(&leaf_descriptor);
  stella_object_init_primitive(replacement, 0, 123);
  stella_gc_write_managed(&parent, 0, &replacement);
  observed = stella_gc_read_managed(&parent, 0);
  CHECK(observed == replacement);
  CHECK(stella_object_read_primitive(observed, 0) == 123);

  stella_gc_pop_root(&observed);
  stella_gc_pop_root(&replacement);
  stella_gc_pop_root(&parent);
  stella_gc_pop_root(&leaf);
  stella_gc_shutdown();
  return EXIT_SUCCESS;
}
