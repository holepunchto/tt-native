import test from 'brittle'

import { spawn } from '../index.js'

test('spawn pty', async (t) => {
  t.plan(1)

  const pty = spawn('tty')

  pty
    .on('data', (data) => {
      t.comment(data.toString().trim())
    })
    .on('end', () => {
      t.pass('ended')
    })
})
