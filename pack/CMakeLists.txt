set(CONF_DIR ${CMAKE_SOURCE_DIR}/conf)
set(RSRC_DIR ${CMAKE_SOURCE_DIR}/rsrc)

set(MAC $<PLATFORM_ID:Darwin>)

# Windows packaging is x64-only. (CMake's WIN32 is true for any Windows ABI,
# including obsolete 32-bit toolchains.)
set(GITTYUP_PACK_WIN64 OFF)
if(WIN32)
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Gittyup packaging: Windows is 64-bit (x64) only.")
  endif()
  set(GITTYUP_PACK_WIN64 ON)
endif()

# Install Qt plugins. The icons of the QML views are SVG files.
set(QT_PLUGINS QJpegPlugin QSvgPlugin QSvgIconPlugin)

if(APPLE)
  set(QT_PLUGINS ${QT_PLUGINS} QCocoaIntegrationPlugin QMacStylePlugin)
elseif(GITTYUP_PACK_WIN64)
  # Schannel is the TLS backend of HTTPS requests, like the pull requests.
  set(QT_PLUGINS ${QT_PLUGINS} QWindowsIntegrationPlugin
                 QModernWindowsStylePlugin QSchannelBackendPlugin)
else()
  set(QT_PLUGINS ${QT_PLUGINS} QXcbIntegrationPlugin
                 QComposePlatformInputContextPlugin)
endif()

if(GITTYUP_PACK_WIN64)
  set(INSTALL_LIBDIR ${CMAKE_INSTALL_BINDIR})
elseif(NOT APPLE)
  set(INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
else()
  set(INSTALL_LIBDIR ${CONTENTS_DIR})
endif()

if(FLATPAK)
  qt_import_plugins(gittyup INCLUDE ${QT_PLUGINS})
elseif(NOT USE_SYSTEM_QT)
  foreach(QT_PLUGIN ${QT_PLUGINS})
    if(NOT TARGET Qt6::${QT_PLUGIN})
      continue()
    endif()

    get_target_property(PLUGIN Qt6::${QT_PLUGIN} LOCATION_${CMAKE_BUILD_TYPE})
    get_filename_component(FILE_NAME ${PLUGIN} NAME)
    get_filename_component(DIR ${PLUGIN} DIRECTORY)
    get_filename_component(DIR_NAME ${DIR} NAME)
    set(PLUGIN_PATH Plugins/${DIR_NAME}/${FILE_NAME})

    install(
      FILES ${PLUGIN}
      DESTINATION ${INSTALL_LIBDIR}/Plugins/${DIR_NAME}
      PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE
      COMPONENT ${GITTYUP_NAME})

    if(GITTYUP_PACK_WIN64)
      # Copy into build dir.
      execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PLUGIN}
                              ${CMAKE_BINARY_DIR}/${PLUGIN_PATH})
    elseif(APPLE)
      # Sign plugin.
      if(CODESIGN_IDENTITY)
        install(
          CODE "execute_process(COMMAND
            codesign --timestamp --options runtime
            -s \"${CODESIGN_IDENTITY}\"
            \${CMAKE_INSTALL_PREFIX}/${CONTENTS_DIR}/${PLUGIN_PATH}
          )"
          COMPONENT ${GITTYUP_NAME})
      endif()
    else()
      # Delete runpath.
      install(CODE "execute_process(COMMAND chrpath --delete
          \${CMAKE_INSTALL_PREFIX}/${PLUGIN_PATH}
        )" COMPONENT ${GITTYUP_NAME})
    endif()
  endforeach()

  # Install Qt libraries.
  foreach(QT_MODULE ${QT_MODULES})
    get_target_property(QT_LIBRARY Qt6::${QT_MODULE}
                        LOCATION_${CMAKE_BUILD_TYPE})

    if(APPLE)
      get_filename_component(QT_FRAMEWORK ${QT_LIBRARY} DIRECTORY)

      # FIXME: QT_LIBRARY points directly to the library bin instead of the
      # symlink 2 directories above (since qt6)
      string(REGEX REPLACE "/Versions/.*" "" QT_FRAMEWORK "${QT_FRAMEWORK}")

      set(DEST_DIR ${CONTENTS_DIR}/Frameworks)
      install(
        DIRECTORY ${QT_FRAMEWORK}
        DESTINATION ${DEST_DIR}
        COMPONENT ${GITTYUP_NAME}
        PATTERN "*.prl" EXCLUDE
        PATTERN "*_debug" EXCLUDE
        PATTERN "Headers" EXCLUDE
        PATTERN ".DS_Store" EXCLUDE)
    else()
      if(GITTYUP_PACK_WIN64)
        get_filename_component(TARGET_NAME ${QT_LIBRARY} NAME)

        # Copy into build dir.
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                ${QT_LIBRARY} ${CMAKE_BINARY_DIR})
      else()
        # Strip minor and patch version extensions.
        get_filename_component(TARGET_NAME_WE ${QT_LIBRARY} NAME_WE)
        set(TARGET_NAME ${TARGET_NAME_WE}.so.6)
      endif()

      install(
        FILES ${QT_LIBRARY}
        DESTINATION ${INSTALL_LIBDIR}
        PERMISSIONS
          OWNER_READ
          OWNER_WRITE
          OWNER_EXECUTE
          GROUP_READ
          GROUP_EXECUTE
          WORLD_READ
          WORLD_EXECUTE
        COMPONENT ${GITTYUP_NAME}
        RENAME ${TARGET_NAME})

      if(NOT GITTYUP_PACK_WIN64)
        # Delete runpath.
        install(CODE "execute_process(COMMAND chrpath --delete
            \${CMAKE_INSTALL_PREFIX}/${TARGET_NAME}
          )" COMPONENT ${GITTYUP_NAME})
      endif()
    endif()
  endforeach()

  # Install the QML modules that the views import, and the modules they import
  # in turn, into the default import path of qt.conf.
  if(GITTYUP_PACK_WIN64)
    set(QML_DIR ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML})
    foreach(QML_MODULE QML QtQml QtQuick)
      install(
        DIRECTORY ${QML_DIR}/${QML_MODULE}
        DESTINATION ${INSTALL_LIBDIR}/qml
        COMPONENT ${GITTYUP_NAME}
        # Modules and styles that aren't used.
        PATTERN "XmlListModel" EXCLUDE
        PATTERN "Dialogs" EXCLUDE
        PATTERN "Effects" EXCLUDE
        PATTERN "LocalStorage" EXCLUDE
        PATTERN "NativeStyle" EXCLUDE
        PATTERN "Particles" EXCLUDE
        PATTERN "Shapes" EXCLUDE
        PATTERN "VectorImage" EXCLUDE
        PATTERN "tooling" EXCLUDE
        PATTERN "designer" EXCLUDE
        PATTERN "FluentWinUI3" EXCLUDE
        PATTERN "Fusion" EXCLUDE
        PATTERN "Imagine" EXCLUDE
        PATTERN "Material" EXCLUDE
        PATTERN "Universal" EXCLUDE
        PATTERN "Windows" EXCLUDE
        PATTERN "*.debug" EXCLUDE
        PATTERN "*.pdb" EXCLUDE)
    endforeach()
  endif()
endif()

# Install SSL libraries.
if(NOT APPLE)
  if(NOT USE_SYSTEM_OPENSSL)
    if(GITTYUP_PACK_WIN64)
      set(SSL_LIB_SUFFIX "-3-x64.dll")
    else()
      set(SSL_LIB_SUFFIX ".so.3")
    endif()

    foreach(SSL_LIB_NAME ssl crypto)
      set(SSL_LIB "${OPENSSL_DIR}/lib${SSL_LIB_NAME}${SSL_LIB_SUFFIX}")

      # Copy into build dir.
      execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SSL_LIB}
                              ${CMAKE_BINARY_DIR})

      install(
        FILES ${SSL_LIB}
        DESTINATION ${INSTALL_LIBDIR}
        PERMISSIONS
          OWNER_READ
          OWNER_WRITE
          OWNER_EXECUTE
          GROUP_READ
          GROUP_EXECUTE
          WORLD_READ
          WORLD_EXECUTE
        COMPONENT ${GITTYUP_NAME})
    endforeach()
  endif()

  # libssh2 (submodule) installs its DLL without COMPONENT ${GITTYUP_NAME};
  # CPack only installs that component, so ship the DLL from Gittyup's pack
  # rules.
  if(GITTYUP_PACK_WIN64
     AND NOT USE_SYSTEM_LIBSSH2
     AND TARGET libssh2_shared)
    install(
      FILES $<TARGET_FILE:libssh2_shared>
      DESTINATION ${INSTALL_LIBDIR}
      PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE
      COMPONENT ${GITTYUP_NAME})
  endif()

  if(GITTYUP_PACK_WIN64 OR APPLE)
    # Install config files.
    set(QT_CONF ${CONF_DIR}/qt.conf)
    install(
      FILES ${QT_CONF}
      DESTINATION
        $<${MAC}:${RESOURCES_DIR}>$<$<PLATFORM_ID:Windows>:${INSTALL_LIBDIR}>
      COMPONENT ${GITTYUP_NAME})
  endif()
endif()

# Install the libraries that the installed executables and plugins load and that
# Windows doesn't provide, like the Qt libraries of the QML modules and the
# runtime of MinGW.
if(GITTYUP_PACK_WIN64 AND NOT USE_SYSTEM_QT)
  get_target_property(QT_CORE_LIBRARY Qt6::Core LOCATION_${CMAKE_BUILD_TYPE})
  get_filename_component(QT_BIN_DIR ${QT_CORE_LIBRARY} DIRECTORY)
  install(
    CODE "
    set(GITTYUP_LIBRARY_DIR \"${INSTALL_LIBDIR}\")
    set(GITTYUP_SEARCH_DIRS \"${QT_BIN_DIR}\")
    set(GITTYUP_MINGW \"${MINGW}\")
    set(GITTYUP_OBJDUMP \"${CMAKE_OBJDUMP}\")
    set(GITTYUP_STRIP \"${CMAKE_STRIP}\")"
    COMPONENT ${GITTYUP_NAME})
  install(
    CODE [[
    if(GITTYUP_MINGW)
      set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
      set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
      set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${GITTYUP_OBJDUMP}")
    endif()

    # Windows ignores the case of library names, but on other systems
    # file(GET_RUNTIME_DEPENDENCIES) misses libraries with upper case letters
    # in their names, like Qt6Core.dll, so look them up by their lower case
    # names.
    foreach(DIR ${GITTYUP_SEARCH_DIRS})
      file(GLOB FILES "${DIR}/*.dll")
      foreach(FILE ${FILES})
        get_filename_component(NAME "${FILE}" NAME)
        string(TOLOWER "${NAME}" NAME)
        if(NOT DEFINED "CANDIDATE_${NAME}")
          set("CANDIDATE_${NAME}" "${FILE}")
        endif()
      endforeach()
    endforeach()

    # Install the libraries until the installed files need no more.
    set(LIBRARY_DIR "${CMAKE_INSTALL_PREFIX}/${GITTYUP_LIBRARY_DIR}")
    while(TRUE)
      file(GLOB_RECURSE EXECUTABLES "${LIBRARY_DIR}/*.exe")
      file(GLOB_RECURSE MODULES "${LIBRARY_DIR}/*.dll")
      file(
        GET_RUNTIME_DEPENDENCIES
        EXECUTABLES ${EXECUTABLES}
        MODULES ${MODULES}
        DIRECTORIES ${GITTYUP_SEARCH_DIRS}
        RESOLVED_DEPENDENCIES_VAR FOUND
        UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED
        CONFLICTING_DEPENDENCIES_PREFIX CONFLICT
        PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
        POST_EXCLUDE_REGEXES "[Ss]ystem32")

      # Libraries found more than once are installed once.
      foreach(NAME ${CONFLICT_FILENAMES})
        list(GET "CONFLICT_${NAME}" 0 FILE)
        list(APPEND FOUND "${FILE}")
      endforeach()

      set(MISSING)
      foreach(NAME ${UNRESOLVED})
        string(TOLOWER "${NAME}" LOWER)
        if(DEFINED "CANDIDATE_${LOWER}")
          list(APPEND FOUND "${CANDIDATE_${LOWER}}")
        else()
          list(APPEND MISSING "${NAME}")
        endif()
      endforeach()

      set(NEW)
      foreach(FILE ${FOUND})
        get_filename_component(NAME "${FILE}" NAME)
        if(NOT EXISTS "${LIBRARY_DIR}/${NAME}")
          list(APPEND NEW "${FILE}")
        endif()
      endforeach()

      if(NOT NEW)
        break()
      endif()

      list(REMOVE_DUPLICATES NEW)
      file(INSTALL ${NEW} DESTINATION "${LIBRARY_DIR}")

      # Some libraries of MinGW come with their debug information.
      if(CMAKE_INSTALL_DO_STRIP AND GITTYUP_MINGW)
        foreach(FILE ${NEW})
          get_filename_component(NAME "${FILE}" NAME)
          execute_process(COMMAND "${GITTYUP_STRIP}" --strip-unneeded
                                  "${LIBRARY_DIR}/${NAME}")
        endforeach()
      endif()
    endwhile()

    if(MISSING)
      list(REMOVE_DUPLICATES MISSING)
      list(JOIN MISSING " " MISSING)
      message(STATUS "Libraries expected from Windows: ${MISSING}")
    endif()
    ]]
    COMPONENT ${GITTYUP_NAME})
endif()

# The bundled libssh2 is shared and referenced as @rpath/libssh2.1.dylib, so it
# must be shipped in Frameworks (as the real file under its soname).
if(APPLE
   AND NOT USE_SYSTEM_LIBSSH2
   AND TARGET libssh2_shared)
  install(
    FILES $<TARGET_FILE:libssh2_shared>
    DESTINATION ${CONTENTS_DIR}/Frameworks
    RENAME $<TARGET_SONAME_FILE_NAME:libssh2_shared>
    PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE
    COMPONENT ${GITTYUP_NAME})
endif()

if(UNIX AND NOT APPLE)
  foreach(icon 16 32 64 128 256 512)
    install(
      FILES ${RSRC_DIR}/Gittyup.iconset/icon_${icon}x${icon}.png
      DESTINATION share/icons/hicolor/${icon}x${icon}/apps
      COMPONENT ${GITTYUP_NAME}
      RENAME ${GITTYUP_EXECUTABLE_NAME}.png)
  endforeach()
  install(
    FILES ${RSRC_DIR}/Gittyup.iconset/gittyup_logo.svg
    DESTINATION share/icons/hicolor/scalable/apps
    COMPONENT ${GITTYUP_NAME}
    RENAME ${GITTYUP_EXECUTABLE_NAME}.svg)
endif()

# Sign bundle on macOS.
if(APPLE AND CODESIGN_IDENTITY)
  install(
    CODE "execute_process(COMMAND
      codesign --deep --timestamp --options runtime
        -s \"${CODESIGN_IDENTITY}\"
        \${CMAKE_INSTALL_PREFIX}/${GITTYUP_EXECUTABLE_NAME}.app
    )"
    COMPONENT ${GITTYUP_NAME})
endif()

# Add package rule.
if(APPLE)
  set(CPACK_GENERATOR DragNDrop)
  set(CPACK_DMG_VOLUME_NAME ${GITTYUP_NAME})
  set(CPACK_DMG_DS_STORE ${RSRC_DIR}/DS_Store)
  set(CPACK_DMG_BACKGROUND_IMAGE ${RSRC_DIR}/dmg-background.png)
elseif(GITTYUP_PACK_WIN64)
  set(CPACK_GENERATOR NSIS)
  set(CPACK_NSIS_CREATE_ICONS_EXTRA
      "CreateShortCut \
    \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${GITTYUP_NAME}.lnk\\\" \
    \\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\""
  )
  set(CPACK_NSIS_EXECUTABLES_DIRECTORY ${CMAKE_INSTALL_BINDIR})
  # The installer doesn't offer to run Gittyup when it finishes: it would run
  # with the rights of the installer, often as another administrator, who
  # doesn't own the repositories of the user.

  # Builds of MinGW ship its runtime instead.
  if(MSVC)
    install(
      FILES ${RSRC_DIR}/vcredist_x64.exe
      DESTINATION .
      COMPONENT ${GITTYUP_NAME})
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        "ExecWait '\\\"$INSTDIR\\\\vcredist_x64.exe\\\" /q /norestart'")
  endif()
  set(PLATFORM "-win64")
else()
  set(CPACK_GENERATOR TGZ)
endif()

if(DEV_BUILD)
  set(BUILD_TAG "-dev")
else()
  set(BUILD_TAG "")
endif()
set(PACKAGE_FILE_NAME
    "${GITTYUP_NAME}${PLATFORM}-${GITTYUP_VERSION}${BUILD_TAG}")

set(CPACK_MONOLITHIC_INSTALL true)

# MSVC keeps debug information in separate files, MinGW in the binaries.
if(MINGW)
  set(CPACK_STRIP_FILES true)
endif()
set(CPACK_INSTALL_CMAKE_PROJECTS ${CMAKE_BINARY_DIR} ${GITTYUP_NAME}
                                 ${GITTYUP_NAME} /)

# Add Context Menu Shortcuts
if(GITTYUP_PACK_WIN64)
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
      "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
    WriteRegStr HKCR 'Directory\\\\Background\\\\shell\\\\${GITTYUP_NAME}' '' 'Open with ${GITTYUP_NAME}'
    WriteRegStr HKCR 'Directory\\\\Background\\\\shell\\\\${GITTYUP_NAME}' 'Icon' '\\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\"'
    WriteRegStr HKCR 'Directory\\\\Background\\\\shell\\\\${GITTYUP_NAME}\\\\command' '' '\\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\" \\\"%V\\\"'
    WriteRegStr HKCR 'Directory\\\\shell\\\\${GITTYUP_NAME}' '' 'Open with ${GITTYUP_NAME}'
    WriteRegStr HKCR 'Directory\\\\shell\\\\${GITTYUP_NAME}' 'Icon' '\\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\"'
    WriteRegStr HKCR 'Directory\\\\shell\\\\${GITTYUP_NAME}\\\\command' '' '\\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\" \\\"%L\\\"'"
  )
  set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
      "${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}
    DeleteRegKey HKCR 'Directory\\\\Background\\\\shell\\\\${GITTYUP_NAME}'
    DeleteRegKey HKCR 'Directory\\\\shell\\\\${GITTYUP_NAME}'")
endif()

set(CPACK_PACKAGE_FILE_NAME ${PACKAGE_FILE_NAME})
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/pack)
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${GITTYUP_NAME})
set(CPACK_PACKAGE_VENDOR "Gittyup Contributors")
set(CPACK_PACKAGE_VERSION_MAJOR ${GITTYUP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GITTYUP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GITTYUP_VERSION_PATCH})

if(GITTYUP_PACK_WIN64)
  set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE.md)
else()
  install(
    FILES ${CMAKE_SOURCE_DIR}/LICENSE.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/${GITTYUP_NAME}
    RENAME LICENSE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
include(CPack)

if(GITTYUP_PACK_WIN64)
  set(PACKAGE_DIR ${CMAKE_BINARY_DIR}/${PACKAGE_FILE_NAME})

  # Add target to package and sign executable.
  add_custom_target(
    deploy
    signtool
    sign
    /v
    /f
    ${CERTIFICATE_FILE}
    /p
    ${CERTIFICATE_PASSWORD}
    ${CMAKE_BINARY_DIR}/${PACKAGE_FILE_NAME}.exe
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/${GITTYUP_EXECUTABLE_NAME}.exe ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/${GITTYUP_EXECUTABLE_NAME}.pdb ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/gittyup-indexer.exe
            ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/gittyup-indexer.pdb
            ${PACKAGE_DIR})

  add_dependencies(deploy package)
endif()
