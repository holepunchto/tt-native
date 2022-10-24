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
  napi_ref on_end;
} tt_napi_pty_t;

typedef struct {
  uv_write_t req;

  napi_env env;
  napi_ref ctx;

  napi_ref on_write;
} tt_napi_pty_write_t;

NAPI_METHOD(tt_napi_pty_spawn) {
  NAPI_ARGV(9)
  NAPI_ARGV_BUFFER_CAST(tt_napi_pty_t *, handle, 0)
  NAPI_ARGV_UINT32(width, 1)
  NAPI_ARGV_UINT32(height, 2)
  NAPI_ARGV_UTF8_MALLOC(file, 3)
  NAPI_ARGV_UTF8_MALLOC(cwd, 5)

  handle->env = env;

  napi_create_reference(env, argv[6], 1, &(handle->ctx));
  napi_create_reference(env, argv[7], 1, &(handle->on_read));
  napi_create_reference(env, argv[8], 1, &(handle->on_end));

  uv_loop_t *loop;
  napi_get_uv_event_loop(env, &loop);

  tt_pty_t *pty = &handle->pty;

  tt_term_options_t term = {
    .width = width,
    .height = height,
  };

  tt_process_options_t process = {
    .file = file,
    .cwd = cwd,
  };

  int err = tt_pty_spawn(loop, pty, &term, &process);

  free(file);
  free(cwd);

  if (err < 0) TT_NAPI_THROW_ERROR(err);

  NAPI_RETURN_UINT32(pty->pid)
}

static void
on_end (uv_stream_t *stream) {
  tt_napi_pty_t *pty = (tt_napi_pty_t *) stream;

  napi_env env = pty->env;

  TT_NAPI_CALLBACK(pty, pty->on_end, {
    NAPI_MAKE_CALLBACK(env, NULL, ctx, callback, 0, NULL, NULL);
  });

  napi_delete_reference(env, pty->ctx);
  napi_delete_reference(env, pty->on_read);
  napi_delete_reference(env, pty->on_end);
}

static void
on_read (uv_stream_t *stream, ssize_t read_len, const uv_buf_t *buf) {
  if (read_len == UV_EOF) return on_end(stream);

  tt_napi_pty_t *pty = (tt_napi_pty_t *) stream;

  napi_env env = pty->env;

  TT_NAPI_CALLBACK(pty, pty->on_read, {
    napi_value result;
    napi_value argv[1];
    napi_create_uint32(env, read_len, &argv[0]);
    NAPI_MAKE_CALLBACK(env, NULL, ctx, callback, 1, argv, &result);

    char *next;
    size_t next_len;
    napi_get_buffer_info(env, result, (void **) &next, &next_len);

    pty->reading = uv_buf_init(next, next_len);
  });
}

static void
on_alloc (uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  tt_napi_pty_t *pty = (tt_napi_pty_t *) handle;

  *buf = pty->reading;
}

NAPI_METHOD(tt_napi_pty_read) {
  NAPI_ARGV(2)
  NAPI_ARGV_BUFFER_CAST(tt_napi_pty_t *, handle, 0)
  NAPI_ARGV_BUFFER(buf, 1)

  handle->reading = uv_buf_init(buf, buf_len);

  tt_pty_t *pty = &handle->pty;

  int err = uv_read_start((uv_stream_t *) pty, on_alloc, on_read);

  if (err < 0) TT_NAPI_THROW_ERROR(err);

  return NULL;
}

static void
on_write (uv_write_t *req, int status) {
  tt_napi_pty_write_t *r = (tt_napi_pty_write_t *) req;

  napi_env env = r->env;

  TT_NAPI_CALLBACK(r, r->on_write, {
    napi_value argv[1];
    TT_NAPI_MAKE_ERROR_OR_NULL(status, &argv[0]);
    NAPI_MAKE_CALLBACK(env, NULL, ctx, callback, 1, argv, NULL);
  });

  napi_delete_reference(env, r->ctx);
  napi_delete_reference(env, r->on_write);
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

  uv_buf_t b = uv_buf_init(buf, buf_len);

  int err = uv_write((uv_write_t *) req, (uv_stream_t *) pty, &b, 1, on_write);

  if (err < 0) TT_NAPI_THROW_ERROR(err);

  NAPI_RETURN_UINT32(err);
}

NAPI_INIT() {
  NAPI_EXPORT_SIZEOF(tt_napi_pty_t)
  NAPI_EXPORT_SIZEOF(tt_napi_pty_write_t)

  NAPI_EXPORT_FUNCTION(tt_napi_pty_spawn)
  NAPI_EXPORT_FUNCTION(tt_napi_pty_read)
  NAPI_EXPORT_FUNCTION(tt_napi_pty_write)
}
