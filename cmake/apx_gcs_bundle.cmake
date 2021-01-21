# cmake-format: off
function(apx_gcs_bundle target title)
    apx_parse_function_args(
        NAME apx_gcs_bundle
		ONE_VALUE
            ICON
		ARGN ${ARGN})
# cmake-format: on

    if(NOT APPLE)
        return()
    endif()

    set(copyright "(C) ${APX_GIT_YEAR} Aliaksei Stratsilatau &lt;sa@uavos.com&gt;")

    # bundle
    set_target_properties(
        ${target}
        PROPERTIES #OUTPUT_NAME "${title}"
                   #    RUNTIME_OUTPUT_NAME "${target}"
                   BUNDLE TRUE
                   MACOSX_BUNDLE TRUE
                   MACOSX_BUNDLE_BUNDLE_NAME ${title}
                   MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
                   MACOSX_BUNDLE_COPYRIGHT ${APX_COPYRIGHT}
                   MACOSX_BUNDLE_GUI_IDENTIFIER com.uavos.apx.${target}
                   MACOSX_FRAMEWORK_IDENTIFIER com.uavos.apx.${target}
                   MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
                   MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
                   MACOSX_BUNDLE_EXECUTABLE_NAME ${target}
    )

    set_target_properties(${target} PROPERTIES BUNDLE TRUE)

    if(ICON)
        set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON})
    endif()

    # # Rename generated executable binary to luminance-hdr
    # add_custom_command(
    #     TARGET ${target}
    #     POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E rename ${title}.app/Contents/MacOS/${title} ${title}.app/Contents/MacOS/${target}
    #     VERBATIM
    #     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    # )

    # based on code from CMake's QtDialog/CMakeLists.txt
    macro(install_qt5_plugin _qt_plugin_name _qt_plugins_var _prefix)
        get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
        if(EXISTS "${_qt_plugin_path}")
            get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
            get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
            get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
            set(_qt_plugin_dest "${_prefix}/PlugIns/${_qt_plugin_type}")
            install(FILES "${_qt_plugin_path}" DESTINATION "${_qt_plugin_dest}")
            set(${_qt_plugins_var} "${${_qt_plugins_var}};\$ENV{DEST_DIR}\${CMAKE_INSTALL_PREFIX}/${_qt_plugin_dest}/${_qt_plugin_file}")
        else()
            message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found")
        endif()
    endmacro()

    install_qt5_plugin("Qt5::QCocoaIntegrationPlugin" QT_PLUGINS ${prefix})

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf" "[Paths]\nPlugins = ${_qt_plugin_dir}\n")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf" DESTINATION "${INSTALL_RESOURCES_DIR}")

    # Destination paths below are relative to ${CMAKE_INSTALL_PREFIX}
    install(
        TARGETS ${TARGET}
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME COMPONENT Runtime
    )
    # RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR} COMPONENT Runtime

    # Note Mac specific extension .app
    set(APPS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${TARGET}.app")

    # Directories to look for dependencies
    set(DIRS "${CMAKE_BINARY_DIR}")
    list(APPEND DIRS "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

    # Path used for searching by FIND_XXX(), with appropriate suffixes added
    if(CMAKE_PREFIX_PATH)
        foreach(dir ${CMAKE_PREFIX_PATH})
            list(APPEND DIRS "${dir}/bin" "${dir}/lib")
        endforeach()
    endif()

    # Append Qt's lib folder which is two levels above Qt5Widgets_DIR
    list(APPEND DIRS "${Qt5Widgets_DIR}/../..")

    include(InstallRequiredSystemLibraries)

    # message(STATUS "APPS: ${APPS}")
    # message(STATUS "QT_PLUGINS: ${QT_PLUGINS}")
    # message(STATUS "DIRS: ${DIRS}")

    install(CODE "include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"${QT_PLUGINS}\" \"${DIRS}\")"
    )

    # Rename generated app
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E rm -Rf \"${TITLE}.app\" WORKING_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}\")")
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E rename \"${TARGET}.app\" \"${TITLE}.app\" WORKING_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}\")")
    # add_custom_command(
    #     TARGET ${target}
    #     POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E rename ${title}.app/Contents/MacOS/${title} ${title}.app/Contents/MacOS/${target}
    #     VERBATIM
    #     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    # )

endfunction()