cmake_minimum_required(VERSION 3.15)
project(OpenBongo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific sources
if(WIN32)
    set(PLATFORM_SOURCES 
        src/platform/WindowsKeyboardHook.cpp
        src/platform/WindowsMouseHook.cpp
    )
    set(PLATFORM_LIBS user32 ole32 shell32 oleaut32)
elseif(APPLE)
    set(PLATFORM_SOURCES src/platform/MacKeyboardHook.cpp)
    set(PLATFORM_LIBS "-framework ApplicationServices -framework Carbon")
elseif(UNIX)
    # Linux
    set(PLATFORM_SOURCES "")
    set(PLATFORM_LIBS "")
else()
    message(FATAL_ERROR "Unsupported platform. Only Windows, macOS, and Linux are supported.")
endif()

# ============================================================================
# Crow (C++ HTTP framework - header only)
# Located at: third_party/crow/
# License: BSD (free and open source)
# ============================================================================
find_path(CROW_INCLUDE_DIR
    NAMES crow.h
    PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/crow/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/crow"
        "C:/vcpkg/installed/x64-windows/include"
        "/usr/local/include"
        "/usr/include"
)

# Try to find Crow via vcpkg or package manager
find_package(Crow QUIET)

if(Crow_FOUND)
    message(STATUS "Found Crow via CMake package")
    set(CROW_FOUND TRUE)
elseif(CROW_INCLUDE_DIR)
    message(STATUS "Found Crow headers at: ${CROW_INCLUDE_DIR}")
    set(CROW_FOUND TRUE)
    
    # Crow requires asio (Asio standalone, not Boost.Asio)
    # Try to find asio via CMake package (vcpkg provides this)
    find_package(asio QUIET)
    
    if(asio_FOUND)
        message(STATUS "Found asio via CMake package")
        # asio package will be linked automatically
    else()
        # Try to find asio manually
        find_path(ASIO_INCLUDE_DIR
            NAMES asio.hpp
            PATHS
                "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include"
                "C:/vcpkg/installed/x64-windows/include"
                "/usr/local/include"
                "/usr/include"
        )
        
        if(ASIO_INCLUDE_DIR)
            message(STATUS "Found asio headers at: ${ASIO_INCLUDE_DIR}")
            # Include directory will be set after target is created
        else()
            message(WARNING "Asio not found. Crow requires asio (standalone, not Boost)")
            message(WARNING "Install via vcpkg: vcpkg install asio")
            message(WARNING "Or download from: https://think-async.com/Asio/")
        endif()
    endif()
else()
    message(FATAL_ERROR "Crow not found at third_party/crow/include/")
    message(FATAL_ERROR "Please ensure Crow is located at: third_party/crow/")
endif()

# ============================================================================
# webview library (cross-platform C++ library)
# Located at: third_party/webview/
# License: MIT (free and open source)
# ============================================================================
# Check if webview exists as a CMake subdirectory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/webview/CMakeLists.txt")
    message(STATUS "Found webview at third_party/webview/, adding as subdirectory")
    # Add webview as a subdirectory (it's a proper CMake library)
    # Disable building examples, tests, docs, etc.
    set(WEBVIEW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(WEBVIEW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(WEBVIEW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/third_party/webview" EXCLUDE_FROM_ALL)
    set(WEBVIEW_FOUND TRUE)
    message(STATUS "webview library configured via CMake")
else()
    message(FATAL_ERROR "webview not found at third_party/webview/")
    message(FATAL_ERROR "Please ensure webview is located at: third_party/webview/")
endif()

# Platform-specific libraries for webview are handled by webview::core target

# Try to find vcpkg and use it for SFML
if(WIN32)
    # Check for vcpkg in common locations
    if(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
        message(STATUS "Found vcpkg, using it for dependencies")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
        message(STATUS "Found local vcpkg, using it for dependencies")
    endif()
endif()

# Find SFML
# First, check if vcpkg is being used (check for vcpkg-installed SFML)
if(WIN32 AND EXISTS "C:/vcpkg/installed/x64-windows/share/sfml/SFMLConfig.cmake")
    set(SFML_DIR "C:/vcpkg/installed/x64-windows/share/sfml" CACHE PATH "")
    message(STATUS "Found SFML in vcpkg, using CONFIG mode")
    # vcpkg requires CONFIG mode
    find_package(SFML 3.0 COMPONENTS System Window Graphics Audio CONFIG QUIET)
    if(NOT SFML_FOUND)
        find_package(SFML 2.5 COMPONENTS system window graphics audio CONFIG QUIET)
    endif()
elseif(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/share/sfml/SFMLConfig.cmake")
    set(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/share/sfml" CACHE PATH "")
    message(STATUS "Found SFML in local vcpkg, using CONFIG mode")
    find_package(SFML 3.0 COMPONENTS System Window Graphics Audio CONFIG QUIET)
    if(NOT SFML_FOUND)
        find_package(SFML 2.5 COMPONENTS system window graphics audio CONFIG QUIET)
    endif()
else()
    # Try to find SFML using module mode (for manual installations)
    find_package(SFML 3.0 COMPONENTS System Window Graphics Audio QUIET)
    if(NOT SFML_FOUND)
        find_package(SFML 2.5 COMPONENTS system window graphics audio QUIET)
    endif()
endif()

# If still not found, try to help locate it
if(NOT SFML_FOUND)
    message(STATUS "SFML not found. Checking common locations...")
    
    # Try common installation paths
    if(WIN32)
        # Try manual installation
        if(EXISTS "C:/SFML/lib/cmake/SFML")
            set(SFML_DIR "C:/SFML/lib/cmake/SFML" CACHE PATH "")
            message(STATUS "Found SFML in C:/SFML")
            find_package(SFML 3.0 COMPONENTS System Window Graphics Audio QUIET)
            if(NOT SFML_FOUND)
                find_package(SFML 2.5 COMPONENTS system window graphics audio QUIET)
            endif()
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SFML/lib/cmake/SFML")
            set(SFML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SFML/lib/cmake/SFML" CACHE PATH "")
            message(STATUS "Found SFML in project directory")
            find_package(SFML 3.0 COMPONENTS System Window Graphics Audio QUIET)
            if(NOT SFML_FOUND)
                find_package(SFML 2.5 COMPONENTS system window graphics audio QUIET)
            endif()
        endif()
    endif()
endif()

# If still not found, provide helpful error
if(NOT SFML_FOUND)
    message(FATAL_ERROR "\n"
        "SFML not found!\n\n"
        "To install SFML on Windows:\n"
        "  1. Install vcpkg (recommended):\n"
        "     git clone https://github.com/Microsoft/vcpkg.git C:\\vcpkg\n"
        "     cd C:\\vcpkg\n"
        "     .\\bootstrap-vcpkg.bat\n"
        "     .\\vcpkg install sfml:x64-windows\n"
        "  2. Or download SFML manually from https://www.sfml-dev.org/download.php\n"
        "     Extract to C:\\SFML and run cmake again\n"
        "  3. Or run the setup script: .\\setup_vcpkg.bat\n"
    )
endif()

# Source files
set(SOURCES
    src/core/main.cpp
    src/core/BongoCat.cpp
    src/core/BongoStats.cpp
    src/config/CatPackConfig.cpp
    src/managers/CatPackManager.cpp
    src/audio/BonkPackConfig.cpp
    src/audio/BonkPackManager.cpp
    src/audio/EntitySFXConfig.cpp
    src/audio/EntitySFXManager.cpp
    src/utils/CounterEncryption.cpp
    src/config/HatConfig.cpp
    src/managers/HatManager.cpp
    src/input/KeyboardHook.cpp
    src/input/MouseHook.cpp
    src/utils/ImageHelper.cpp
    src/ui/WebViewWindow.cpp
    ${PLATFORM_SOURCES}
)

# Windows resource file for icon
if(WIN32)
    # Check if icon.ico exists, if not, try to copy from media/icon.jpg location
    # Note: icon.ico must be manually created from icon.jpg using convert_icon.bat or similar
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/resources/icon.ico")
        set(SOURCES ${SOURCES} src/resources/OpenBongo.rc)
        message(STATUS "Found icon.ico, will embed in executable")
    else()
        message(WARNING "icon.ico not found in src/resources/ directory. Run convert_icon.bat to create it from media/icon.jpg")
        # Still add the resource file, but it will fail to compile without the icon
        set(SOURCES ${SOURCES} src/resources/OpenBongo.rc)
    endif()
endif()

# Header files
set(HEADERS
    src/core/BongoCat.h
    src/input/KeyboardHook.h
    src/input/MouseHook.h
    src/ui/WebViewWindow.h
    src/platform/WindowsKeyboardHook.h
    src/platform/WindowsMouseHook.h
    src/platform/MacKeyboardHook.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Organize build output by platform
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/Windows"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/Windows"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/Windows"
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/Mac"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/Mac"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/Mac"
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/Linux"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/Linux"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/Linux"
    )
endif()

# Link libraries (SFML 3.0 and 2.5 both use SFML:: namespace)
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    SFML::System 
    SFML::Window 
    SFML::Graphics
    SFML::Audio
)

# Link webview library (CMake target from subdirectory)
if(WEBVIEW_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE webview::core)
    message(STATUS "Linking webview::core")
endif()

# Link asio if found (required by Crow)
if(asio_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE asio::asio)
    message(STATUS "Linking asio for Crow")
endif()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PLATFORM_LIBS})
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PLATFORM_LIBS})
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PLATFORM_LIBS})
    # WebKitGTK is handled by webview::core target
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Add Crow include directory if found manually (not via package)
if(CROW_FOUND AND CROW_INCLUDE_DIR AND NOT Crow_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CROW_INCLUDE_DIR})
endif()

# Add asio include directory if found manually (not via package)
if(ASIO_INCLUDE_DIR AND NOT asio_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ASIO_INCLUDE_DIR})
endif()

# Copy SFML DLLs and dependencies on Windows
if(WIN32)
    # Copy DLLs to same directory as executable (Windows loads DLLs from exe directory)
    # Also copy to libs folder for organization, but exe directory takes priority
    set(EXE_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    set(DLL_DIR "${EXE_DIR}/libs")
    
    # Create libs directory for organization (optional, but kept for reference)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DLL_DIR}"
        COMMENT "Creating libs directory for DLLs"
    )
    
    # Copy SFML DLLs to executable directory (required for loading)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SFML::System>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Audio>
        "${EXE_DIR}"
        COMMENT "Copying SFML DLLs to executable directory"
    )
    
    # Also copy to libs folder for organization
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SFML::System>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Audio>
        "${DLL_DIR}"
    )
    
    # Copy all dependency DLLs from vcpkg (if using vcpkg)
    # This ensures all required DLLs are copied, including transitive dependencies
    if(EXISTS "C:/vcpkg/installed/x64-windows/bin")
        set(VCPKG_BIN_DIR "C:/vcpkg/installed/x64-windows/bin")
        # Copy all DLLs from vcpkg bin directory
        file(GLOB VCPKG_DLLS "${VCPKG_BIN_DIR}/*.dll")
        foreach(DLL ${VCPKG_DLLS})
            get_filename_component(DLL_NAME ${DLL} NAME)
            # Copy to executable directory (required)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                "${EXE_DIR}"
                COMMENT "Copying ${DLL_NAME} to executable directory"
            )
            # Also copy to libs folder (for organization)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                "${DLL_DIR}"
            )
        endforeach()
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/bin")
        set(VCPKG_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/bin")
        # Copy all DLLs from vcpkg bin directory
        file(GLOB VCPKG_DLLS "${VCPKG_BIN_DIR}/*.dll")
        foreach(DLL ${VCPKG_DLLS})
            get_filename_component(DLL_NAME ${DLL} NAME)
            # Copy to executable directory (required)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                "${EXE_DIR}"
                COMMENT "Copying ${DLL_NAME} to executable directory"
            )
            # Also copy to libs folder (for organization)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                "${DLL_DIR}"
            )
        endforeach()
    endif()
    
    # Copy catpacks folder to executable directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/catpacks"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/catpacks"
        COMMENT "Copying catpacks folder to output directory"
    )
    
    # Copy hats folder to executable directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/hats"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/hats"
        COMMENT "Copying hats folder to output directory"
    )
    
    # Copy sfx folder to executable directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/sfx"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/sfx"
        COMMENT "Copying sfx folder to output directory"
    )
    
    # Copy UI folder to executable directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/ui"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/ui"
        COMMENT "Copying UI folder to output directory"
    )
endif()

# Copy UI folder on all platforms (not just Windows)
if(APPLE OR UNIX)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/ui"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/ui"
        COMMENT "Copying UI folder to output directory"
    )
endif()

# ============================================================================
# CPack Configuration for Creating Installers
# ============================================================================

# Set installer output directory to Release/Installers folder (must be before include(CPack))
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/Release/Installers")

# Set CPack variables
set(CPACK_PACKAGE_NAME "OpenBongo")
set(CPACK_PACKAGE_VENDOR "OpenBongo")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Bongo Cat - Animated desktop companion")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenBongo")
# Only set license file if it exists (optional)
if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()
set(CPACK_PACKAGE_EXECUTABLES "OpenBongo" "OpenBongo")

# Include CPack (reads all the variables set above)
include(CPack)

# Use both NSIS and WiX generators for Windows (creates .exe and .msi installers)
if(WIN32)
    # Generate both NSIS (.exe) and WiX (.msi) installers
    set(CPACK_GENERATOR "NSIS;WIX")
    
    # ============================================================================
    # NSIS Configuration (.exe installer)
    # ============================================================================
    set(CPACK_NSIS_PACKAGE_NAME "OpenBongo")
    set(CPACK_NSIS_DISPLAY_NAME "OpenBongo")
    set(CPACK_NSIS_HELP_LINK "https://github.com/yourusername/OpenBongo")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/yourusername/OpenBongo")
    set(CPACK_NSIS_CONTACT "your-email@example.com")
    set(CPACK_NSIS_MODIFY_PATH ON)  # Option to add to PATH (not needed for this app)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    
    # Create Start Menu shortcut
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortcut '$SMPROGRAMS\\\\$START_MENU_FOLDER\\\\OpenBongo.lnk' '$INSTDIR\\\\bin\\\\OpenBongo.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$SMPROGRAMS\\\\$START_MENU_FOLDER\\\\OpenBongo.lnk'"
    )
    
    # Install to Program Files
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    
    # ============================================================================
    # WiX Configuration (.msi installer)
    # ============================================================================
    # WiX package settings
    set(CPACK_WIX_PACKAGE_NAME "OpenBongo")
    set(CPACK_WIX_PACKAGE_VENDOR "OpenBongo")
    
    # Generate unique GUIDs for product and upgrade (you can generate new ones using guidgen.exe or online)
    # These GUIDs identify your product uniquely in Windows
    set(CPACK_WIX_PRODUCT_GUID "A1B2C3D4-E5F6-7890-ABCD-EF1234567890")
    set(CPACK_WIX_UPGRADE_GUID "B2C3D4E5-F6A7-8901-BCDE-F12345678901")
    
    # WiX UI settings (modern UI with directory selection)
    set(CPACK_WIX_UI_REF "WixUI_InstallDir")
    
    # WiX properties for Add/Remove Programs
    set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://github.com/yourusername/OpenBongo")
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/yourusername/OpenBongo")
    set(CPACK_WIX_PROPERTY_ARPCONTACT "your-email@example.com")
    
    # WiX architecture (x64 for 64-bit)
    set(CPACK_WIX_ARCHITECTURE "x64")
    
    # WiX will automatically create Start Menu shortcuts for executables
    # The shortcut will be created based on CPACK_PACKAGE_EXECUTABLES
endif()

# Install rules - define what gets installed and where
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT applications
)

# Install DLLs to bin directory (same as executable) for proper loading
if(WIN32)
    # Install SFML DLLs to bin directory (Windows loads from exe directory)
    install(FILES
        $<TARGET_FILE:SFML::System>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Audio>
        DESTINATION bin
        COMPONENT libraries
    )
    
    # Also install to libs folder for organization
    install(FILES
        $<TARGET_FILE:SFML::System>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Audio>
        DESTINATION bin/libs
        COMPONENT libraries
    )
    
    # Install vcpkg DLLs if they exist
    if(EXISTS "C:/vcpkg/installed/x64-windows/bin")
        file(GLOB VCPKG_DLLS "C:/vcpkg/installed/x64-windows/bin/*.dll")
        install(FILES ${VCPKG_DLLS}
            DESTINATION bin
            COMPONENT libraries
        )
        install(FILES ${VCPKG_DLLS}
            DESTINATION bin/libs
            COMPONENT libraries
        )
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/bin")
        file(GLOB VCPKG_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/bin/*.dll")
        install(FILES ${VCPKG_DLLS}
            DESTINATION bin
            COMPONENT libraries
        )
        install(FILES ${VCPKG_DLLS}
            DESTINATION bin/libs
            COMPONENT libraries
        )
    endif()
endif()

# Install catpacks, hats, and sfx folders
install(DIRECTORY "${CMAKE_SOURCE_DIR}/catpacks"
    DESTINATION bin
    COMPONENT data
)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/hats"
    DESTINATION hats
    FILES_MATCHING PATTERN "*"
)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/sfx"
    DESTINATION bin
    COMPONENT data
)

# Create empty logs and stats directories in installation
install(CODE "
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/bin/logs)
    file(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/bin/stats)
" COMPONENT data)
