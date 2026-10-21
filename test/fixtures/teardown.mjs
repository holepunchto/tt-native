import process from 'process'
import { parentPort } from 'worker_threads'

import { spawn } from '../../index.js'

const pty = spawn('node', ['test/fixtures/spin.mjs'])

parentPort.postMessage(pty.pid)

process.exit()
