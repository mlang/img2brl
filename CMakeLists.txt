cmake_minimum_required(VERSION 3.13)
project(img2brl CXX)
set(CMAKE_CXX_STANDARD 11)
find_package(Boost 1.49.0 REQUIRED COMPONENTS locale)
find_package(PkgConfig REQUIRED)
pkg_search_module(CGICC REQUIRED IMPORTED_TARGET cgicc)
pkg_search_module(CURL REQUIRED IMPORTED_TARGET libcurl>=7.10.7)
pkg_search_module(MAGICKPP REQUIRED IMPORTED_TARGET Magick++)
add_subdirectory(translation)
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
add_library(img2brl STATIC src/accept_language.cc src/ubrl.cc)
target_link_libraries(img2brl PkgConfig::MAGICKPP)
add_executable(img2brl.cgi src/img2brl.cc
                           ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)
target_link_libraries(img2brl.cgi img2brl ${Boost_LIBRARIES} PkgConfig::CGICC
                                  PkgConfig::CURL PkgConfig::MAGICKPP)

install(TARGETS img2brl.cgi DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(FILES favicon.png img2brl.css DESTINATION "${CMAKE_INSTALL_PREFIX}")

add_executable(${CMAKE_PROJECT_NAME}_test src/test.cc)
target_link_libraries(${CMAKE_PROJECT_NAME}_test img2brl)
enable_testing()
foreach(test_name accept_language_1 accept_language_2 accept_language_3
                  accept_language_4 accept_language_5 accept_language_6
                  accept_language_7)
  add_test(NAME ${test_name}
           COMMAND ${CMAKE_PROJECT_NAME}_test --run_test=${test_name})
endforeach(test_name)
