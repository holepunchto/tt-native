import test from 'brittle'
import util from 'util'
import path from 'path'

import { spawn } from '../index.js'

const shell = process.platform === 'win32' ? 'powershell.exe' : 'bash'

test('basic', async (t) => {
  t.plan(4)

  const pty = spawn('node', ['test/fixtures/hello.mjs'])
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      t.comment(util.inspect(`${data}`, { colors: true }))
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

test('kill twice', async (t) => {
  t.plan(2)

  const pty = spawn('node', ['test/fixtures/spin.mjs'])
  t.ok(pty.pid)

  pty
    .on('close', () => {
      t.pass('closed')
    })

  pty.kill()
  pty.kill()
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
      t.is(code, process.platform === 'win32' ? 1 : 0)
      t.is(signal, 'SIGTERM')
    })
    .on('close', () => {
      t.pass('closed')
    })
    .kill('SIGTERM')
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
      t.ok(`${data}`.includes(path.join(process.cwd(), 'test')))
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
      t.comment(util.inspect(`${data}`, { colors: true }))
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
      t.comment(util.inspect(`${data}`, { colors: true }))
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
      t.comment(util.inspect(`${data}`, { colors: true }))
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
