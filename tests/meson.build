executable('test-multisession', 'test-multisession.c',
    c_args: '-DBUILDDIR="@0@"'.format(meson.current_build_dir()),
    include_directories: top_include, link_with: libchimara,
    dependencies: [gmodule, gtk])
executable('glulxercise', 'glulxercise.c',
    c_args: '-DPACKAGE_SRC_DIR="@0@"'.format(meson.current_source_dir()),
    include_directories: top_include, link_with: libchimara,
    dependencies: [gmodule, gtk])
executable('plugin-loader', 'plugin-loader.c',
    include_directories: top_include, link_with: libchimara,
    dependencies: [gmodule, gtk])
executable('test-close', 'test-close.c',
    c_args: '-DPACKAGE_SRC_DIR="@0@"'.format(meson.current_source_dir()),
    include_directories: top_include, link_with: libchimara,
    dependencies: [gmodule, gtk])
executable('csstest', 'csstest.c',
    c_args: '-DBUILDDIR="@0@"'.format(meson.current_build_dir()),
    include_directories: top_include, link_with: libchimara,
    dependencies: [gmodule, gtk])
glkunit_runner = executable('glkunit-runner', 'glkunit-runner.c', 'keycode.c',
    include_directories: top_include, link_with: libchimara,
    dependencies: [gmodule, gtk])
cssparse = executable('cssparse', 'cssparse.c',
    include_directories: top_include, link_with: libchimara,
    dependencies: [gmodule, gtk])
glulxercise_runner = executable('glulxercise-runner',
    'glulxercise-runner.c', 'keycode.c',
    include_directories: top_include, link_with: libchimara,
    dependencies: [gmodule, gtk])
reftest_runner = executable('reftest-runner',
    'reftest-runner.c', 'reftest-compare.c',
    include_directories: top_include, link_with: libchimara,
    dependencies: [gmodule, gtk])

shared_module('first', 'first.c', name_prefix: '',
    include_directories: top_include,
    link_args: plugin_link_args, link_depends: plugin_link_depends)
shared_module('model', 'model.c', name_prefix: '',
    include_directories: top_include,
    link_args: plugin_link_args, link_depends: plugin_link_depends)
shared_module('gridtest', 'gridtest.c', name_prefix: '',
    include_directories: top_include,
    link_args: plugin_link_args, link_depends: plugin_link_depends)
shared_module('splittest', 'splittest.c', name_prefix: '',
    include_directories: top_include,
    link_args: plugin_link_args, link_depends: plugin_link_depends)
shared_module('multiwin', 'multiwin.c', name_prefix: '',
    include_directories: top_include,
    link_args: plugin_link_args, link_depends: plugin_link_depends)
shared_module('styletest', 'styletest.c', name_prefix: '',
    include_directories: top_include,
    link_args: plugin_link_args, link_depends: plugin_link_depends)
shared_module('test-userstyle', 'test-userstyle.c', name_prefix: '',
    include_directories: top_include,
    link_args: plugin_link_args, link_depends: plugin_link_depends)
shared_module('soundtest', 'soundtest.c', name_prefix: '',
    include_directories: top_include,
    link_args: plugin_link_args, link_depends: plugin_link_depends)
shared_module('fileio', 'fileio.c', name_prefix: '',
    include_directories: top_include, dependencies: glib,
    link_args: plugin_link_args, link_depends: plugin_link_depends)

test_env = environment()
test_env.set('NO_AT_BRIDGE', '1')
test_env.set('G_DEBUG', 'fatal-warnings')
test_env.set('SOURCE_DIR', meson.current_source_dir())
test_env.set('LSAN_OPTIONS',
    'suppressions=@0@/lsan.supp'.format(meson.current_source_dir()))

unit_tests = [
    ['datetime', []],
    ['graphchar', ['-c', 'return', '-c', '0xc4']],
    ['io', []],
    ['stream', []],
    ['window', []],
]

foreach t : unit_tests
    name = t[0]
    plugin = shared_module(name, 'unit/@0@.c'.format(name), 'unit/glkunit.c',
        name_prefix: '', include_directories: [top_include, '../libchimara'],
        link_args: plugin_link_args, link_depends: plugin_link_depends)
    test(name, glkunit_runner, args: [plugin, t[1]], suite: 'unit',
        protocol: 'tap', env: test_env)
endforeach

reftests = [
    'zero-height-window',
    'zero-width-window',
]

reftest_deps = []
foreach t : reftests
    plugin = shared_module(t, 'reftest/@0@.c'.format(t), name_prefix: '',
        include_directories: [top_include, '../libchimara'],
        link_args: plugin_link_args, link_depends: plugin_link_depends)
    reftest_deps += plugin
endforeach

reftest_dir = join_paths(meson.current_source_dir(), 'reftest')
test('reftests', reftest_runner, args: [reftest_dir, meson.current_build_dir()],
    depends: reftest_deps, suite: 'reftest', protocol: 'tap', env: test_env)

glulxercise_tests = [
    'abbrevtest',
    'abbrevtest-e',
    'chinesedicttest',
    'datetimetest',
    'dictflagtest',
    'extbinaryfile',
    'externalfile',
    'floatconsttest',
    'floatparsetest',
    'glulxercise',
    'graphwintest',
    'imagetest',
    'inputeventtest',
    'memcopytest',
    'memheaptest',
    'memstreamtest',
    'resstreamtest',
    'startsavetest',
    'unicasetest',
    'unicodetest',
    'unidicttest',
    'unisourcetest',
    'unisourcetestb',
    'windowtest',
]

foreach t : glulxercise_tests
    path = files('glulxercise/@0@.regtest'.format(t))
    test(t, glulxercise_runner, args: [path], suite: 'glulxercise',
        protocol: 'tap', env: test_env, depends: [glulxe, glulxercise_runner])
endforeach

test('cssparse', cssparse, protocol: 'tap', env: test_env)
