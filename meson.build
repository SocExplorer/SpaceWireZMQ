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

argparse_dep = cmake.subproject('argparse').dependency('argparse')

SpaceWireZMQ_src = files(['src/main.cpp'])

SpaceWireZMQ_headers = files([
    'src/ZMQServer.hpp',
    'src/SpaceWireBridge.hpp',
    'src/config/Config.hpp',
    'src/config/yaml_io.hpp',
    'src/config/json_io.hpp',
    'src/PacketQueue.hpp',
    'src/SpaceWireZMQ.hpp',
    'src/SpaceWireBridges.hpp'
])

if get_option('with_stardundee')
    SpaceWireZMQ_src += 'src/bridges/StarDundee.cpp'
    SpaceWireZMQ_headers += 'src/bridges/StarDundee.hpp'
endif


SpaceWireZMQ_dep = declare_dependency(
        include_directories:['src'],
        dependencies:[cppzmq_dep, spdlog_dep, yaml_cpp_dep, cppdict_dep,
                      nlohmann_json_dep, channels_dep, fmt_dep, yas_dep,
                      cpp_utils_dep, argparse_dep]
        )

executable('spacewirezmq',SpaceWireZMQ_src,
    dependencies:[SpaceWireZMQ_dep],
    extra_files:SpaceWireZMQ_headers
    )

subdir('tests')
