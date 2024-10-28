#ifndef   PURRR_H_
#define   PURRR_H_

#ifndef NOBE_H_
#error "Please include nobe.h before purrr.h"
#endif // NOBE_H_

Nobe_Target create_purrr_target(const char *path, bool debug, Nobe_Target *nob_target, Nobe_Target *glfw_target, Nobe_Target *vulkan_target) {
  Nobe_Target target = {
    .kind = NOBE_TARGET_KIND_LIBRARY,
    .name = "purrr",
    .path_name = "libpurrr.a",
    .path = "./src/",
  };

  const char *sources[] = {
    "internal.c",
    "vulkan.c",
    "purrr.c",
  };
  nob_da_append_many(&target.sources, sources, NOB_ARRAY_LEN(sources));

  Nobe_Target_Include includes[] = {
    (Nobe_Target_Include){
      .visibility = NOBE_VISIBILITY_PUBLIC,
      .path = "./include/",
    },
    (Nobe_Target_Include){
      .visibility = NOBE_VISIBILITY_PRIVATE,
      .path = "./src/",
    },
  };
  nob_da_append_many(&target.includes, includes, NOB_ARRAY_LEN(includes));

  Nobe_Target_Dependency dependencies[] = {
    (Nobe_Target_Dependency){
      .kind = NOBE_TARGET_DEPENDENCY_KIND_TARGET,
      .as.target = nob_target,
    },
    (Nobe_Target_Dependency){
      .kind = NOBE_TARGET_DEPENDENCY_KIND_TARGET,
      .as.target = glfw_target,
    },
    (Nobe_Target_Dependency){
      .kind = NOBE_TARGET_DEPENDENCY_KIND_TARGET,
      .as.target = vulkan_target,
    },
  };
  nob_da_append_many(&target.dependencies, dependencies, NOB_ARRAY_LEN(dependencies));

  const char *cflags[] = {
    "-Wall", "-Wextra", "-Werror", "-std=c99", "-pedantic"
  };
  nob_da_append_many(&target.cflags, cflags, NOB_ARRAY_LEN(cflags));
  if (debug) {
    nob_da_append(&target.cflags, "-ggdb");
    Nobe_Target_Define define = {
      .visibility = NOBE_VISIBILITY_PRIVATE,
      .name = "PURRR_DEBUG"
    };
    nob_da_append(&target.defines, define);
  }

  return target;
}

#endif // PURRR_H_