
tests = [
    'json_cppdict',
    'yaml_cppdict'
]

foreach test:tests
    exe = executable(test, test + '/main.cpp', dependencies:[catch_dep, SpaceWireZMQ_dep])
    test('test_'+test, exe)
endforeach
