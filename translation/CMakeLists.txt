find_package(Gettext)

if(GETTEXT_FOUND)
  set(${CMAKE_PROJECT_NAME}_TRANSLATIONS de es fa fr hu it)
  set(${CMAKE_PROJECT_NAME}_LANGUAGES en ${${CMAKE_PROJECT_NAME}_TRANSLATIONS})
  set(${CMAKE_PROJECT_NAME}_gmo_files)
  foreach(lang ${${CMAKE_PROJECT_NAME}_TRANSLATIONS})
    add_custom_command(OUTPUT ${lang}.gmo
                       COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${lang}.gmo ${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po
                       DEPENDS ${lang}.po)
    install(FILES ${lang}.gmo DESTINATION locale/${lang}/LC_MESSAGES
            RENAME ${CMAKE_PROJECT_NAME}.mo)
    list(APPEND ${CMAKE_PROJECT_NAME}_gmo_files ${lang}.gmo)
  endforeach(lang)
  add_custom_target(gmo ALL DEPENDS ${${CMAKE_PROJECT_NAME}_gmo_files})
endif(GETTEXT_FOUND)

