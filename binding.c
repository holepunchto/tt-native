#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdbool.h>
#include <stdlib.h>
#include <tt.h>
#include <utf.h>
#include <uv.h>

typedef struct {
  tt_pty_t pty;

  uv_buf_t reading;

  js_env_t *env;
  js_ref_t *ctx;

  js_ref_t *on_read;
  js_ref_t *on_exit;

  bool exited;
  bool exiting;

  js_deferred_teardown_t *teardown;
} tt_js_pty_t;

typedef struct {
  tt_pty_write_t req;

  uv_buf_t writing;

  tt_js_pty_t *pty;

  js_env_t *env;
  js_ref_t *ctx;

  js_ref_t *on_write;
} tt_js_pty_write_t;

static js_value_t *
tt_js__create_error_or_null(js_env_t *env, int status) {
  int err;

  js_value_t *result;

  if (status < 0) {
    js_value_t *code;
    err = js_create_string_utf8(env, (utf8_t *) uv_err_name(status), -1, &code);
    assert(err == 0);

    js_value_t *message;
    err = js_create_string_utf8(env, (utf8_t *) uv_strerror(status), -1, &message);
    assert(err == 0);

    err = js_create_error(env, code, message, &result);
    assert(err == 0);
  } else {
    err = js_get_null(env, &result);
    assert(err == 0);
  }

  return result;
}

static char *
tt_js__get_string(js_env_t *env, js_value_t *value) {
  int err;

  size_t len;
  err = js_get_value_string_utf8(env, value, NULL, 0, &len);
  assert(err == 0);

  utf8_t *str = calloc(len + 1, sizeof(utf8_t));

  err = js_get_value_string_utf8(env, value, str, len + 1, NULL);
  assert(err == 0);

  return (char *) str;
}

static char **
tt_js__get_string_array(js_env_t *env, js_value_t *value, uint32_t *result) {
  int err;

  uint32_t len;
  err = js_get_array_length(env, value, &len);
  assert(err == 0);

  char **strs = calloc(len + 1, sizeof(char *));

  for (uint32_t i = 0; i < len; i++) {
    js_value_t *element;
    err = js_get_element(env, value, i, &element);
    assert(err == 0);

    strs[i] = tt_js__get_string(env, element);
  }

  *result = len;

  return strs;
}

static void
tt_js__free_string_array(char **strs, uint32_t len) {
  if (strs == NULL) return;

  for (uint32_t i = 0; i < len; i++) {
    free(strs[i]);
  }

  free(strs);
}

static bool
tt_js__is_null(js_env_t *env, js_value_t *value) {
  int err;

  bool is_null;
  err = js_is_null(env, value, &is_null);
  assert(err == 0);

  return is_null;
}

static void
tt_js__on_close(tt_pty_t *handle) {
  int err;

  tt_js_pty_t *pty = (tt_js_pty_t *) handle;

  js_env_t *env = pty->env;

  err = js_delete_reference(env, pty->on_read);
  assert(err == 0);

  err = js_delete_reference(env, pty->on_exit);
  assert(err == 0);

  err = js_delete_reference(env, pty->ctx);
  assert(err == 0);

  err = js_finish_deferred_teardown_callback(pty->teardown);
  assert(err == 0);
}

static void
tt_js__on_exit(tt_pty_t *handle, int64_t exit_status, int term_signal) {
  int err;

  tt_js_pty_t *pty = (tt_js_pty_t *) handle;

  pty->exited = true;

  if (pty->exiting) {
    tt_pty_close(handle, tt_js__on_close);

    return;
  }

  js_env_t *env = pty->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *ctx;
  err = js_get_reference_value(env, pty->ctx, &ctx);
  assert(err == 0);

  js_value_t *on_exit;
  err = js_get_reference_value(env, pty->on_exit, &on_exit);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_int64(env, exit_status, &argv[0]);
  assert(err == 0);

  err = js_create_int32(env, term_signal, &argv[1]);
  assert(err == 0);

  js_call_function_with_checkpoint(env, ctx, on_exit, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  tt_pty_close(handle, tt_js__on_close);
}

static void
tt_js__on_read(tt_pty_t *handle, ssize_t read_len, const uv_buf_t *buf) {
  int err;

  if (read_len == UV_EOF || read_len == 0) return;

  tt_js_pty_t *pty = (tt_js_pty_t *) handle;

  if (pty->exiting) return;

  js_env_t *env = pty->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *ctx;
  err = js_get_reference_value(env, pty->ctx, &ctx);
  assert(err == 0);

  js_value_t *on_read;
  err = js_get_reference_value(env, pty->on_read, &on_read);
  assert(err == 0);

  js_value_t *argv[2];

  argv[0] = tt_js__create_error_or_null(env, (int) read_len);

  err = js_create_uint32(env, read_len < 0 ? 0 : (uint32_t) read_len, &argv[1]);
  assert(err == 0);

  js_value_t *result;
  err = js_call_function_with_checkpoint(env, ctx, on_read, 2, argv, &result);

  if (err == 0) {
    void *data;
    size_t len;
    err = js_get_typedarray_info(env, result, NULL, &data, &len, NULL, NULL);
    assert(err == 0);

    pty->reading = uv_buf_init(data, (unsigned int) len);
  }

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
tt_js__on_alloc(tt_pty_t *handle, size_t suggested_size, uv_buf_t *buf) {
  tt_js_pty_t *pty = (tt_js_pty_t *) handle;

  *buf = pty->reading;
}

static void
tt_js__on_write(tt_pty_write_t *handle, int status) {
  int err;

  tt_js_pty_write_t *req = (tt_js_pty_write_t *) handle;

  js_env_t *env = req->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *ctx;
  err = js_get_reference_value(env, req->ctx, &ctx);
  assert(err == 0);

  js_value_t *on_write;
  err = js_get_reference_value(env, req->on_write, &on_write);
  assert(err == 0);

  err = js_delete_reference(env, req->on_write);
  assert(err == 0);

  err = js_delete_reference(env, req->ctx);
  assert(err == 0);

  if (!req->pty->exiting) {
    js_value_t *argv[1] = {tt_js__create_error_or_null(env, status)};

    js_call_function_with_checkpoint(env, ctx, on_write, 1, argv, NULL);
  }

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
tt_js__on_teardown(js_deferred_teardown_t *handle, void *data) {
  int err;

  tt_js_pty_t *pty = (tt_js_pty_t *) data;

  pty->exiting = true;

  if (pty->exited) return;

  // Interactive shells ignore SIGTERM, so only SIGKILL guarantees that the
  // process exits and teardown completes.
  err = tt_pty_kill(&pty->pty, SIGKILL);
  assert(err == 0 || err == UV_ESRCH);
}

static js_value_t *
tt_js_pty_spawn(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 10;
  js_value_t *argv[10];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 10);

  tt_js_pty_t *pty;
  err = js_get_typedarray_info(env, argv[0], NULL, (void **) &pty, NULL, NULL, NULL);
  assert(err == 0);

  uint32_t width;
  err = js_get_value_uint32(env, argv[1], &width);
  assert(err == 0);

  uint32_t height;
  err = js_get_value_uint32(env, argv[2], &height);
  assert(err == 0);

  char *file = tt_js__get_string(env, argv[3]);

  uint32_t args_len;
  char **args = tt_js__get_string_array(env, argv[4], &args_len);

  uint32_t pairs_len = 0;
  char **pairs = NULL;

  if (!tt_js__is_null(env, argv[5])) {
    pairs = tt_js__get_string_array(env, argv[5], &pairs_len);
  }

  char *cwd = NULL;

  if (!tt_js__is_null(env, argv[6])) {
    cwd = tt_js__get_string(env, argv[6]);
  }

  uv_loop_t *loop;
  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  tt_term_options_t term = {
    .width = (int) width,
    .height = (int) height,
  };

  tt_process_options_t process = {
    .file = file,
    .args = args,
    .cwd = cwd,
    .env = pairs,
  };

  int status = tt_pty_spawn(loop, &pty->pty, &term, &process, tt_js__on_exit);

  free(file);
  free(cwd);

  tt_js__free_string_array(args, args_len);
  tt_js__free_string_array(pairs, pairs_len);

  if (status < 0) {
    err = js_throw_error(env, uv_err_name(status), uv_strerror(status));
    assert(err == 0);

    return NULL;
  }

  pty->env = env;
  pty->exited = false;
  pty->exiting = false;

  err = js_create_reference(env, argv[7], 1, &pty->ctx);
  assert(err == 0);

  err = js_create_reference(env, argv[8], 1, &pty->on_read);
  assert(err == 0);

  err = js_create_reference(env, argv[9], 1, &pty->on_exit);
  assert(err == 0);

  err = js_add_deferred_teardown_callback(env, tt_js__on_teardown, (void *) pty, &pty->teardown);
  assert(err == 0);

  js_value_t *result;
  err = js_create_uint32(env, (uint32_t) pty->pty.pid, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
tt_js_pty_read(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 2);

  tt_js_pty_t *pty;
  err = js_get_typedarray_info(env, argv[0], NULL, (void **) &pty, NULL, NULL, NULL);
  assert(err == 0);

  void *data;
  size_t len;
  err = js_get_typedarray_info(env, argv[1], NULL, &data, &len, NULL, NULL);
  assert(err == 0);

  pty->reading = uv_buf_init(data, (unsigned int) len);

  int status = tt_pty_read_start(&pty->pty, tt_js__on_alloc, tt_js__on_read);

  if (status < 0) {
    err = js_throw_error(env, uv_err_name(status), uv_strerror(status));
    assert(err == 0);
  }

  return NULL;
}

static js_value_t *
tt_js_pty_write(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 5;
  js_value_t *argv[5];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 5);

  tt_js_pty_t *pty;
  err = js_get_typedarray_info(env, argv[0], NULL, (void **) &pty, NULL, NULL, NULL);
  assert(err == 0);

  tt_js_pty_write_t *req;
  err = js_get_typedarray_info(env, argv[1], NULL, (void **) &req, NULL, NULL, NULL);
  assert(err == 0);

  void *data;
  size_t len;
  err = js_get_typedarray_info(env, argv[2], NULL, &data, &len, NULL, NULL);
  assert(err == 0);

  req->writing = uv_buf_init(data, (unsigned int) len);

  int status = tt_pty_write(&req->req, &pty->pty, &req->writing, 1, tt_js__on_write);

  if (status < 0) {
    err = js_throw_error(env, uv_err_name(status), uv_strerror(status));
    assert(err == 0);

    return NULL;
  }

  req->pty = pty;
  req->env = env;

  err = js_create_reference(env, argv[3], 1, &req->ctx);
  assert(err == 0);

  err = js_create_reference(env, argv[4], 1, &req->on_write);
  assert(err == 0);

  return NULL;
}

static js_value_t *
tt_js_pty_resize(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 3;
  js_value_t *argv[3];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 3);

  tt_js_pty_t *pty;
  err = js_get_typedarray_info(env, argv[0], NULL, (void **) &pty, NULL, NULL, NULL);
  assert(err == 0);

  uint32_t width;
  err = js_get_value_uint32(env, argv[1], &width);
  assert(err == 0);

  uint32_t height;
  err = js_get_value_uint32(env, argv[2], &height);
  assert(err == 0);

  int status = tt_pty_resize(&pty->pty, (int) width, (int) height);

  if (status < 0) {
    err = js_throw_error(env, uv_err_name(status), uv_strerror(status));
    assert(err == 0);
  }

  return NULL;
}

static js_value_t *
tt_js_pty_kill(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 2);

  tt_js_pty_t *pty;
  err = js_get_typedarray_info(env, argv[0], NULL, (void **) &pty, NULL, NULL, NULL);
  assert(err == 0);

  int32_t signum;
  err = js_get_value_int32(env, argv[1], &signum);
  assert(err == 0);

  int status = tt_pty_kill(&pty->pty, signum);

  if (status < 0) {
    err = js_throw_error(env, uv_err_name(status), uv_strerror(status));
    assert(err == 0);
  }

  return NULL;
}

static js_value_t *
tt_js_pty_get_name(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 1);

  tt_js_pty_t *pty;
  err = js_get_typedarray_info(env, argv[0], NULL, (void **) &pty, NULL, NULL, NULL);
  assert(err == 0);

  size_t len = 4096;
  char name[len];

  err = tt_pty_get_name(&pty->pty, name, len);
  assert(err == 0);

  js_value_t *result;
  err = js_create_string_utf8(env, (utf8_t *) name, -1, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
tt_js_exports(js_env_t *env, js_value_t *exports) {
  int err;

#define V(name, n) \
  { \
    js_value_t *val; \
    err = js_create_uint32(env, n, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }

  V("SIGINT", SIGINT)
  V("SIGKILL", SIGKILL)
  V("SIGTERM", SIGTERM)

  V("sizeofPTY", sizeof(tt_js_pty_t))
  V("sizeofWrite", sizeof(tt_js_pty_write_t))
#undef V

#define V(name, fn) \
  { \
    js_value_t *val; \
    err = js_create_function(env, name, -1, fn, NULL, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }

  V("spawn", tt_js_pty_spawn)
  V("read", tt_js_pty_read)
  V("write", tt_js_pty_write)
  V("resize", tt_js_pty_resize)
  V("kill", tt_js_pty_kill)
  V("getName", tt_js_pty_get_name)
#undef V

  return exports;
}

BARE_MODULE(tt_js, tt_js_exports)
