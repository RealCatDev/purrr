/* nobe - Public Domain - https://github.com/RealCatDev/nobe
  NOB Extension.
*/

#ifndef   NOBE_H_
#define   NOBE_H_

#ifndef   NOB_H_
#  error "Please include nob.h before nobe.h"
#endif // NOB_H_

#ifndef NOBE_BUILD_PATH
#  define NOBE_BUILD_PATH "./build/"
#endif // NOBE_BUILD_PATH

#ifndef NOBE_BIN_PATH
#  define NOBE_BIN_PATH "./build/bin/"
#endif // NOBE_BIN_PATH

#ifndef NOBE_COMPILER
#  define NOBE_COMPILER "gcc"
#endif // NOBE_COMPILER

#ifndef NOBE_COMPARE_DATES
#  define NOBE_COMPARE_DATES 1
#endif // NOBE_COMPARE_DATES

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef enum {
  NOBE_VISIBILITY_PUBLIC = 0,
  NOBE_VISIBILITY_PRIVATE,
  COUNT_NOBE_VISIBILITIES
} Nobe_Visibility;

typedef struct {
  Nobe_Visibility visibility;
  const char *path;
} Nobe_Target_Include;

typedef struct {
  Nobe_Visibility visibility;
  const char *name;
  const char *def;
} Nobe_Target_Define;

typedef struct Nobe_Target Nobe_Target;
typedef enum {
  NOBE_TARGET_DEPENDENCY_KIND_TARGET = 0,
  NOBE_TARGET_DEPENDENCY_KIND_LIBRARY,
  COUNT_NOBE_TARGET_DEPENDENCY_KINDS
} Nobe_Target_Dependency_Kind;

typedef struct {
  Nobe_Target_Dependency_Kind kind;
  union {
    const Nobe_Target *target;
    const char *library;
  } as;
} Nobe_Target_Dependency;

typedef enum {
  NOBE_TARGET_KIND_EXECUTABLE = 0,
  NOBE_TARGET_KIND_LIBRARY,
  NOBE_TARGET_KIND_INTERFACE,
  COUNT_NOBE_TARGET_KINDS
} Nobe_Target_Kind;

struct Nobe_Target {
  Nobe_Target_Kind kind;
  const char *name;
  const char *path_name;
  const char *path;

  struct {
    const char **items;
    size_t capacity;
    size_t count;
  } sources;

  struct {
    Nobe_Target_Include *items;
    size_t capacity;
    size_t count;
  } includes;

  struct {
    Nobe_Target_Dependency *items;
    size_t capacity;
    size_t count;
  } dependencies;

  struct {
    Nobe_Target_Define *items;
    size_t capacity;
    size_t count;
  } defines;

  struct {
    const char **items;
    size_t capacity;
    size_t count;
  } cflags;

  struct {
    const char **items;
    size_t capacity;
    size_t count;
  } ldflags;

  #if NOBE_COMPARE_DATES
  bool rebuilt;
  #endif // NOBE_COMPARE_DATES
};

bool nobe_target_build(Nobe_Target *target);

#ifdef NOBE_IMPLEMENTATION

#ifndef NOB_IMPLEMENTATION
#  error "Please define NOB_IMPLEMENTATION before including nob"
#endif // NOB_IMPLEMENTATION

typedef struct {
  char **items;
  size_t capacity;
  size_t count;
} Nobe_Impl_Strs;

bool nobe_impl_strs_has(Nobe_Impl_Strs strs, char *v) {
  for (size_t i = 0; i < strs.count; ++i)
    if (strs.items[i] == v || (strcmp(strs.items[i], v) == 0)) return true;
  return false;
}

bool nobe_impl_targets_collect_includes(const Nobe_Target *target, Nobe_Impl_Strs *includes, bool root) {
  if (!includes ||
      !target) return false;
  for (size_t i = 0; i < target->includes.count; ++i) {
    Nobe_Target_Include include = target->includes.items[i];
    // first target is this so we ignore visibility
    if ((!root && include.visibility != NOBE_VISIBILITY_PUBLIC) || !include.path) continue;
    Nob_String_Builder sb = {0};
    nob_sb_append_cstr(&sb, "-I");
    nob_sb_append_cstr(&sb, include.path);
    nob_sb_append_null(&sb);
    if (!nobe_impl_strs_has(*includes, sb.items)) nob_da_append(includes, sb.items);
    else nob_sb_free(sb);
  }
  for (size_t i = 0; i < target->dependencies.count; ++i) {
    Nobe_Target_Dependency dep = target->dependencies.items[i];
    if (dep.kind == NOBE_TARGET_DEPENDENCY_KIND_TARGET) {
      if (!nobe_impl_targets_collect_includes(dep.as.target, includes, false)) return false;
    } else if (dep.kind == NOBE_TARGET_DEPENDENCY_KIND_LIBRARY) continue;
    else return false;
  }
  return true;
}

bool nobe_impl_targets_collect_libraries(const Nobe_Target *target, Nobe_Impl_Strs *libraries) {
  if (!libraries ||
      !target) return false;
  if (target->kind == NOBE_TARGET_KIND_LIBRARY) {
    Nob_String_Builder sb = {0};
    nob_sb_append_cstr(&sb, "-l:");
    nob_sb_append_cstr(&sb, target->path_name);
    nob_sb_append_null(&sb);
    if (!nobe_impl_strs_has(*libraries, sb.items)) nob_da_append(libraries, sb.items);
    else nob_sb_free(sb);
  }
  for (size_t i = 0; i < target->dependencies.count; ++i) {
    Nobe_Target_Dependency dep = target->dependencies.items[i];
    if (dep.kind == NOBE_TARGET_DEPENDENCY_KIND_TARGET) {
      if (!nobe_impl_targets_collect_libraries(dep.as.target, libraries)) return false;
    } else if (dep.kind == NOBE_TARGET_DEPENDENCY_KIND_LIBRARY) {
      Nob_String_Builder sb = {0};
      nob_sb_append_cstr(&sb, "-l");
      nob_sb_append_cstr(&sb, dep.as.library);
      nob_sb_append_null(&sb);
      if (!nobe_impl_strs_has(*libraries, sb.items)) nob_da_append(libraries, sb.items);
      else nob_sb_free(sb);
    } else return false;
  }
  return true;
}

bool nobe_target_build(Nobe_Target *target) {
  if (!target->name ||
      target->kind >= COUNT_NOBE_TARGET_KINDS)
    return false;
  if (target->kind == NOBE_TARGET_KIND_INTERFACE) return true;

  if (!target->sources.items ||
      target->sources.count == 0 ||
      !target->path_name) return false;

  Nobe_Impl_Strs actual_sources = {0};
  {
    Nob_String_Builder sb = {0};
    nob_sb_append_cstr(&sb, target->path);
    size_t sb_count = sb.count;
    for (size_t i = 0; i < target->sources.count; ++i) {
      sb.count = sb_count;
      nob_sb_append_cstr(&sb, target->sources.items[i]);
      nob_sb_append_null(&sb);
      nob_da_append(&actual_sources, strdup(sb.items));
    }
    nob_sb_free(sb);
  }

  size_t temp_saved = nob_temp_save();
  char *output = nob_temp_sprintf("%s%s", NOBE_BUILD_PATH, target->path_name);
  #if NOBE_COMPARE_DATES
  bool check = true;
  for (size_t i = 0; i < target->dependencies.count; ++i) {
    if (target->dependencies.items[i].kind != NOBE_TARGET_DEPENDENCY_KIND_TARGET) continue;
    if (target->dependencies.items[i].as.target->rebuilt) {
      check = false;
      break;
    }
  }
  if (check && nob_needs_rebuild(output, (const char **)actual_sources.items, actual_sources.count) != 1) return true;
  target->rebuilt = true;
  #endif // NOBE_COMPARE_DATES

  Nobe_Impl_Strs includes = {0};
  if (!nobe_impl_targets_collect_includes(target, &includes, true)) return false;

  Nob_Cmd cmd = {0};
  if (target->kind == NOBE_TARGET_KIND_EXECUTABLE) {
    nob_cmd_append(&cmd, NOBE_COMPILER);
    nob_da_append_many(&cmd, target->ldflags.items, target->ldflags.count);
    nob_cmd_append(&cmd, "-o");
  } else nob_cmd_append(&cmd, "ar", "rcs");

  nob_mkdir_if_not_exists(nob_temp_sprintf("%s%s", NOBE_BIN_PATH, target->name));

  nob_cmd_append(&cmd, output);

  Nobe_Impl_Strs defines = {0};
  for (size_t i = 0; i < target->defines.count; ++i) {
    Nobe_Target_Define define = target->defines.items[i];
    Nob_String_Builder sb = {0};
    nob_sb_append_cstr(&sb, "-D");
    nob_sb_append_cstr(&sb, define.name);
    if (define.def) {
      nob_sb_append_cstr(&sb, "=");
      nob_sb_append_cstr(&sb, define.def);
    }
    nob_sb_append_null(&sb);
    nob_da_append(&defines, sb.items);
  }

  bool result = true;
  for (size_t i = 0; i < target->sources.count; ++i) {
    char *obj = nob_temp_sprintf("%s/%s/%s.o", NOBE_BIN_PATH, target->name, target->sources.items[i]);
    nob_cmd_append(&cmd, obj);

    Nob_Cmd c_cmd = {0};
    nob_cmd_append(&c_cmd, NOBE_COMPILER);
    nob_da_append_many(&c_cmd, target->cflags.items, target->cflags.count);
    nob_da_append_many(&c_cmd, defines.items, defines.count);
    nob_cmd_append(&c_cmd, "-c", "-o", (const char*)obj, (const char *)actual_sources.items[i]);
    nob_da_append_many(&c_cmd, includes.items, includes.count);
    bool cmd_success = nob_cmd_run_sync(c_cmd);
    nob_cmd_free(c_cmd);
    if (!cmd_success) nob_return_defer(false);
  }

  if (target->kind == NOBE_TARGET_KIND_EXECUTABLE) {
    nob_cmd_append(&cmd, nob_temp_sprintf("-L%s", NOBE_BUILD_PATH));
    Nobe_Impl_Strs libraries = {0};
    if (!nobe_impl_targets_collect_libraries(target, &libraries)) nob_return_defer(false);
    nob_da_append_many(&cmd, libraries.items, libraries.count);
  }

  if (!nob_cmd_run_sync(cmd)) nob_return_defer(false);

defer:
  nob_temp_rewind(temp_saved);
  nob_cmd_free(cmd);

  for (size_t i = 0; i < actual_sources.count; ++i)
    free(actual_sources.items[i]);
  free(actual_sources.items);

  return result;
}

#endif // NOBE_IMPLEMENTATION

#endif // NOBE_H_