
tests = [
    'json_cppdict',
    'yaml_cppdict',
    'server',
    'client'
]

test_args = []
if get_option('teamcity')
    test_args += ['-r', 'teamcity']
endif

foreach test:tests
    exe = executable(test, test + '/main.cpp', include_directories:'common', dependencies:[catch_dep, SpaceWireZMQ_dep])
    test('test_'+test, exe, is_parallel: false, args: test_args)
endforeach
