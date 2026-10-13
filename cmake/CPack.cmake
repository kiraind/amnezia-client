# Package identity comes from the client branding (client/cmake/branding/common.cmake) so this build
# installs side by side with the official AmneziaVPN client. The CPack component stays named
# "AmneziaVPN": it is only an internal id (IFW package dir, WiX fragment ids), never shown or shared.
set(CPACK_PACKAGE_NAME              ${CLIENT_APPLICATION_NAME})
set(CPACK_PACKAGE_VENDOR            ${CLIENT_ORGANIZATION_NAME})
# Numeric MAJOR.MINOR.(PATCH*1000+LESHA_VERSION): grows with every fork release, so the WiX
# MajorUpgrade (same CPACK_WIX_UPGRADE_GUID, MSI compares only the first three fields) replaces
# an older fork install. The human-readable LESHA_FULL_VERSION goes into the file names.
set(CPACK_PACKAGE_VERSION           ${LESHA_NUMERIC_VERSION})
if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "${CLIENT_APPLICATION_NAME}_${LESHA_FULL_VERSION}_windows_x64")
elseif(APPLE AND NOT IOS AND NOT MACOS_NE)
    set(CPACK_PACKAGE_FILE_NAME "${CLIENT_APPLICATION_NAME}_${LESHA_FULL_VERSION}_macos_x64")
elseif(LINUX AND NOT ANDROID)
    set(CPACK_PACKAGE_FILE_NAME "${CLIENT_APPLICATION_NAME}_${LESHA_FULL_VERSION}_linux_x64")
endif()
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CLIENT_APPLICATION_NAME})
set(CPACK_PACKAGE_EXECUTABLES       ${CLIENT_APPLICATION_NAME} "${CLIENT_DISPLAY_NAME}")
set(CPACK_PRE_BUILD_SCRIPTS         ${CMAKE_CURRENT_LIST_DIR}/sign_binaries.cmake)
set(CPACK_POST_BUILD_SCRIPTS        ${CMAKE_CURRENT_LIST_DIR}/sign_packages.cmake)
set(CPACK_PROJECT_CONFIG_FILE       ${CMAKE_CURRENT_LIST_DIR}/CPackOptions.cmake)
# read by sign_binaries.cmake at package time
set(CPACK_AMNEZIA_CLIENT_EXEC_NAME  ${CLIENT_APPLICATION_NAME})
set(CPACK_AMNEZIA_SERVICE_EXEC_NAME ${CLIENT_SERVICE_NAME})
set(CPACK_RESOURCE_FILE_LICENSE     ${CMAKE_SOURCE_DIR}/deploy/data/LICENSE.txt)

list(PREPEND CPACK_COMPONENTS_ALL AmneziaVPN)

if(APPLE)
    set(CPACK_GENERATOR productbuild)
else()
    set(CPACK_GENERATOR IFW)
endif()

# === CPack IFW generator settings ===
# Name drives the default target dir, maintenancetool lookup and the service name in the QIF scripts
set(CPACK_IFW_PACKAGE_NAME                          ${CLIENT_APPLICATION_NAME})
set(CPACK_IFW_PACKAGE_TITLE                         "${CLIENT_DISPLAY_NAME}")
set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_WIDTH          600)
set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_HEIGHT         380)
set(CPACK_IFW_PACKAGE_WIZARD_STYLE                  Modern)
set(CPACK_IFW_PACKAGE_REMOVE_TARGET_DIR             ON)
set(CPACK_IFW_PACKAGE_ALLOW_SPACE_IN_PATH           ON)
set(CPACK_IFW_PACKAGE_ALLOW_NON_ASCII_CHARACTERS    ON)
set(CPACK_IFW_PACKAGE_CONTROL_SCRIPT                ${CMAKE_SOURCE_DIR}/deploy/installer/qif/controlscript.js)

# === CPack WIX generator settings ===
set(CPACK_WIX_VERSION               4)
# Own upgrade code (NOT the official client's {2D55AC62-...}) so MSI never upgrades/replaces it
set(CPACK_WIX_UPGRADE_GUID          "{25866F49-DD8F-4DC5-AA76-1046D9953D0A}")
set(CPACK_WIX_PRODUCT_ICON          ${CMAKE_SOURCE_DIR}/client/images/app.ico)
set(CPACK_WIX_CUSTOM_XMLNS          "util=http://wixtoolset.org/schemas/v4/wxs/util")
# WiX fragment id of the service exe: CM_CP_<component>.<file name with '-' -> '_'>
string(REPLACE "-" "_" _AMNEZIA_WIX_SERVICE_FILE_ID "${CLIENT_SERVICE_NAME}.exe")
configure_file(${CMAKE_SOURCE_DIR}/deploy/installer/wix/service_install_patch.xml.in
               ${CMAKE_BINARY_DIR}/deploy/installer/wix/service_install_patch.xml @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/deploy/installer/wix/close_client_patch.xml.in
               ${CMAKE_BINARY_DIR}/deploy/installer/wix/close_client_patch.xml @ONLY)
set(_AMNEZIA_WIX_PATCH_SERVICE      ${CMAKE_BINARY_DIR}/deploy/installer/wix/service_install_patch.xml)
set(_AMNEZIA_WIX_PATCH_CLOSE_APP    ${CMAKE_BINARY_DIR}/deploy/installer/wix/close_client_patch.xml)
file(TO_CMAKE_PATH                  "${_AMNEZIA_WIX_PATCH_SERVICE}" _AMNEZIA_WIX_PATCH_SERVICE_CMAKE)
file(TO_CMAKE_PATH                  "${_AMNEZIA_WIX_PATCH_CLOSE_APP}" _AMNEZIA_WIX_PATCH_CLOSE_APP_CMAKE)
list(APPEND CPACK_WIX_PATCH_FILE    "${_AMNEZIA_WIX_PATCH_SERVICE_CMAKE}" "${_AMNEZIA_WIX_PATCH_CLOSE_APP_CMAKE}")
list(APPEND CPACK_WIX_EXTENSIONS    "WixToolset.Util.wixext")

# === CPack productbuild generator settings ===
# deploy/data/<os>/*.in are templates filled from the branding vars
set(_AMNEZIA_DEPLOY_OUT ${CMAKE_BINARY_DIR}/deploy/data)
foreach(_f post_install.sh post_uninstall.sh)
    configure_file(${CMAKE_SOURCE_DIR}/deploy/data/macos/${_f}.in ${_AMNEZIA_DEPLOY_OUT}/macos/${_f}
                   @ONLY FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    configure_file(${CMAKE_SOURCE_DIR}/deploy/data/linux/${_f}.in ${_AMNEZIA_DEPLOY_OUT}/linux/${_f}
                   @ONLY FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endforeach()
configure_file(${CMAKE_SOURCE_DIR}/deploy/data/macos/service.plist.in
               ${_AMNEZIA_DEPLOY_OUT}/macos/${CLIENT_APPLICATION_NAME}.plist @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/deploy/data/linux/app.service.in
               ${_AMNEZIA_DEPLOY_OUT}/linux/${CLIENT_APPLICATION_NAME}.service @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/deploy/data/linux/app.desktop.in
               ${_AMNEZIA_DEPLOY_OUT}/linux/${CLIENT_APPLICATION_NAME}.desktop @ONLY)
foreach(_f post_install.cmd post_uninstall.cmd)
    configure_file(${CMAKE_SOURCE_DIR}/deploy/data/windows/${_f}.in ${_AMNEZIA_DEPLOY_OUT}/windows/${_f} @ONLY)
endforeach()

set(CPACK_PRODUCTBUILD_IDENTIFIER       ${CLIENT_APP_DOMAIN}.installer)
set(CPACK_PREFLIGHT_AMNEZIAVPN_SCRIPT   ${_AMNEZIA_DEPLOY_OUT}/macos/post_uninstall.sh)
set(CPACK_POSTFLIGHT_AMNEZIAVPN_SCRIPT  ${_AMNEZIA_DEPLOY_OUT}/macos/post_install.sh)
set(CPACK_POSTFLIGHT_UNINSTALL_SCRIPT   ${_AMNEZIA_DEPLOY_OUT}/macos/post_uninstall.sh)
# provide custom CPack.distribution.dist.in
list(APPEND CMAKE_MODULE_PATH           ${CMAKE_SOURCE_DIR}/deploy/data/macos)

if(LINUX AND NOT ANDROID)
    install(FILES
        ${_AMNEZIA_DEPLOY_OUT}/linux/${CLIENT_APPLICATION_NAME}.service
        ${_AMNEZIA_DEPLOY_OUT}/linux/${CLIENT_APPLICATION_NAME}.desktop
        ${_AMNEZIA_DEPLOY_OUT}/linux/post_install.sh
        ${_AMNEZIA_DEPLOY_OUT}/linux/post_uninstall.sh
        DESTINATION "."
        COMPONENT AmneziaVPN
    )
    install(FILES ${CMAKE_SOURCE_DIR}/deploy/data/linux/AmneziaVPN.png
        DESTINATION "."
        RENAME ${CLIENT_APPLICATION_NAME}.png
        COMPONENT AmneziaVPN
    )
endif()

if(WIN32)
    install(FILES
        ${_AMNEZIA_DEPLOY_OUT}/windows/post_install.cmd
        ${_AMNEZIA_DEPLOY_OUT}/windows/post_uninstall.cmd
        DESTINATION "."
        COMPONENT AmneziaVPN
    )

    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
    if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
            DESTINATION "."
            COMPONENT AmneziaVPN
        )
    else()
        message(WARNING "MSVC runtime libraries were not found, packages will not ship them")
    endif()
endif()

if (APPLE AND NOT IOS AND NOT MACOS_NE)
    install(FILES ${_AMNEZIA_DEPLOY_OUT}/macos/${CLIENT_APPLICATION_NAME}.plist
        DESTINATION "${CLIENT_APPLICATION_NAME}.app/Contents/Resources"
        COMPONENT AmneziaVPN
    )
endif()

include(CPackIFW)
cpack_ifw_configure_component(AmneziaVPN
    DISPLAY_NAME "${CLIENT_DISPLAY_NAME}"
    VERSION ${LESHA_NUMERIC_VERSION}
    RELEASE_DATE ${RELEASE_DATE}
    REQUIRES_ADMIN_RIGHTS
    FORCED_INSTALLATION
    SCRIPT ${CMAKE_SOURCE_DIR}/deploy/installer/qif/componentscript.js
)

include(CPack)
cpack_add_component(Uninstall
    DISPLAY_NAME "Uninstall ${CLIENT_DISPLAY_NAME}"
    REQUIRES_ADMIN_RIGHTS
    DISABLED
)
