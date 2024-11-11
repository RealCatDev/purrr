#define NOB_IMPLEMENTATION
#include "./deps/nob/nob.h"

#define NOBE_COMPARE_DATES 1

#define NOBE_IMPLEMENTATION
#include "nobe.h"

#include "purrr.h"
#include "glfw.h"

const char *cflags[] = {
  "-Wall", "-Wextra", "-Werror", "-std=c99", "-pedantic", "-ggdb", "-Wswitch-enum"
};

Nobe_Target create_vulkan_target(const char *path) {
  Nobe_Target target = {
    .kind = NOBE_TARGET_KIND_INTERFACE,
    .name = "vulkan",
  };

  Nobe_Target_Dependency dep = {
    .kind = NOBE_TARGET_DEPENDENCY_KIND_LIBRARY,
    .as.library = "vulkan-1",
  };
  nob_da_append(&target.dependencies, dep);

  const char *include_path = nob_temp_sprintf("%s/Include/", path);

  Nobe_Target_Include include = {
    .visibility = NOBE_VISIBILITY_PUBLIC,
    .path = include_path,
  };
  nob_da_append(&target.includes, include);

  return target;
}

Nobe_Target create_example_target(Nobe_Target *purrr_target) {
  Nobe_Target target = {
    .kind = NOBE_TARGET_KIND_EXECUTABLE,
    .name = "example",
    .path_name = "example.exe",
    .path = "./example/",
  };

  const char *sources[] = {
    "main.c",
  };
  nob_da_append_many(&target.sources, sources, NOB_ARRAY_LEN(sources));

  Nobe_Target_Include include = {
    .visibility = NOBE_VISIBILITY_PUBLIC,
    .path = "./example/",
  };
  nob_da_append(&target.includes, include);

  Nobe_Target_Dependency dependency = {
    .kind = NOBE_TARGET_DEPENDENCY_KIND_TARGET,
    .as.target = purrr_target,
  };
  nob_da_append(&target.dependencies, dependency);

  nob_da_append_many(&target.cflags, cflags, NOB_ARRAY_LEN(cflags));

  return target;
}

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  const char *const program = nob_shift_args(&argc, &argv);

  nob_mkdir_if_not_exists("./build/");
  nob_mkdir_if_not_exists("./build/bin/");

  Nobe_Target glfw_target = create_glfw_target("./deps/glfw/");
  if (!nobe_target_build(&glfw_target)) return 1;

  Nobe_Target vulkan_target = create_vulkan_target(getenv("VULKAN_SDK"));

  bool debug = (argc == 1 && strcmp(nob_shift_args(&argc, &argv), "dbg") == 0);

  Nobe_Target purrr_target = create_purrr_target("./", debug, &glfw_target, &vulkan_target);
  if (!nobe_target_build(&purrr_target)) return 1;

  Nobe_Target example_target = create_example_target(&purrr_target);
  if (!nobe_target_build(&example_target)) return 1;

  return 0;
}