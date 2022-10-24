const binding = require('node-gyp-build')(__dirname)
const b4a = require('b4a')
const { Duplex } = require('streamx')

class PTY extends Duplex {
  constructor (file, args, opts) {
    const {
      width = 80,
      height = 60,
      cwd = process.cwd()
    } = opts

    super({ mapWritable: toBuffer })

    this._handle = b4a.allocUnsafe(binding.sizeof_tt_napi_pty_t)
    this._reading = null
    this._writing = null

    binding.tt_napi_pty_spawn(this._handle, width, height, file, args, cwd, this,
      this._onread,
      this._onend
    )
  }

  _onread (read) {
    const data = this._reading.subarray(0, read)
    this.push(data)

    this._reading = this._reading.subarray(read)
    if (this._reading.byteLength === 0) this._alloc()
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

  _alloc () {
    this._reading = b4a.allocUnsafe(65536)
  }
}

exports.spawn = function spawn (file, args, opts = {}) {
  return new PTY(file, args, opts)
}

function toBuffer (data) {
  return typeof data === 'string' ? b4a.from(data) : data
}
