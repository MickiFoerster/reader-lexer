project(
  'lexer', 
  'c',
)

lexer_src = ['lexer.c', 'pattern_matching.c', 'reader.c']

executable(
  'lexer-test',
  sources: [lexer_src, 'test.c'],
  dependencies: [meson.get_compiler('c').find_library('pthread')],
  c_args: ['-std=c11', '-D_POSIX_C_SOURCE=199309L', '-Wall', '-Werror']
)

static_library(
  'lexer',
  sources: lexer_src,
  dependencies: [
    meson.get_compiler('c').find_library('pthread'),
    meson.get_compiler('c').find_library('rt'),
  ],
  c_args: ['-std=c11', '-D_POSIX_C_SOURCE=199309L', '-Wall', '-Werror']
)
