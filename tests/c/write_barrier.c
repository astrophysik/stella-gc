#include <stella/gc.h>
#include <string.h>
#include "check.h"

/* Algorithm-specific test: the current non-moving collector encodes white,
 * grey and black as 0, 1 and 2. GC_MARK_STEPS=5 and GC_HEAP_THRESHOLD=0.75
 * (the defaults) leave the chain below partially scanned between allocations.
 * We inspect colors but never modify collector metadata. */
enum { WHITE = 0, GREY = 1, BLACK = 2, CHAIN_LENGTH = 64 };

static const StellaObjectDescriptor node = {
  .abi_version = STELLA_C_GC_ABI_VERSION, .kind = STELLA_OBJECT_DATA,
  .managed_count = 1, .primitive_count = 1, .slot_capacity = 2,
  .debug_name = "barrier test node"
};
static const StellaObjectDescriptor leaf = {
  .abi_version = STELLA_C_GC_ABI_VERSION, .kind = STELLA_OBJECT_DATA,
  .primitive_count = 1, .slot_capacity = 1,
  .debug_name = "barrier test leaf"
};

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

static void allocate_garbage(void) {
  StellaValue garbage = stella_gc_alloc(&leaf);
  stella_object_init_primitive(garbage, 0, 999);
}

int main(void) {
  const StellaGcConfig config = {STELLA_C_GC_ABI_VERSION, 16384};
  stella_gc_init(&config);
  StellaValue chain = NULL, temporary = NULL, target = NULL, owner = NULL;
  stella_gc_push_root(&chain);
  stella_gc_push_root(&temporary);
  stella_gc_push_root(&target);
  /* Registered last, owner is visited first by the collector's LIFO stack. */
  stella_gc_push_root(&owner);

  temporary = stella_gc_alloc(&leaf);
  stella_object_init_primitive(temporary, 0, 777);
  target = stella_gc_alloc(&node);
  stella_object_init_managed(target, 0, temporary);
  stella_object_init_primitive(target, 0, 222);
  chain = target;
  /* Observation pointers are deliberately NOT roots. They stay valid because
   * the non-moving collector must preserve the rooted chain until we cut it. */
  StellaValue expected_target = target;
  StellaValue predecessor = NULL;
  for (size_t i = 0; i < CHAIN_LENGTH; ++i) {
    temporary = stella_gc_alloc(&node);
    stella_object_init_managed(temporary, 0, chain);
    stella_object_init_primitive(temporary, 0, i);
    if (i == 0) predecessor = temporary;
    chain = temporary;
  }
  owner = stella_gc_alloc(&node);
  stella_object_init_managed(owner, 0, NULL);
  stella_object_init_primitive(owner, 0, 111);
  target = temporary = NULL;
  CHECK(statistic("collections") == 0);

  size_t attempts = 0;
  while (owner->gc_word != BLACK && attempts < 2000) {
    allocate_garbage();
    ++attempts;
  }
  CHECK(owner->gc_word == BLACK);
  CHECK(expected_target->gc_word == WHITE);
  CHECK(predecessor->gc_word != BLACK);

  size_t before = statistic("write barrier activations");
  size_t completed = statistic("collections");
  target = expected_target;
  stella_gc_write_managed(&owner, 0, &target);
  CHECK(target->gc_word == GREY);
  CHECK(owner->gc_word == BLACK);
  CHECK(statistic("write barrier activations") == before + 1);

  /* Cut the old path, including for ancestors already queued by marking.
   * Then clear ALL temporary roots to B: the final root scan must not rescue
   * it and thereby conceal a broken write barrier. */
  temporary = predecessor;
  target = NULL;
  stella_gc_write_managed(&temporary, 0, &target);
  CHECK(statistic("write barrier activations") == before + 1);
  chain = temporary = NULL;
  expected_target = predecessor = NULL;

  attempts = 0;
  while (statistic("collections") == completed && attempts < 2000) {
    allocate_garbage();
    ++attempts;
  }
  CHECK(statistic("collections") > completed);
  CHECK(stella_object_read_primitive(owner, 0) == 111);
  target = stella_gc_read_managed(&owner, 0);
  CHECK(target != NULL);
  CHECK(stella_object_read_primitive(target, 0) == 222);
  temporary = stella_gc_read_managed(&target, 0);
  CHECK(temporary != NULL);
  CHECK(stella_object_read_primitive(temporary, 0) == 777);

  stella_gc_pop_root(&owner);
  stella_gc_pop_root(&target);
  stella_gc_pop_root(&temporary);
  stella_gc_pop_root(&chain);
  stella_gc_shutdown();
  return EXIT_SUCCESS;
}
