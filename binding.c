#include <napi-macros.h>
#include <node_api.h>
#include <stdlib.h>
#include <tt.h>
#include <uv.h>

#define TT_NAPI_MAKE_ERROR(status, err) \
  { \
    napi_value code; \
    napi_value msg; \
    napi_create_string_utf8(env, uv_err_name(status), NAPI_AUTO_LENGTH, &code); \
    napi_create_string_utf8(env, uv_strerror(status), NAPI_AUTO_LENGTH, &msg); \
    napi_create_error(env, code, msg, err); \
  }

#define TT_NAPI_MAKE_ERROR_OR_NULL(status, err) \
  { \
    if (status < 0) { \
      TT_NAPI_MAKE_ERROR(status, err); \
    } else { \
      napi_get_null(env, err); \
    } \
  }

#define TT_NAPI_THROW_ERROR(status) \
  { \
    napi_value res; \
    TT_NAPI_MAKE_ERROR(status, &res); \
    napi_throw(env, res); \
    return NULL; \
  }

#define TT_NAPI_CALLBACK(self, fn, src) \
  { \
    napi_handle_scope scope; \
    napi_open_handle_scope(env, &scope); \
    napi_value ctx; \
    napi_get_reference_value(env, self->ctx, &ctx); \
    napi_value callback; \
    napi_get_reference_value(env, fn, &callback); \
    src; \
    napi_close_handle_scope(env, scope); \
  }

typedef struct {
  tt_pty_t pty;

  uv_buf_t reading;

  napi_env env;
  napi_ref ctx;

  napi_ref on_read;
  napi_ref on_exit;
} tt_napi_pty_t;

typedef struct {
  tt_pty_write_t req;

  uv_buf_t writing;

  napi_env env;
  napi_ref ctx;

  napi_ref on_write;
} tt_napi_pty_write_t;

static void
on_close (tt_pty_t *handle) {
  tt_napi_pty_t *pty = (tt_napi_pty_t *) handle;

  napi_env env = pty->env;

  napi_delete_reference(env, pty->on_read);
  napi_delete_reference(env, pty->on_exit);
  napi_delete_reference(env, pty->ctx);
}

static void
on_process_exit (tt_pty_t *handle, int64_t exit_status, int term_signal) {
  tt_napi_pty_t *pty = (tt_napi_pty_t *) handle;

  napi_env env = pty->env;

  TT_NAPI_CALLBACK(pty, pty->on_exit, {
    napi_value argv[2];
    napi_create_int64(env, exit_status, &argv[0]);
    napi_create_int32(env, term_signal, &argv[1]);
    NAPI_MAKE_CALLBACK(env, NULL, ctx, callback, 2, argv, NULL);
  });

  tt_pty_close(handle, on_close);
}

NAPI_METHOD(tt_napi_pty_spawn) {
  NAPI_ARGV(10)
  NAPI_ARGV_BUFFER_CAST(tt_napi_pty_t *, handle, 0)
  NAPI_ARGV_UINT32(width, 1)
  NAPI_ARGV_UINT32(height, 2)
  NAPI_ARGV_UTF8_MALLOC(file, 3)

  handle->env = env;

  napi_create_reference(env, argv[7], 1, &(handle->ctx));
  napi_create_reference(env, argv[8], 1, &(handle->on_read));
  napi_create_reference(env, argv[9], 1, &(handle->on_exit));

  char **args = NULL;
  uint32_t args_len = 0;

  napi_get_array_length(env, argv[4], &args_len);

  args = calloc(args_len + 1, sizeof(char *));

  for (size_t i = 0; i < args_len; i++) {
    napi_value value;
    napi_get_element(env, argv[4], i, &value);

    size_t arg_len;
    napi_get_value_string_utf8(env, value, NULL, 0, &arg_len);

    char *arg = calloc(arg_len + 1, sizeof(char));
    napi_get_value_string_utf8(env, value, arg, arg_len + 1, &arg_len);

    args[i] = arg;
  }

  char **pairs = NULL;
  uint32_t pairs_len = 0;

  napi_valuetype pairs_type;
  napi_typeof(env, argv[5], &pairs_type);

  if (pairs_type != napi_null) {
    napi_get_array_length(env, argv[5], &pairs_len);

    pairs = calloc(pairs_len + 1, sizeof(char *));

    for (size_t i = 0; i < pairs_len; i++) {
      napi_value value;
      napi_get_element(env, argv[5], i, &value);

      size_t pair_len;
      napi_get_value_string_utf8(env, value, NULL, 0, &pair_len);

      char *pair = calloc(pair_len + 1, sizeof(char));
      napi_get_value_string_utf8(env, value, pair, pair_len + 1, &pair_len);

      pairs[i] = pair;
    }
  }

  char *cwd = NULL;
  size_t cwd_len = 0;

  napi_valuetype cwd_type;
  napi_typeof(env, argv[6], &cwd_type);

  if (cwd_type != napi_null) {
    napi_get_value_string_utf8(env, argv[6], NULL, 0, &cwd_len);

    cwd = calloc(cwd_len + 1, sizeof(char *));
    napi_get_value_string_utf8(env, argv[6], cwd, cwd_len + 1, &cwd_len);
  }

  uv_loop_t *loop;
  napi_get_uv_event_loop(env, &loop);

  tt_pty_t *pty = &handle->pty;

  tt_term_options_t term = {
    .width = width,
    .height = height,
  };

  tt_process_options_t process = {
    .file = file,
    .args = args,
    .cwd = cwd,
    .env = pairs,
  };

  int err = tt_pty_spawn(loop, pty, &term, &process, on_process_exit);

  free(file);

  for (size_t i = 0; i < args_len; i++) {
    free(args[i]);
  }

  for (size_t i = 0; i < pairs_len; i++) {
    free(pairs[i]);
  }

  if (args) free(args);
  if (pairs) free(pairs);
  if (cwd) free(cwd);

  if (err < 0) TT_NAPI_THROW_ERROR(err);

  NAPI_RETURN_UINT32(pty->pid)
}

static void
on_read (tt_pty_t *handle, ssize_t read_len, const uv_buf_t *buf) {
  if (read_len == UV_EOF) return;

  tt_napi_pty_t *pty = (tt_napi_pty_t *) handle;

  napi_env env = pty->env;

  TT_NAPI_CALLBACK(pty, pty->on_read, {
    napi_value result;
    napi_value argv[2];
    TT_NAPI_MAKE_ERROR_OR_NULL(read_len, &argv[0]);
    napi_create_uint32(env, read_len < 0 ? 0 : read_len, &argv[1]);
    NAPI_MAKE_CALLBACK(env, NULL, ctx, callback, 2, argv, &result);

    char *next;
    size_t next_len;
    napi_get_buffer_info(env, result, (void **) &next, &next_len);

    pty->reading = uv_buf_init(next, next_len);
  });
}

static void
on_alloc (tt_pty_t *handle, size_t suggested_size, uv_buf_t *buf) {
  tt_napi_pty_t *pty = (tt_napi_pty_t *) handle;

  *buf = pty->reading;
}

NAPI_METHOD(tt_napi_pty_read) {
  NAPI_ARGV(2)
  NAPI_ARGV_BUFFER_CAST(tt_napi_pty_t *, handle, 0)
  NAPI_ARGV_BUFFER(buf, 1)

  handle->reading = uv_buf_init(buf, buf_len);

  tt_pty_t *pty = &handle->pty;

  int err = tt_pty_read_start(pty, on_alloc, on_read);

  if (err < 0) TT_NAPI_THROW_ERROR(err);

  return NULL;
}

static void
on_write (tt_pty_write_t *req, int status) {
  tt_napi_pty_write_t *r = (tt_napi_pty_write_t *) req;

  napi_env env = r->env;

  TT_NAPI_CALLBACK(r, r->on_write, {
    napi_value argv[1];
    TT_NAPI_MAKE_ERROR_OR_NULL(status, &argv[0]);
    NAPI_MAKE_CALLBACK(env, NULL, ctx, callback, 1, argv, NULL);
  });

  napi_delete_reference(env, r->on_write);
  napi_delete_reference(env, r->ctx);
}

NAPI_METHOD(tt_napi_pty_write) {
  NAPI_ARGV(5)
  NAPI_ARGV_BUFFER_CAST(tt_napi_pty_t *, handle, 0)
  NAPI_ARGV_BUFFER_CAST(tt_napi_pty_write_t *, req, 1)
  NAPI_ARGV_BUFFER(buf, 2)

  req->env = env;

  napi_create_reference(env, argv[3], 1, &(req->ctx));
  napi_create_reference(env, argv[4], 1, &(req->on_write));

  tt_pty_t *pty = &handle->pty;

  req->writing = uv_buf_init(buf, buf_len);

  int err = tt_pty_write((tt_pty_write_t *) req, pty, &req->writing, 1, on_write);

  if (err < 0) TT_NAPI_THROW_ERROR(err);

  NAPI_RETURN_UINT32(err);
}

NAPI_METHOD(tt_napi_pty_resize) {
  NAPI_ARGV(3)
  NAPI_ARGV_BUFFER_CAST(tt_napi_pty_t *, handle, 0)
  NAPI_ARGV_UINT32(width, 1)
  NAPI_ARGV_UINT32(height, 2)

  tt_pty_t *pty = &handle->pty;

  int err = tt_pty_resize(pty, width, height);

  if (err < 0) TT_NAPI_THROW_ERROR(err);

  return NULL;
}

NAPI_METHOD(tt_napi_pty_kill) {
  NAPI_ARGV(2)
  NAPI_ARGV_BUFFER_CAST(tt_napi_pty_t *, handle, 0)
  NAPI_ARGV_INT32(signum, 1)

  tt_pty_t *pty = &handle->pty;

  int err = tt_pty_kill(pty, signum);

  if (err < 0) TT_NAPI_THROW_ERROR(err);

  return NULL;
}

NAPI_INIT() {
  NAPI_EXPORT_UINT32(SIGINT)
  NAPI_EXPORT_UINT32(SIGKILL)
  NAPI_EXPORT_UINT32(SIGTERM)

  NAPI_EXPORT_SIZEOF(tt_napi_pty_t)
  NAPI_EXPORT_SIZEOF(tt_napi_pty_write_t)

  NAPI_EXPORT_FUNCTION(tt_napi_pty_spawn)
  NAPI_EXPORT_FUNCTION(tt_napi_pty_read)
  NAPI_EXPORT_FUNCTION(tt_napi_pty_write)
  NAPI_EXPORT_FUNCTION(tt_napi_pty_resize)
  NAPI_EXPORT_FUNCTION(tt_napi_pty_kill)
}
