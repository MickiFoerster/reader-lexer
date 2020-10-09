project(
  'lexer', 
  'c',
)

executable(
  'lexer',
  sources: ['lexer.c', 'main.c', 'reader.c'],
  dependencies: [meson.get_compiler('c').find_library('pthread')],
  c_args: ['-std=c11', '-Wall', '-Werror']
)

