project(
  'lexer', 
  'c',
)

lexer_src = ['lexer.c', 'pattern_matching.c', 'reader.c']
cargs = ['-std=c11', '-D_XOPEN_SOURCE=600', '-Wall', '-Werror']

static_library(
  'lexer',
  sources: lexer_src,
  dependencies: [
    meson.get_compiler('c').find_library('pthread'),
    meson.get_compiler('c').find_library('rt'),
  ],
  c_args: cargs
)

executable(
  'lexer-test',
  sources: [lexer_src, 'test.c'],
  dependencies: [meson.get_compiler('c').find_library('pthread')],
  c_args: cargs
)

executable(
  'pattern-test',
  sources: ['pattern_matching.c', 'pattern_matching_test.c'],
  c_args: cargs
)
