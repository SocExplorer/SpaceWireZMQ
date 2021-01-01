project('SpaceWireZMQ', 'cpp', default_options : ['cpp_std=c++17'])

cppzmq_dep = dependency('cppzmq')
spdlog_dep = dependency('spdlog')
yaml_cpp_dep = dependency('yaml-cpp')
nlohmann_json_dep = dependency('nlohmann_json')
cppdict_dep = dependency('cppdict')
catch_dep = dependency('catch2', main : true)


SpaceWireZMQ_src = files(['src/main.cpp'])
SpaceWireZMQ_headers = files([
    'src/ZMQServer.hpp',
    'src/SpaceWireBridge.hpp',
    'src/config/Config.hpp',
    'src/config/yaml_io.hpp',
    'src/config/json_io.hpp'
])

SpaceWireZMQ_dep = declare_dependency(
        include_directories:['src'],
        dependencies:[cppzmq_dep, spdlog_dep, yaml_cpp_dep, cppdict_dep, nlohmann_json_dep]
        )

executable('spacewirezmq',SpaceWireZMQ_src,
    dependencies:[SpaceWireZMQ_dep],
    extra_files:SpaceWireZMQ_headers
    )

subdir('tests')
