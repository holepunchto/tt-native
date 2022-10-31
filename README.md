# tt-native

https://github.com/holepunchto/libtt JavaScript bindings for Node.js.

```sh
npm install fs-native
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

#### `pty.pid`

#### `pty.on('exit', code[, signal])`

#### `pty.resize(width, height)`

#### `pty.kill([signal])`

## License

ISC
