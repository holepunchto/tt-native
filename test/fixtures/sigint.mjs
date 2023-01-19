const { stdin, stdout } = process

stdin.resume()

process.on('SIGINT', () => {
  stdout.write('SIGINT\n')
})
