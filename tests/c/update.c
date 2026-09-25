#include <stella/gc.h>
#include "check.h"

static const StellaObjectDescriptor before = {
  .abi_version = STELLA_C_GC_ABI_VERSION,
  .kind = STELLA_OBJECT_DATA, .flags = STELLA_DESCRIPTOR_UPDATABLE,
  .primitive_count = 2, .slot_capacity = 2,
  .debug_name = "before update"
};

static const StellaObjectDescriptor after = {
  .abi_version = STELLA_C_GC_ABI_VERSION,
  .kind = STELLA_OBJECT_DATA,
  .managed_count = 1, .primitive_count = 1, .slot_capacity = 2,
  .debug_name = "after update"
};

static const StellaObjectDescriptor leaf_descriptor = {
  .abi_version = STELLA_C_GC_ABI_VERSION,
  .kind = STELLA_OBJECT_DATA,
  .primitive_count = 1, .slot_capacity = 1,
  .debug_name = "update target"
};

/* A permanent root slot must remain alive until shutdown. */
static StellaValue permanent;

int main(void) {
  const StellaGcConfig config = {STELLA_C_GC_ABI_VERSION, 1024 * 1024};
  stella_gc_init(&config);
  stella_gc_register_permanent_root(&permanent);
  StellaValue child = NULL, observed = NULL;
  stella_gc_push_root(&child);
  stella_gc_push_root(&observed);
  permanent = stella_gc_alloc(&before);
  stella_object_init_primitive(permanent, 0, 11);
  stella_object_init_primitive(permanent, 1, 22);
  child = stella_gc_alloc(&leaf_descriptor);
  stella_object_init_primitive(child, 0, 55);

  StellaValue *managed[] = {&child};
  const StellaPrimitive primitives[] = {77};
  stella_gc_update_object(&permanent, &after, managed, primitives);
  CHECK(permanent->descriptor == &after);
  CHECK(stella_object_read_primitive(permanent, 0) == 77);
  observed = stella_gc_read_managed(&permanent, 0);
  CHECK(observed == child);
  CHECK(stella_object_read_primitive(observed, 0) == 55);
  stella_object_write_primitive(permanent, 0, 88);
  CHECK(stella_object_read_primitive(permanent, 0) == 88);

  stella_gc_pop_root(&observed);
  stella_gc_pop_root(&child);
  stella_gc_shutdown();
  return EXIT_SUCCESS;
}
