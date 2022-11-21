# tt-native

https://github.com/holepunchto/libtt JavaScript bindings for Node.js.

```sh
npm install tt-native
```

## Usage

```js
const { spawn } = require('tt-native')

const pty = spawn('node', ['script.js'])
```

## API

#### `const pty = spawn(file[, args][, options])`

Spawn a process attached to a pseudo TTY. The returned PTY is a [duplex stream](https://github.com/streamxorg/streamx#duplex-stream).

Options include:

```js
{
  width: 80,
  height: 60,
  env: process.env,
  cwd: process.cwd()
}
```

#### `pty.width`

The current width of the PTY.

#### `pty.height`

The current height of the PTY.

#### `pty.pid`

The process ID of the spawned process.

#### `pty.on('exit', code[, signal])`

Emitted when the process exit.

#### `pty.resize(width, height)`

Resize the PTY.

#### `pty.kill([signal])`

Kill the process with the specified signal, which defaults to `SIGINT`. The following signals may be used:

- `SIGINT`
- `SIGKILL`
- `SIGTERM`

## License

ISC
