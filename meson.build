project(
  'lexer', 
  'c',
)

executable(
  'lexer',
  sources: ['lexer.c', 'main.c', 'pattern_matching.c'],
  dependencies: [meson.get_compiler('c').find_library('pthread')],
  c_args: ['-std=c11', '-Wall', '-Werror']
)

static_library(
  'lexer',
  sources: ['lexer.c', 'pattern_matching.c'],
  dependencies: [meson.get_compiler('c').find_library('pthread')],
  c_args: ['-std=c11', '-Wall', '-Werror']
)
