cmake_minimum_required(VERSION 2.8)

project("native-static-webserver")

file(GLOB SOURCE_FILES "src/*.cc" "src/*.h" "include/*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC} "node-headers/node-headers" "asio/asio/include")

target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} )