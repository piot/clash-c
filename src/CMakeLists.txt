cmake_minimum_required(VERSION 3.16.3)
project(clash C)

add_subdirectory(deps/piot/clog/src/lib)
add_subdirectory(deps/piot/flood-c/src/lib)
add_subdirectory(deps/piot/tinge-c/src/lib)
add_subdirectory(deps/piot/tiny-libc/src/lib)


add_subdirectory(lib)
add_subdirectory(examples)


