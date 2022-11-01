import test from 'brittle'
import path from 'path'
import b4a from 'b4a'

import { spawn } from '../index.js'

const shell = process.platform === 'win32?' ? 'powershell.exe' : (process.env.SHELL || 'bash')

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
      t.is(code, process.platform === 'win32' ? 1 : 0)
      t.is(signal, 'SIGTERM')
    })
    .on('close', () => {
      t.pass('closed')
    })
    .kill('SIGTERM')
})

test('resize', { skip: process.platform === 'win32' }, async (t) => {
  t.plan(3)

  const pty = spawn('node', ['test/fixtures/resize.mjs'])
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      const size = b4a.toString(data)
      t.is(size, '120x90')
    })
    .on('close', () => {
      t.pass('closed')
    })

  setTimeout(() => pty.resize(120, 90), 100)
})

test('env', { skip: process.platform === 'win32' }, async (t) => {
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
      const foo = data.toString().trim()
      t.is(foo, '42')
    })
    .on('close', () => {
      t.pass('closed')
    })
})

test('cwd', { skip: process.platform === 'win32' }, async (t) => {
  t.plan(3)

  const pty = spawn('node', ['fixtures/cwd.mjs'], {
    cwd: 'test'
  })
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      const cwd = data.toString().trim()
      t.is(cwd, path.join(process.cwd(), 'test'))
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
