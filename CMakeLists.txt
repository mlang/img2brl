cmake_minimum_required(VERSION 2.8)
project(img2brl CXX)
find_package(Boost REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(CGICC REQUIRED cgicc)
pkg_search_module(CURL REQUIRED libcurl)
pkg_search_module(MAGICKPP REQUIRED Magick++)
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
  file(SHA512 "${CMAKE_CURRENT_BINARY_DIR}/img2brl.xpi" IMG2BRL_XPI_SHA512)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/img2brl.xpi
          DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories("{CMAKE_CURRENT_BINARY_DIR}")
add_executable(img2brl.cgi img2brl.cc ubrl.cc ${CMAKE_CURRENT_BINARY_DIR}/config.h)
target_link_libraries(img2brl.cgi ${CGICC_LIBRARIES} ${CURL_LIBRARIES}
                                  ${MAGICKPP_LIBRARIES})

install(TARGETS img2brl.cgi DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(FILES favicon.png img2brl.css
        DESTINATION "${CMAKE_INSTALL_PREFIX}")

