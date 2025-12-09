cmake_minimum_required(VERSION 3.11-3.18)

project(libBrakeCooling)

set(BUILD_SHARED_LIBS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS True) # iso using declsped(dllexport)

add_library(libBrakeCooling STATIC src/libBrakeCooling.cpp)

# PUBLIC needed to make both libBrakeCooling.h and libBrakeCooling library available elsewhere in project
target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_features(libBrakeCooling PUBLIC cxx_std_17)

# copy dll to driver app folder
add_custom_command(TARGET libBrakeCooling POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:libBrakeCooling>"
     #"../$<CONFIGURATION>/$<TARGET_FILE_NAME:libBrakeCooling>" 
     "../$<TARGET_FILE_NAME:libBrakeCooling>" 
  COMMENT "Copying to output directory")