cmake_minimum_required(VERSION 3.5)
project(rtsp2ws)

set (CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release")
endif()
MESSAGE("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")


find_package(Git)
if(GIT_FOUND)
    EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive)
    EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty OUTPUT_VARIABLE PROJECTVERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    set (VERSION "${PROJECTVERSION}/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif()

# define executable to build
include_directories("inc")
aux_source_directory(src SRC_FILES)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION=\"${VERSION}\")

# liblive555helperr
add_subdirectory(live555helper EXCLUDE_FROM_ALL)
target_link_libraries (${PROJECT_NAME} liblive555helper)

# libhttpjsonserver
add_subdirectory(libhttpjsonserver EXCLUDE_FROM_ALL)
target_link_libraries (${PROJECT_NAME} httpjsonserver)

# cxxopts
add_subdirectory(cxxopts EXCLUDE_FROM_ALL)
target_link_libraries (${PROJECT_NAME} cxxopts)

# install
install (TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install (DIRECTORY www DESTINATION share/${PROJECT_NAME})
install (FILES config.json DESTINATION share/${PROJECT_NAME})
install (FILES keycert.pem DESTINATION share/${PROJECT_NAME})

SET(CPACK_GENERATOR "TGZ")
SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE})
SET(CPACK_PACKAGE_CONTACT "michel.promonet@free.fr") 
if(PROJECTVERSION)
    SET(CPACK_PACKAGE_VERSION "${PROJECTVERSION}")
endif()
INCLUDE(CPack)
