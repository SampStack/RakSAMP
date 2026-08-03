cmake_minimum_required(VERSION 3.24)

project(RakSAMP
    VERSION 0.10.1
    DESCRIPTION "Cross-platform SA-MP 0.3.7 and 0.3DL fake client and development server"
    LANGUAGES C CXX)

include(CTest)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(RAKSAMP_BUILD_CLIENT "Build the headless fake client" ON)
option(RAKSAMP_BUILD_SERVER "Build the Lua development server" ON)
option(RAKSAMP_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(RAKSAMP_ENABLE_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "RAKSAMP_ENABLE_SANITIZERS requires GCC or Clang")
    endif()
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

if(BUILD_TESTING)
    add_executable(protocol-tests tests/protocol_tests.cpp)
    target_include_directories(protocol-tests PRIVATE common)
    add_test(NAME protocol-selection COMMAND protocol-tests)

    add_executable(raknet-container-tests tests/raknet_container_tests.cpp)
    target_include_directories(raknet-container-tests PRIVATE raknet)
    add_test(NAME raknet-containers COMMAND raknet-container-tests)

    add_executable(client-lifecycle-tests tests/client_lifecycle_tests.cpp)
    target_include_directories(client-lifecycle-tests PRIVATE client/src)
    add_test(NAME client-lifecycle COMMAND client-lifecycle-tests)

    add_executable(load-mode-options-tests
        tests/load_mode_options_tests.cpp
        client/src/load_mode_options.cpp)
    target_include_directories(load-mode-options-tests PRIVATE client/src common)
    add_test(NAME load-mode-options COMMAND load-mode-options-tests)

    add_executable(player-damage-tests
        tests/player_damage_tests.cpp
        client/src/player_damage.cpp)
    target_include_directories(player-damage-tests PRIVATE client/src common)
    add_test(NAME player-damage COMMAND player-damage-tests)

    add_executable(weapon-inventory-tests
        tests/weapon_inventory_tests.cpp)
    target_include_directories(weapon-inventory-tests PRIVATE client/src)
    add_test(NAME weapon-inventory COMMAND weapon-inventory-tests)

    add_executable(safe-parse-tests tests/safe_parse_tests.cpp)
    target_include_directories(safe-parse-tests PRIVATE common)
    add_test(NAME safe-parse COMMAND safe-parse-tests)

    add_executable(checked-reader-tests tests/checked_reader_tests.cpp)
    target_include_directories(checked-reader-tests PRIVATE client/src common)
    add_test(NAME checked-reader COMMAND checked-reader-tests)

    add_executable(automation-protocol-tests
        tests/automation_protocol_tests.cpp
        client/src/automation_protocol.cpp)
    target_include_directories(automation-protocol-tests PRIVATE client/src common)
    target_compile_definitions(automation-protocol-tests PRIVATE
        RAKSAMP_CLIENT POSIX_HEADLESS)
    add_test(NAME automation-protocol COMMAND automation-protocol-tests)

    add_executable(key-state-tests
        tests/key_state_tests.cpp
        client/src/key_state.cpp)
    target_include_directories(key-state-tests PRIVATE client/src common raknet/SAMP)
    add_test(NAME key-state COMMAND key-state-tests)

    add_executable(tinyxml-extension-tests
        tests/tinyxml_extension_tests.cpp
        tinyxml/tinystr.cpp
        tinyxml/tinyxml.cpp
        tinyxml/tinyxmlerror.cpp
        tinyxml/tinyxmlparser.cpp)
    target_include_directories(tinyxml-extension-tests PRIVATE tinyxml)
    add_test(NAME tinyxml-extensions COMMAND tinyxml-extension-tests)
endif()

set(RAKSAMP_RAKNET_SOURCES
    raknet/BitStream.cpp
    raknet/BitStream_NoTemplate.cpp
    raknet/CheckSum.cpp
    raknet/DataBlockEncryptor.cpp
    raknet/DataCompressor.cpp
    raknet/DS_ByteQueue.cpp
    raknet/DS_HuffmanEncodingTree.cpp
    raknet/DS_Table.cpp
    raknet/EncodeClassName.cpp
    raknet/GetTime.cpp
    raknet/InternalPacketPool.cpp
    raknet/NetworkIDGenerator.cpp
    raknet/NetworkTypes.cpp
    raknet/PluginInterface.cpp
    raknet/RakClient.cpp
    raknet/RakNetStatistics.cpp
    raknet/RakNetworkFactory.cpp
    raknet/RakPeer.cpp
    raknet/rakserver.cpp
    raknet/RakSleep.cpp
    raknet/Rand.cpp
    raknet/ReliabilityLayer.cpp
    raknet/rijndael.cpp
    raknet/RPCMap.cpp
    raknet/SHA1.cpp
    raknet/SimpleMutex.cpp
    raknet/SocketLayer.cpp
    raknet/StringCompressor.cpp
    raknet/StringTable.cpp
    raknet/SystemAddressList.cpp
    raknet/TableSerializer.cpp
    raknet/SAMP/samp_auth.cpp
    raknet/SAMP/samp_netencr.cpp
    raknet/SAMP/SAMPRPC.cpp)

set(RAKSAMP_TINYXML_SOURCES
    tinyxml/tinystr.cpp
    tinyxml/tinyxml.cpp
    tinyxml/tinyxmlerror.cpp
    tinyxml/tinyxmlparser.cpp)

if(NOT MSVC)
    find_package(Threads REQUIRED)
endif()

function(add_raksamp_core target product_include product_definition)
    add_library(${target} STATIC
        common/common.cpp
        ${RAKSAMP_RAKNET_SOURCES}
        ${RAKSAMP_TINYXML_SOURCES})
    target_include_directories(${target} PUBLIC
        ${product_include}
        common
        raknet
        raknet/SAMP
        tinyxml)
    target_compile_definitions(${target} PRIVATE
        ${product_definition}
        _CRT_SECURE_NO_WARNINGS)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W3 /permissive)
        target_link_libraries(${target} PUBLIC ws2_32)
    else()
        target_compile_options(${target} PRIVATE
            -fpermissive
            -fno-strict-aliasing
            -Wno-write-strings
            -Wno-conversion-null
            -Wno-deprecated-declarations)
        target_link_libraries(${target} PUBLIC Threads::Threads)
    endif()
endfunction()

if(RAKSAMP_BUILD_CLIENT)
    add_raksamp_core(raksamp-client-core client/src RAKSAMP_CLIENT)
    add_executable(raksamp-client
		client/src/automation_protocol.cpp
        client/src/cmds.cpp
		client/src/key_state.cpp
        client/src/localplayer.cpp
        client/src/load_mode.cpp
        client/src/load_mode_options.cpp
        client/src/main.cpp
        client/src/math_stuff.cpp
        client/src/misc_funcs.cpp
        client/src/native_driver.cpp
        client/src/netgame.cpp
        client/src/netrpc.cpp
        client/src/player_damage.cpp
        client/src/xmlsets.cpp)
    target_include_directories(raksamp-client PRIVATE client/src)
    target_compile_definitions(raksamp-client PRIVATE RAKSAMP_CLIENT POSIX_HEADLESS)
    target_link_libraries(raksamp-client PRIVATE raksamp-client-core)
    if(NOT MSVC)
        target_compile_options(raksamp-client PRIVATE
            -fpermissive
            -fno-strict-aliasing
            -Wno-write-strings
            -Wno-conversion-null)
    endif()
    install(TARGETS raksamp-client
        RUNTIME DESTINATION .
        COMPONENT client)
    install(FILES
        client/bin/RakSAMPClient.xml
        README.md
        THIRD_PARTY_NOTICES.md
        DESTINATION .
        COMPONENT client)
endif()

if(RAKSAMP_BUILD_SERVER)
    add_raksamp_core(raksamp-server-core server/src RAKSAMP_SERVER)
    FetchContent_Declare(lua
        URL https://www.lua.org/ftp/lua-5.4.8.tar.gz
        URL_HASH SHA256=4f18ddae154e793e46eeab727c59ef1c0c0c2b744e7b94219710d76f530629ae
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(lua)

    file(GLOB LUA54_SOURCES CONFIGURE_DEPENDS "${lua_SOURCE_DIR}/src/*.c")
    list(REMOVE_ITEM LUA54_SOURCES
        "${lua_SOURCE_DIR}/src/lua.c"
        "${lua_SOURCE_DIR}/src/luac.c"
        "${lua_SOURCE_DIR}/src/onelua.c")
    add_library(lua54 STATIC ${LUA54_SOURCES})
    target_include_directories(lua54 PUBLIC "${lua_SOURCE_DIR}/src")
    target_include_directories(raksamp-server-core PUBLIC "${lua_SOURCE_DIR}/src")
    if(UNIX)
        target_compile_definitions(lua54 PRIVATE LUA_USE_POSIX)
        target_link_libraries(lua54 PUBLIC ${CMAKE_DL_LIBS} m)
    endif()

    add_executable(raksamp-server
        server/src/Events.cpp
        server/src/RPCs.cpp
        server/src/ScrFunctions.cpp
        server/src/ScrInternal.cpp
        server/src/main.cpp
        server/src/misc_funcs.cpp
        server/src/netgame.cpp
        server/src/players.cpp
        server/src/query.cpp
        server/src/vehicles.cpp)
    target_include_directories(raksamp-server PRIVATE server/src client/src)
    target_compile_definitions(raksamp-server PRIVATE RAKSAMP_SERVER)
    target_link_libraries(raksamp-server PRIVATE raksamp-server-core lua54)
    if(NOT MSVC)
        target_compile_options(raksamp-server PRIVATE
            -fpermissive
            -fno-strict-aliasing
            -Wno-write-strings
            -Wno-conversion-null)
    endif()
    install(TARGETS raksamp-server
        RUNTIME DESTINATION .
        COMPONENT server)
    install(FILES
        server/bin/RakSAMPServer.xml
        README.md
        THIRD_PARTY_NOTICES.md
        DESTINATION .
        COMPONENT server)
    install(DIRECTORY server/bin/scripts
        DESTINATION .
        COMPONENT server)
endif()
