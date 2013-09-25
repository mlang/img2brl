cmake_minimum_required(VERSION 2.8)
project(img2brl CXX)

find_package(Boost 1.49.0 REQUIRED COMPONENTS locale)

find_package(Gettext)
if(GETTEXT_FOUND)
  set(${CMAKE_PROJECT_NAME}_TRANSLATIONS de fr hu)
  set(${CMAKE_PROJECT_NAME}_LANGUAGES en ${${CMAKE_PROJECT_NAME}_TRANSLATIONS})
  set(${CMAKE_PROJECT_NAME}_gmo_files)
  foreach(lang ${${CMAKE_PROJECT_NAME}_TRANSLATIONS})
    add_custom_command(OUTPUT ${lang}.gmo
                       COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${lang}.gmo ${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po
                       DEPENDS ${lang}.po)
    install(FILES ${lang}.gmo
            DESTINATION locale/${lang}/LC_MESSAGES
            RENAME ${CMAKE_PROJECT_NAME}.mo)
    list(APPEND ${CMAKE_PROJECT_NAME}_gmo_files ${lang}.gmo)
  endforeach(lang)
  add_custom_target(gmo ALL DEPENDS ${${CMAKE_PROJECT_NAME}_gmo_files})
endif(GETTEXT_FOUND)

find_package(PkgConfig REQUIRED)
pkg_search_module(CGICC REQUIRED cgicc)
pkg_search_module(CURL REQUIRED libcurl>=7.10.7)
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
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/img2brl.xpi
          DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories("{CMAKE_CURRENT_BINARY_DIR}")
add_executable(img2brl.cgi img2brl.cc accept_language.cc ubrl.cc ${CMAKE_CURRENT_BINARY_DIR}/config.h)
target_link_libraries(img2brl.cgi ${Boost_LIBRARIES} ${CGICC_LIBRARIES}
                                  ${CURL_LIBRARIES} ${MAGICKPP_LIBRARIES})

install(TARGETS img2brl.cgi DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(FILES favicon.png img2brl.css
        DESTINATION "${CMAKE_INSTALL_PREFIX}")

add_executable(${CMAKE_PROJECT_NAME}_test test.cc accept_language.cc)
enable_testing()
foreach(test_name accept_language_1 accept_language_2 accept_language_3
                  accept_language_4 accept_language_5 accept_language_6
                  accept_language_7)
  add_test(NAME ${test_name}
           COMMAND ${CMAKE_PROJECT_NAME}_test --run_test=${test_name})
endforeach(test_name)

