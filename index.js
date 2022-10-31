const binding = require('node-gyp-build')(__dirname)
const b4a = require('b4a')
const { Duplex } = require('streamx')

const constants = exports.constants = {
  SIGINT: binding.SIGINT,
  SIGKILL: binding.SIGKILL,
  SIGTERM: binding.SIGTERM
}

class PTY extends Duplex {
  constructor (file, args, opts) {
    const {
      width = 80,
      height = 60,
      env = null,
      cwd = null
    } = opts

    super({ mapWritable: toBuffer })

    this._handle = b4a.allocUnsafe(binding.sizeof_tt_napi_pty_t)
    this._running = true
    this._reading = null
    this._writing = null
    this._destroying = null

    this.pid = binding.tt_napi_pty_spawn(this._handle, width, height, file, args, env, cwd, this,
      this._onread,
      this._onend,
      this._onexit
    )
  }

  _onread (err, read) {
    if (err) this.destroy(err)
    else {
      const data = this._reading.subarray(0, read)
      this.push(data)

      this._reading = this._reading.subarray(read)
      if (this._reading.byteLength === 0) this._alloc()
    }

    return this._reading
  }

  _onend () {
    this.push(null)
  }

  _onwrite (err) {
    const { cb } = this._writing
    this._writing.data = null
    this._writing.cb = null

    if (err) cb(err)
    else cb(null)
  }

  _onexit (status, signal) {
    this._running = false

    switch (signal) {
      case constants.SIGINT:
        signal = 'SIGINT'
        break
      case constants.SIGKILL:
        signal = 'SIGKILL'
        break
      case constants.SIGTERM:
        signal = 'SIGTERM'
        break
      default:
        signal = null
    }

    this.emit('exit', status, signal)

    const cb = this._destroying
    this._destroying = null

    if (cb) cb(null)
    else this.destroy()
  }

  _open (cb) {
    this._alloc()
    binding.tt_napi_pty_read(this._handle, this._reading)
    cb(null)
  }

  _write (data, cb) {
    if (this._writing === null) {
      const req = b4a.allocUnsafe(binding.sizeof_tt_napi_pty_write_t)

      this._writing = {
        req,
        data: null,
        cb: null
      }
    }

    this._writing.data = data
    this._writing.cb = cb

    binding.tt_napi_pty_write(this._handle, this._writing.req, data, this,
      this._onwrite
    )
  }

  _predestroy () {
    this.kill()
  }

  _destroy (cb) {
    if (this._running) this._destroying = cb
    else cb(null)
  }

  _alloc () {
    this._reading = b4a.allocUnsafe(65536)
  }

  resize (width, height) {
    binding.tt_napi_pty_resize(this._handle, width, height)
  }

  kill (signal = constants.SIGINT) {
    if (typeof signal !== 'number') {
      switch (signal) {
        case 'SIGINT':
          signal = constants.SIGINT
          break
        case 'SIGKILL':
          signal = constants.SIGKILL
          break
        case 'SIGTERM':
          signal = constants.SIGTERM
          break
        default:
          throw new Error(`Unknown signal "${signal}"`)
      }
    }

    if (this._running) binding.tt_napi_pty_kill(this._handle, signal)
  }
}

exports.spawn = function spawn (file, args, opts) {
  if (Array.isArray(args)) {
    //
  } else if (args === null) {
    args = []
  } else {
    opts = args
    args = []
  }

  if (opts) {
    opts = { ...opts }
  } else {
    opts = {}
  }

  if (opts.env) {
    const env = []

    for (const key in opts.env) env.push(`${key}=${opts.env[key]}`)

    opts.env = env
  }

  args = [file, ...args]

  return new PTY(file, args, opts)
}

function toBuffer (data) {
  return typeof data === 'string' ? b4a.from(data) : data
}
