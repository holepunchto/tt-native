import test from 'brittle'

import { spawn } from '../index.js'

test('spawn', async (t) => {
  t.plan(2)

  const pty = spawn('tty')
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

  const pty = spawn('sh')
  t.ok(pty.pid)

  pty
    .on('close', () => {
      t.pass('closed')
    })
    .kill()
})

test('spawn + destroy', async (t) => {
  t.plan(2)

  const pty = spawn('sh')
  t.ok(pty.pid)

  pty
    .on('close', () => {
      t.pass('closed')
    })
    .destroy()
})
