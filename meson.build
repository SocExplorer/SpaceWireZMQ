project('SpaceWireZMQ', 'cpp', default_options : ['cpp_std=c++17'])

cmake = import('cmake')

cppzmq_dep = dependency('cppzmq', required: false)
if not cppzmq_dep.found()
    cppzmq_dep = declare_dependency(include_directories:'subprojects/cppzmq')
endif

spdlog_dep = dependency('spdlog')
yaml_cpp_dep = dependency('yaml-cpp')
nlohmann_json_dep = dependency('nlohmann_json')
fmt_dep = dependency('fmt')
cppdict_dep = dependency('cppdict')
cpp_utils_dep = dependency('cpp_utils')
catch_dep = dependency('catch2', main : true)
channels_dep = dependency('channels')
yas_dep = dependency('yas')
SpaceWirePP_dep = dependency('spacewirepp')

argparse_dep = cmake.subproject('argparse').dependency('argparse')

SpaceWireZMQ_src = files([
    'src/ZMQServer.cpp'
])

SpaceWireZMQ_headers = files([
    'src/ZMQServer.hpp',
    'src/SpaceWireBridge.hpp',
    'src/config/Config.hpp',
    'src/config/yaml_io.hpp',
    'src/config/json_io.hpp',
    'src/PacketQueue.hpp',
    'src/SpaceWireZMQ.hpp',
    'src/SpaceWireBridges.hpp',
    'src/callable.hpp'
])

SpaceWireZMQ_dependencies = [
                  cppzmq_dep, spdlog_dep, yaml_cpp_dep, cppdict_dep,
                  nlohmann_json_dep, channels_dep, fmt_dep, yas_dep,
                  cpp_utils_dep, argparse_dep
]

if get_option('with_stardundee')
    SpaceWireZMQ_src += ['src/bridges/StarDundee.cpp']
    SpaceWireZMQ_headers += ['src/bridges/StarDundee.hpp', 'src/bridges/StarAPI.hpp']
    cpp_compiler = meson.get_compiler('cpp')
    StarDundee_dep = declare_dependency(
        dependencies: [
            cpp_compiler.find_library('star-api', dirs:'/usr/local/STAR-Dundee/STAR-System/lib/x86-64'),
            cpp_compiler.find_library('star_conf_api_generic', dirs:'/usr/local/STAR-Dundee/STAR-System/lib/x86-64'),
            cpp_compiler.find_library('star_conf_api_mk2', dirs:'/usr/local/STAR-Dundee/STAR-System/lib/x86-64'),
            cpp_compiler.find_library('star_conf_api_brick_mk2', dirs:'/usr/local/STAR-Dundee/STAR-System/lib/x86-64'),
            cpp_compiler.find_library('star_conf_api_brick_mk3', dirs:'/usr/local/STAR-Dundee/STAR-System/lib/x86-64')
            ]
        ,
        include_directories: '/usr/local/STAR-Dundee/STAR-System/inc/star'
    )
    SpaceWireZMQ_dependencies += StarDundee_dep
endif


SpaceWireZMQ_lib = library('spacewirezmq',SpaceWireZMQ_src,
                           include_directories:['src'],
                           dependencies: SpaceWireZMQ_dependencies)

SpaceWireZMQ_dep = declare_dependency(
        link_with: SpaceWireZMQ_lib,
        include_directories: ['src'],
        dependencies: SpaceWireZMQ_dependencies
        )

executable('spacewirezmq','src/main.cpp',
    dependencies: [SpaceWireZMQ_dep],
    extra_files: SpaceWireZMQ_headers
    )

subdir('tests')
subdir('manual_tests')
