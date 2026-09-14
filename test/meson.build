# gtest: use the wrap in subprojects/ if the system package is missing.
# default_library=static avoids Anaconda .so clashes at link/run time.
gtest_dep = dependency(
  'gtest',
  main: true,
  required: true,
  fallback: ['gtest', 'gtest_main_dep'],
  default_options: ['gtest:default_library=static'],
)

# Regex implementation units linked into the tester (same as libpz/regex/).
regex_sources = files(
  '../libpz/regex/RegexTokenizer.cpp',
  '../libpz/regex/RegexPostfix.cpp',
  '../libpz/regex/NfaBuilder.cpp',
  '../libpz/regex/NfaMatcher.cpp',
)

# Binary: test/tester.cpp + regex sources + gtest_main.
# FILES_DIR  → test/files/          (small fixtures, gen_small.txt)
# PZTEST_DIR → test/pz-test/        (large corpora from setup_testdata.sh)
tester = executable(
  'tester',
  ['tester.cpp'] + regex_sources,
  dependencies: [gtest_dep, libpz_dep],
  cpp_args: [
    '-DFILES_DIR="' + meson.current_source_dir() / 'files' + '"',
    '-DPZTEST_DIR="' + meson.current_source_dir() / 'pz-test' + '"',
  ],
)

# Registered as `meson test -C builddir` target `poozle_unit_tests`.
# timeout 300s covers large 1MB oracle passes (std + python + grep).
test(
  'poozle_unit_tests',
  tester,
  args: ['--gtest_output=xml:tester_results.xml'],
  timeout: 300,
)
