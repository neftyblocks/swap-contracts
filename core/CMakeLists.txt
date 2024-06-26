   cmake_minimum_required(VERSION 3.5)

   project(swap_contracts)

   set(VERSION_MAJOR 1)
   set(VERSION_MINOR 0)
   set(VERSION_PATCH 0)

   set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

   include(ExternalProject)

   find_package(eosio.cdt)

   message(STATUS "Building swap contracts v${VERSION_FULL}")


   if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(TEST_BUILD_TYPE "Debug")
      set(CMAKE_BUILD_TYPE "Release")
   else()
      set(TEST_BUILD_TYPE ${CMAKE_BUILD_TYPE})
   endif()

   ExternalProject_Add(
      contracts_project
      SOURCE_DIR ${CMAKE_SOURCE_DIR}/contracts
      BINARY_DIR ${CMAKE_BINARY_DIR}/contracts
      CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${EOSIO_CDT_ROOT}/lib/cmake/eosio.cdt/EosioWasmToolchain.cmake
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      TEST_COMMAND ""
      INSTALL_COMMAND ""
      BUILD_ALWAYS 1
   )

   if (APPLE)
      set(OPENSSL_ROOT "/usr/local/opt/openssl")
   elseif (UNIX)
      set(OPENSSL_ROOT "/usr/include/openssl")
   endif()
   set(SECP256K1_ROOT "/usr/local")

   if (APPLE)
      set(OPENSSL_ROOT "/usr/local/opt/openssl")
   elseif (UNIX)
      set(OPENSSL_ROOT "/usr/include/openssl")
   endif()
   set(SECP256K1_ROOT "/usr/local")

   string(REPLACE ";" "|" TEST_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
   string(REPLACE ";" "|" TEST_FRAMEWORK_PATH "${CMAKE_FRAMEWORK_PATH}")
   string(REPLACE ";" "|" TEST_MODULE_PATH "${CMAKE_MODULE_PATH}")


