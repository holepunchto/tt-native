import test from 'brittle'
import path from 'path'
import process from 'process'
import b4a from 'b4a'

import { spawn } from './index.js'

const isWindows = process.platform === 'win32'

const shell = isWindows ? 'powershell.exe' : 'bash'

test('basic', async (t) => {
  t.plan(4)

  const pty = spawn('node', ['test/fixtures/hello.mjs'])
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      t.comment(data)
    })
    .on('exit', () => {
      t.pass('exited')
    })
    .on('end', () => {
      t.pass('ended')
    })
    .on('close', () => {
      t.pass('closed')
    })
})

test('no args or opts', async (t) => {
  t.plan(2)

  const pty = spawn('node')
  t.ok(pty.pid)

  pty
    .on('close', () => {
      t.pass('closed')
    })
    .kill()
})

test('kill', async (t) => {
  t.plan(2)

  const pty = spawn('node', ['test/fixtures/spin.mjs'])
  t.ok(pty.pid)

  pty
    .on('close', () => {
      t.pass('closed')
    })
    .kill()
})

test('destroy', async (t) => {
  t.plan(2)

  const pty = spawn('node', ['test/fixtures/spin.mjs'])
  t.ok(pty.pid)

  pty
    .on('close', () => {
      t.pass('closed')
    })
    .destroy()
})

test('exit with code', async (t) => {
  t.plan(3)

  const pty = spawn('node', ['test/fixtures/exit.mjs'])
  t.ok(pty.pid)

  pty
    .on('exit', (code) => {
      t.is(code, 42)
    })
    .on('close', () => {
      t.pass('closed')
    })
})

test('kill with signal', async (t) => {
  t.plan(5)

  const pty = spawn('node', ['test/fixtures/spin.mjs'])
  t.ok(pty.pid)

  pty
    .on('exit', (code, signal) => {
      t.pass('exited')
      t.is(code, isWindows ? 1 : 0)
      t.is(signal, 'SIGTERM')
    })
    .on('close', () => {
      t.pass('closed')
    })
    .kill('SIGTERM')
})

test('kill twice', async (t) => {
  t.plan(5)

  const pty = spawn('node', ['test/fixtures/sigint.mjs'])
  t.ok(pty.pid)

  pty
    .on('exit', (code, signal) => {
      t.pass('exited')
      t.is(code, isWindows ? 1 : 0)
      t.is(signal, 'SIGTERM')
    })
    .on('close', () => {
      t.pass('closed')
    })

  setTimeout(() => {
    pty.kill('SIGINT') // Caught and ignored

    setTimeout(() => pty.kill('SIGTERM'), 200)
  }, 200)
})

test.skip('resize', async (t) => {
  t.plan(3)

  const pty = spawn('node', ['test/fixtures/resize.mjs'])
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      t.is(`${data}`, '120x90')
    })
    .on('close', () => {
      t.pass('closed')
    })

  setTimeout(() => pty.resize(120, 90), 100)
})

test('resize after exit', async (t) => {
  t.plan(3)

  const pty = spawn('node')
  t.ok(pty.pid)

  pty
    .on('exit', () => {
      t.pass('exited')
      t.exception(() => pty.resize(120, 90), /Process has exited/)
    })
    .kill()
})

test('env', async (t) => {
  t.plan(3)

  const pty = spawn('node', ['test/fixtures/env.mjs'], {
    env: {
      ...process.env,
      FOO: '42'
    }
  })
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      t.ok(`${data}`.includes('42'))
    })
    .on('close', () => {
      t.pass('closed')
    })
})

test('cwd', async (t) => {
  t.plan(3)

  const pty = spawn('node', ['fixtures/cwd.mjs'], {
    cwd: 'test'
  })
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      t.ok(`${data}`.includes(path.resolve('test')))
    })
    .on('close', () => {
      t.pass('closed')
    })
})

test('shell', async (t) => {
  t.comment('shell', shell)

  t.plan(3)

  const pty = spawn(shell)
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      t.comment(data)
    })
    .on('exit', () => {
      t.pass('exited')
    })
    .on('close', () => {
      t.pass('closed')
    })

  setTimeout(() => pty.kill('SIGKILL'), 200)
})

test('shell write', async (t) => {
  t.comment('shell', shell)

  t.plan(3)

  const pty = spawn(shell)
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      t.comment(data)
    })
    .on('exit', () => {
      t.pass('exited')
    })
    .on('close', () => {
      t.pass('closed')
    })

  pty.write('echo hello world\n')

  setTimeout(() => pty.kill('SIGKILL'), 200)
})

test('shell write after delay', async (t) => {
  t.comment('shell', shell)

  t.plan(3)

  const pty = spawn(shell)
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      t.comment(data)
    })
    .on('exit', () => {
      t.pass('exited')
    })
    .on('close', () => {
      t.pass('closed')
    })

  setTimeout(() => {
    pty.write('echo hello world\n')

    setTimeout(() => pty.kill('SIGKILL'), 200)
  }, 500)
})

// https://github.com/holepunchto/libtt/issues/1
test.skip('trigger signal', async (t) => {
  t.plan(3)

  const pty = spawn('node', ['test/fixtures/sigint.mjs'])
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      t.comment(data)

      if (b4a.includes(data, 'SIGINT')) {
        t.pass('triggered SIGINT')
      }
    })
    .on('exit', () => {
      t.pass('exited')
    })
    .on('close', () => {
      t.pass('closed')
    })

  setTimeout(() => pty.write('\x03' /* Ctrl + C */), 100)
})
