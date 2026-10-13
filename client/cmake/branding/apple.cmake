if(NOT CLIENT_SWIFT_MODULE_NAME)
    set(CLIENT_SWIFT_MODULE_NAME "${CLIENT_APPLICATION_NAME}" CACHE STRING "Swift module name for Objective-C interop header")
endif()
if(NOT CLIENT_SWIFT_OBJC_HEADER_NAME)
    set(CLIENT_SWIFT_OBJC_HEADER_NAME "${CLIENT_SWIFT_MODULE_NAME}-Swift.h" CACHE STRING "Generated Swift Objective-C header name")
endif()
if(NOT CLIENT_NETWORK_EXTENSION_NAME)
    set(CLIENT_NETWORK_EXTENSION_NAME "${CLIENT_APPLICATION_NAME}NetworkExtension" CACHE STRING "Display name for Apple network extension targets")
endif()

if(NOT BUILD_OSX_APP_IDENTIFIER)
    set(BUILD_OSX_APP_IDENTIFIER "${CLIENT_APP_DOMAIN}" CACHE STRING "OSX Application identifier")
endif()
if(NOT BUILD_IOS_APP_IDENTIFIER)
    set(BUILD_IOS_APP_IDENTIFIER "${CLIENT_APP_DOMAIN}" CACHE STRING "iOS Application identifier")
endif()
if(NOT BUILD_IOS_GROUP_IDENTIFIER)
    set(BUILD_IOS_GROUP_IDENTIFIER "group.${CLIENT_APP_DOMAIN}" CACHE STRING "iOS Group identifier")
    # NOTE: the app group is also written literally in client/ios/**/*.entitlements and
    # client/macos/**/*.entitlements; keep them in sync when overriding this.
endif()
# Apple Developer Team ID used for signing. Deliberately empty in this fork (upstream used
# Amnezia's team); the fork owner passes their own: -DBUILD_VPN_DEVELOPMENT_TEAM=<TEAMID>
if(NOT BUILD_VPN_DEVELOPMENT_TEAM)
    set(BUILD_VPN_DEVELOPMENT_TEAM "" CACHE STRING "Apple Development Team ID (set by the fork owner)")
endif()
