const { stdout } = process

const data = Buffer.alloc(64 * 1024, 'abcd')

stdout.write(data)
