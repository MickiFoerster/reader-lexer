project(
  'lexer', 
  'c',
)

executable(
  'lexer-test',
  sources: ['lexer.c', 'test.c', 'pattern_matching.c'],
  dependencies: [meson.get_compiler('c').find_library('pthread')],
  c_args: ['-std=c11', '-D_POSIX_C_SOURCE=199309L', '-Wall', '-Werror']
)

static_library(
  'lexer',
  sources: ['lexer.c', 'pattern_matching.c'],
  dependencies: [
    meson.get_compiler('c').find_library('pthread'),
    meson.get_compiler('c').find_library('rt'),
  ],
  c_args: ['-std=c11', '-D_POSIX_C_SOURCE=199309L', '-Wall', '-Werror']
)
