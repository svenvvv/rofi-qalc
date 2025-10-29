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
    meson_version: '>= 1.1.0',
)

dep_cairo = dependency('cairo')
dep_glib = dependency('glib-2.0')
dep_qalc = dependency('libqalculate')
dep_rofi = dependency('rofi')

if get_option('use_rofi_next')
    add_project_arguments('-DRQ_ROFI_NEXT', language: 'cpp')
endif

lib = shared_module('rofi-qalc',
    [
        'src/options.cpp',
        'src/rofi_shim.cpp',
        'src/rofi_qalc.cpp',
        'src/rofi_qalc_thread.cpp',
    ],
    install: true,
    include_directories: include_directories('./include'),
    dependencies: [dep_cairo, dep_glib, dep_qalc, dep_rofi],
)

meson.add_install_script('scripts/install_rename.sh', get_option('libdir'), lib.name())
