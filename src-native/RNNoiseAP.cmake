pkg_check_modules(rnnoise REQUIRED IMPORTED_TARGET rnnoise)
add_library(rnnoise-audioproc MODULE rnnoise_proc.cpp)
target_link_libraries(rnnoise-audioproc PkgConfig::rnnoise)

set_target_properties(rnnoise-audioproc PROPERTIES BUNDLED_LIBS "librnnoise")
append_audioproc(rnnoise-audioproc)
