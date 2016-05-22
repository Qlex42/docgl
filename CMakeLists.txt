# --------------------------------- . ---------------------------------------- #
# Filename : CMakeList.txt          | DOCGL sample CMake Entry point           #
# Author   : Alexandre Buge         |                                          #
# Started  : 26/05/2011 14:00       |                                          #
# --------------------------------- . ---------------------------------------- #

cmake_minimum_required(VERSION 3.1)

PROJECT(docgl CXX C)

include(ExternalProject)
ExternalProject_Add(
    glm
    PREFIX ${CMAKE_BINARY_DIR}/glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.7.4
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory
                    <SOURCE_DIR>/glm ${CMAKE_BINARY_DIR}/extern/include/glm
    LOG_DOWNLOAD ON
    LOG_INSTALL ON
    )

ExternalProject_Get_Property(glm source_dir)
set(GlmIncludeDir ${source_dir})

OPTION(DOCGL_MULTICONTEXT "MultiContext" OFF)
IF (DOCGL_MULTICONTEXT)
  ADD_DEFINITIONS(-DGLEW_MX)
  IF   (UNIX) # g++ does'nt like implicite access to glew context from template.
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
  ENDIF(UNIX)
ENDIF (DOCGL_MULTICONTEXT)

OPTION(DOCGL4_1 "OpenGL 4.1 Sample" OFF)
IF (DOCGL4_1)
  ADD_DEFINITIONS(-DDOCGL4_1)
ENDIF (DOCGL4_1)

SET (CMAKE_BUILD_TYPE Debug)

SET(EXTERN_PATH ${docgl_SOURCE_DIR}/extern CACHE PATH "extern directory" FORCE)
MARK_AS_ADVANCED(EXTERN_PATH)

IF   (WIN32)
ELSE (WIN32)
  IF   (UNIX)
    IF   (APPLE)
    ELSE (APPLE)
      ADD_DEFINITIONS(-DLINUX)
      IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
        ADD_DEFINITIONS(-D_DEBUG)
      ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    ENDIF(APPLE)
  ENDIF(UNIX)
ENDIF(WIN32)

INCLUDE_DIRECTORIES(
  ${EXTERN_PATH}/include
  ${GlmIncludeDir}
)

IF   (WIN32)
  ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501 -D_CRT_SECURE_NO_WARNINGS)

  IF (MSVC)
    # Set this to make visual studio create pdb files in release mode
    # SET (CMAKE_EXE_LINKER_FLAGS_RELEASE /DEBUG ${CMAKE_EXE_LINKER_FLAGS_RELEASE})
    # SET (CMAKE_SHARED_LINKER_FLAGS /DEBUG ${CMAKE_SHARED_LINKER_FLAGS})

    # Multi-threaded compilation on visual studio
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

    IF (CMAKE_CL_64)
      ADD_DEFINITIONS(-D_WIN64)
      # SSE2 32/64bit floating point arithmetic instruction set replace x87 80bit one, and be implemented by default in X64.
    ELSE (CMAKE_CL_64)
      # activate SSE2 Instruction set on visual studio x86 (Intel Pentium 4 (2001) AMD Operon / AMD 64 (2003))
      # activate address over 2GO limits (app source code and libs must have correct pointer arithmetics)
      SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
      SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
    ENDIF (CMAKE_CL_64)
  ELSE (MSVC)
    # MINGW CYGWIN ?
    ADD_DEFINITIONS(-DWINVER=0x0501)
  ENDIF (MSVC)
ENDIF (WIN32)
ADD_DEFINITIONS(-DGLEW_STATIC)


MACRO(SOURCE_SUB_GROUP ROOT_NAME SUB_NAME) # other args
  IF (WIN32)
    SOURCE_GROUP(${ROOT_NAME}\\${SUB_NAME} ${ARGN})
  ELSE (WIN32)
    SOURCE_GROUP(${ROOT_NAME}/${SUB_NAME} ${ARGN})
  ENDIF (WIN32)
ENDMACRO(SOURCE_SUB_GROUP)


MACRO(SOURCE_SUB_GROUP2 ROOT_NAME SUB_NAME SUB_NAME2) # other args
  IF (WIN32)
    SOURCE_GROUP(${ROOT_NAME}\\${SUB_NAME}\\${SUB_NAME2} ${ARGN})
  ELSE (WIN32)
    SOURCE_GROUP(${ROOT_NAME}/${SUB_NAME}/${SUB_NAME2} ${ARGN})
  ENDIF (WIN32)
ENDMACRO(SOURCE_SUB_GROUP2)

# build redistribuable zip
SET (CPACK_PACKAGE_NAME "docgl")
SET (CPACK_PACKAGE_VENDOR "d-labs.fr")
SET (CPACK_PACKAGE_DESCRIPTION_SUMMARY "docgl")
SET (CPACK_PACKAGE_VERSION_MAJOR "1")
SET (CPACK_PACKAGE_VERSION_MINOR "0")
SET (CPACK_GENERATOR "ZIP")
INCLUDE(CPack)

SET(GLEW_SOURCES extern/src/GL/glew.c)

IF   (WIN32)
  SET(DOCGL_PLATEFORM_SOURCES extern/include/Docgl/Docwgl.h)
  SET(OPENGLIBS opengl32)
ELSE (WIN32)
  IF (UNIX)
    SET(DOCGL_PLATEFORM_SOURCES extern/include/Docgl/Docglx.h)
    SET(OPENGLIBS X11 GL m)
  ELSE (UNIX)
    SET(DOCGL_PLATEFORM_SOURCES)
    SET(OPENGLIBS)
  ENDIF (UNIX)
ENDIF (WIN32)

SET(DOCGL_SOURCES
  extern/include/Docgl/Docgl.h
  extern/include/Docgl/DocglWindowCallback.h
  extern/include/Docgl/DocglWindow.h
)

SET(SUPERFORMULA_SAMPLES_SOURCES
  Main.cpp
  SuperFormula.cpp
  Tools.h
)

SET(SUPERFORMULA_SOURCES
  ${DOCGL_SOURCES}
  ${DOCGL_PLATEFORM_SOURCES}
  ${GLEW_SOURCES}
  ${SUPERFORMULA_SAMPLES_SOURCES}
)

SET(BOUNCE_SAMPLES_SOURCES
  Bounce.cpp
  Main.cpp
  Tools.h
)

SET(BOUNCE_SOURCES
  ${DOCGL_SOURCES}
  ${DOCGL_PLATEFORM_SOURCES}
  ${GLEW_SOURCES}
  ${BOUNCE_SAMPLES_SOURCES}
)

SOURCE_GROUP(Sample FILES ${BOUNCE_SAMPLES_SOURCES})
SOURCE_GROUP(Sample FILES ${SUPERFORMULA_SAMPLES_SOURCES})
SOURCE_GROUP(Docgl FILES ${DOCGL_SOURCES})
SOURCE_SUB_GROUP(Docgl Plateform FILES ${DOCGL_PLATEFORM_SOURCES})
SOURCE_GROUP(Glew FILES ${GLEW_SOURCES})

MACRO(ADD_APPLICATION NAME LABEL)
  IF(WIN32)
    ADD_EXECUTABLE(${NAME} ${ARGN}  )
  ELSE(WIN32)
    IF(UNIX)
      IF(APPLE)
	      SET(MACOSX_BUNDLE_INFO_STRING ${LABEL})
	      SET(MACOSX_BUNDLE_ICON_FILE "")
	      SET(MACOSX_BUNDLE_GUI_IDENTIFIER "")
	      SET(MACOSX_BUNDLE_LONG_VERSION_STRING "")
	      SET(MACOSX_BUNDLE_BUNDLE_NAME "")
	      SET(MACOSX_BUNDLE_SHORT_VERSION_STRING "")
	      SET(MACOSX_BUNDLE_BUNDLE_VERSION "")
	      SET(MACOSX_BUNDLE_COPYRIGHT "D-Labs")
	      ADD_EXECUTABLE(${NAME} MACOSX_BUNDLE ${ARGN})
      ELSE(APPLE)
	      ADD_EXECUTABLE(${NAME} ${ARGN})
      ENDIF(APPLE)
    ENDIF(UNIX)
  ENDIF(WIN32)
  SET_TARGET_PROPERTIES(${NAME} PROPERTIES PROJECT_LABEL ${LABEL})
  TARGET_LINK_LIBRARIES(${NAME} ${OPENGLIBS})
ENDMACRO(ADD_APPLICATION)

ADD_APPLICATION(SuperFormula "SuperFormula" ${SUPERFORMULA_SOURCES})
add_dependencies(SuperFormula glm)
INSTALL(TARGETS SuperFormula DESTINATION bin)

#ADD_APPLICATION(Bounce "Bounce" ${BOUNCE_SOURCES})
#add_dependencies(Bounce glm)
#INSTALL(TARGETS Bounce DESTINATION bin)
