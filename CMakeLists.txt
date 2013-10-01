cmake_minimum_required(VERSION 2.8)
project(img2brl CXX)

find_package(Boost 1.49.0 REQUIRED COMPONENTS locale)
find_package(PkgConfig REQUIRED)
pkg_search_module(CGICC REQUIRED cgicc)
pkg_search_module(CURL REQUIRED libcurl>=7.10.7)
pkg_search_module(MAGICKPP REQUIRED Magick++)

add_subdirectory(translation)
include_directories(${CGICC_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${MAGICKPP_INCLUDE_DIRS})
add_definitions(-std=c++11)
find_package(Git)
if(GIT_FOUND)
  install(PROGRAMS git-post-update.hook
          DESTINATION "${CMAKE_INSTALL_PREFIX}/hooks"
          RENAME post-update)
endif()

find_program(ZIP_EXECUTABLE zip)
if(ZIP_EXECUTABLE)
  add_custom_target(firefox-extension ALL DEPENDS img2brl.xpi)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/img2brl.xpi"
                     COMMAND ${ZIP_EXECUTABLE} -r ${CMAKE_CURRENT_BINARY_DIR}/img2brl.xpi .
                     DEPENDS xul/chrome.manifest xul/install.rdf
                             xul/content/browserOverlay.xul
                             xul/content/img2brl.js
                             xul/content/img2brl.properties
                             xul/locale/en-US/browserOverlay.dtd
                     WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/xul")
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/img2brl.xpi
          DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)
include_directories("{CMAKE_CURRENT_BINARY_DIR}/src")
add_executable(img2brl.cgi src/img2brl.cc src/accept_language.cc src/ubrl.cc
                           ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)
target_link_libraries(img2brl.cgi ${Boost_LIBRARIES} ${CGICC_LIBRARIES}
                                  ${CURL_LIBRARIES} ${MAGICKPP_LIBRARIES})

install(TARGETS img2brl.cgi DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(FILES favicon.png img2brl.css DESTINATION "${CMAKE_INSTALL_PREFIX}")

add_executable(${CMAKE_PROJECT_NAME}_test src/test.cc src/accept_language.cc)
enable_testing()
foreach(test_name accept_language_1 accept_language_2 accept_language_3
                  accept_language_4 accept_language_5 accept_language_6
                  accept_language_7)
  add_test(NAME ${test_name}
           COMMAND ${CMAKE_PROJECT_NAME}_test --run_test=${test_name})
endforeach(test_name)

