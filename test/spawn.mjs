import test from 'brittle'

import { spawn } from '../index.js'

test('spawn', async (t) => {
  t.plan(2)

  const pty = spawn('node', ['test/fixtures/hello.mjs'])
  t.ok(pty.pid)

  pty
    .on('data', (data) => {
      t.comment(data.toString().trim())
    })
    .on('end', () => {
      t.pass('ended')
    })
})

test('spawn + kill', async (t) => {
  t.plan(2)

  const pty = spawn('node', ['test/fixtures/spin.mjs'])
  t.ok(pty.pid)

  pty
    .on('close', () => {
      t.pass('closed')
    })
    .kill()
})

test('spawn + destroy', async (t) => {
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
  t.plan(2)

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
  t.plan(4)

  const pty = spawn('node', ['test/fixtures/spin.mjs'])
  t.ok(pty.pid)

  pty
    .on('exit', (code, signal) => {
      t.is(code, process.platform === 'win32' ? 1 : 0)
      t.is(signal, 'SIGTERM')
    })
    .on('close', () => {
      t.pass('closed')
    })
    .kill('SIGTERM')
})
