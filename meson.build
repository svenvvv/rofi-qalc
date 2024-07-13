project(
    'rofi-qalculate',
    'cpp',
    version : '1.0.0',
    default_options : [
        'buildtype=debug',
        'optimization=0',
        'cpp_std=c++20',
        'warning_level=3',
    ],
    meson_version: '>= 0.60.0',
)

dep_cairo = dependency('cairo')
dep_glib = dependency('glib-2.0')
dep_qalc = dependency('libqalculate')
dep_rofi = dependency('rofi')

lib = shared_module('rofi-qalculate',
    [
        'src/options.cpp',
        'src/rofi_shim.cpp',
        'src/rofiqalc.cpp',
    ],
    include_directories: include_directories('./include'),
    dependencies: [dep_cairo, dep_glib, dep_qalc, dep_rofi],
)

