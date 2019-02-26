# uncomment as necessary
project('killer', 'cpp',
        license : ['Boost'],
        version : '0.1',
        default_options : ['cpp_std=c++17', 'warning_level=3', 'werror=true'])

thread_dep = dependency('threads')

# begin asio: https://think-async.com/Asio/
add_global_arguments('-I' + get_option('asio_include_dir'), language : 'cpp')
add_global_arguments('-DASIO_STANDALONE', language : 'cpp')
add_global_arguments('-DASIO_NO_DEPRECATED', language : 'cpp')
# end asio

# CLI11: https://github.com/CLIUtils/CLI11
add_global_arguments('-I' + get_option('cli11_include_dir'), language : 'cpp')

# spdlog: https://github.com/gabime/spdlog
add_global_arguments('-I' + get_option('spdlog_include_dir'), language : 'cpp')

# json: https://github.com/nlohmann/json
#add_global_arguments('-I' + get_option('json_include_dir'), language : 'cpp')

# # begin protobuf: https://developers.google.com/protocol-buffers/
# # protobuf: must be installed independently and has to be found...
protoc = find_program('protoc', required : true)
protobuf_dep = dependency('protobuf', required : true)

gen = generator(protoc, \
  output    : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
  arguments : ['--proto_path=@CURRENT_SOURCE_DIR@/src', '--cpp_out=@BUILD_DIR@', '@PLAINNAME@'])

# # change proto file accordingly
generated = gen.process('src/messages.proto')
# # end protobuf

# subdir('SUBDIR1')

inc_dir = include_directories('include')
#src = ['src/main.cpp']
#protobuf: 

executable('Broker',
           sources : ['src/Broker/broker.cpp', generated],
           include_directories : inc_dir,
           dependencies : [thread_dep, protobuf_dep]
          )

executable('Client',
           sources : ['src/Client/client.cpp', generated],
           include_directories : inc_dir,
           dependencies : [thread_dep, protobuf_dep]
          )