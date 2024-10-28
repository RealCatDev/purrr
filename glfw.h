#ifndef   GLFW_H_
#define   GLFW_H_

#ifndef NOBE_H_
#error "Please include nobe.h before purrr.h"
#endif // NOBE_H_

Nobe_Target create_glfw_target(const char *path) {
  Nobe_Target target = {
    .kind = NOBE_TARGET_KIND_LIBRARY,
    .name = "glfw",
    .path_name = "libglfw3.a",
    .path = nob_temp_sprintf("%s/src/", path),
  };

  const char *sources[] = {
    "context.c",
    "init.c",
    "input.c",
    "monitor.c",
    "platform.c",
    "vulkan.c",
    "window.c",
    "egl_context.c",
    "osmesa_context.c",
    "null_init.c",
    "null_monitor.c",
    "null_window.c",
    "null_joystick.c",
    #ifdef _WIN32
    "win32_init.c",
    "win32_joystick.c",
    "win32_module.c",
    "win32_monitor.c",
    "win32_thread.c",
    "win32_time.c",
    "win32_window.c",
    "wgl_context.c",
    #endif // _WIN32
  };
  nob_da_append_many(&target.sources, sources, NOB_ARRAY_LEN(sources));

  Nobe_Target_Include includes[] = {
    (Nobe_Target_Include){
      .visibility = NOBE_VISIBILITY_PUBLIC,
      .path = nob_temp_sprintf("%s/include/", path)
    },
    (Nobe_Target_Include){
      .visibility = NOBE_VISIBILITY_PRIVATE,
      .path = target.path
    },
    // (Nobe_Target_Include){
    //   .visibility = NOBE_VISIBILITY_PRIVATE,
    //   .path = "src/"
    // },
  };
  nob_da_append_many(&target.includes, includes, NOB_ARRAY_LEN(includes));

  #ifdef _WIN32
  Nobe_Target_Define win_defines[] = {
    (Nobe_Target_Define){
      .visibility = NOBE_VISIBILITY_PRIVATE,
      .name = "_GLFW_WIN32",
    },
    (Nobe_Target_Define){
      .visibility = NOBE_VISIBILITY_PRIVATE,
      .name = "UNICODE",
    },
    (Nobe_Target_Define){
      .visibility = NOBE_VISIBILITY_PRIVATE,
      .name = "_UNICODE",
    },
    (Nobe_Target_Define){
      .visibility = NOBE_VISIBILITY_PRIVATE,
      .name = "WINVER",
      .def = "0x0501",
    },
  };
  nob_da_append_many(&target.defines, win_defines, NOB_ARRAY_LEN(win_defines));

  Nobe_Target_Dependency dependency = {
    .kind = NOBE_TARGET_DEPENDENCY_KIND_LIBRARY,
    .as.library = "gdi32",
  };
  nob_da_append(&target.dependencies, dependency);
  #endif // _WIN32

  return target;
}

#endif // GLFW_H_