#include <stella/gc.h>
#include <string.h>
#include "check.h"

static const StellaObjectDescriptor node_descriptor = {
  .abi_version = STELLA_C_GC_ABI_VERSION,
  .kind = STELLA_OBJECT_DATA,
  .managed_count = 1, .primitive_count = 1, .slot_capacity = 2,
  .debug_name = "cycle node"
};

static const StellaObjectDescriptor garbage_descriptor = {
  .abi_version = STELLA_C_GC_ABI_VERSION,
  .kind = STELLA_OBJECT_DATA,
  .primitive_count = 1, .slot_capacity = 1,
  .debug_name = "temporary garbage"
};

/* This test uses the collector's documented text statistics to ensure that
 * collections actually finish and that the unreachable cycle is reclaimed. */
static size_t statistic(const char *name) {
  FILE *stream = tmpfile();
  CHECK(stream != NULL);
  stella_gc_print_statistics(stream);
  rewind(stream);
  char line[256];
  size_t result = 0;
  int found = 0;
  while (fgets(line, sizeof(line), stream) != NULL) {
    if (strncmp(line, name, strlen(name)) == 0 &&
        line[strlen(name)] == ':') {
      CHECK(sscanf(line + strlen(name) + 1, "%zu", &result) == 1);
      found = 1;
      break;
    }
  }
  CHECK(fclose(stream) == 0);
  CHECK(found);
  return result;
}

static void await_collection(void) {
  size_t before = statistic("collections");
  for (size_t i = 0; i < 10000; ++i) {
    /* No collection point occurs between allocation and initialization.
     * The object is deliberately discarded before the next allocation. */
    StellaValue garbage = stella_gc_alloc(&garbage_descriptor);
    stella_object_init_primitive(garbage, 0, i);
    if (statistic("collections") > before) {
      return;
    }
  }
  CHECK(0 && "no completed collection after 10000 allocations");
}

int main(void) {
  const StellaGcConfig config = {STELLA_C_GC_ABI_VERSION, 4096};
  stella_gc_init(&config);
  StellaValue root = NULL, second = NULL;
  stella_gc_push_root(&root);
  stella_gc_push_root(&second);

  root = stella_gc_alloc(&node_descriptor);
  stella_object_init_managed(root, 0, NULL);
  stella_object_init_primitive(root, 0, 111);
  second = stella_gc_alloc(&node_descriptor);
  stella_object_init_managed(second, 0, root);
  stella_object_init_primitive(second, 0, 222);
  stella_gc_write_managed(&root, 0, &second);

  /* Only root retains the cycle; second must be reached through its edge. */
  stella_gc_pop_root(&second);
  second = NULL;
  for (size_t round = 0; round < 3; ++round) {
    await_collection();
    CHECK(stella_object_read_primitive(root, 0) == 111);
    second = stella_gc_read_managed(&root, 0);
    stella_gc_push_root(&second);
    CHECK(second != NULL && second != root);
    CHECK(stella_object_read_primitive(second, 0) == 222);
    CHECK(stella_gc_read_managed(&second, 0) == root);
    stella_gc_pop_root(&second);
    second = NULL;
  }

  stella_gc_pop_root(&root);
  root = NULL;
  /* Finish any cycle already in progress, then a fresh cycle: previously
   * marked objects may legitimately survive the first one (floating garbage). */
  await_collection();
  await_collection();

  /* With no roots, only the last allocation made after sweep remains.
   * These checks distinguish reclaiming the cycle from merely reclaiming
   * temporary garbage. Never dereference the now-unreachable nodes. */
  CHECK(statistic("occupied bytes") ==
        sizeof(StellaObject) + sizeof(StellaSlot));
  CHECK(statistic("allocated objects") == statistic("reclaimed objects") + 1);
  CHECK(statistic("requested bytes") ==
        statistic("reclaimed bytes") + statistic("occupied bytes"));

  stella_gc_shutdown();
  return EXIT_SUCCESS;
}
