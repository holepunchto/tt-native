{
  'targets': [{
    'target_name': 'libtt',
    'type': 'static_library',
    'sources': [
      './libtt/src/pty.c',
    ],
    'configurations': {
      'Debug': {
        'defines': ['DEBUG'],
      },
      'Release': {
        'defines': ['NDEBUG'],
      },
    },
    'conditions': [
      ['OS=="win"', {
        'sources': [
          './libtt/src/win/pty.c',
        ],
      }, {
        'sources': [
          './libtt/src/unix/pty.c',
        ],
      }],
    ],
  }]
}
