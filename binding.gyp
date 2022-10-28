{
  'targets': [{
    'target_name': 'tt',
    'include_dirs': [
      '<!(node -e "require(\'napi-macros\')")',
      './vendor/libtt/include',
    ],
    'dependencies': [
      './vendor/libtt.gyp:libtt',
    ],
    'sources': [
      './binding.c',
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
      ['OS=="linux"', {
        'libraries': [
          '-lutil',
        ],
      }],
    ]
  }]
}
