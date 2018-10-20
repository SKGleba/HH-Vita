cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

OPTION(DEFINE_DEBUG
  "Build the project using debugging code"
  OFF)
IF(DEFINE_DEBUG)
  MESSAGE("Adding Debug flag...")
  ADD_DEFINITIONS(-DDEFINE_DEBUG)
  #SET(CMAKE_BUILD_TYPE Debug)
  MESSAGE("Build type is " ${CMAKE_BUILD_TYPE})
ENDIF(DEFINE_DEBUG)


set(SHORT_NAME HHBRWS)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "HentaiHaven")
set(VITA_TITLEID  "SKGHHBRWS")
set(VITA_VERSION  "01.50")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fpermissive -std=gnu11 -w ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fpermissive -std=c++11 -w ")


include_directories(
  src/includes
)

add_executable(${SHORT_NAME}.elf
  src/main.cpp
  src/EpisodeVideo.cpp
  src/Episode.cpp
  src/Series.cpp
  src/VitaPad.cpp
)

target_link_libraries(${SHORT_NAME}.elf
  
  debugnet
  vita2d
  freetype
  png
  jpeg
  z
  m
  c
  ScePgf_stub
  ScePvf_stub
  SceGxm_stub
  SceCtrl_stub
  SceTouch_stub
  SceAppUtil_stub
  SceDisplay_stub
  SceLibKernel_stub
  SceSysmodule_stub
  ScePower_stub
  SceCommonDialog_stub
  SceRegistryMgr_stub
  pthread
  SceAppMgr_stub
  SceHttp_stub
  SceNet_stub
  SceNetCtl_stub
)


vita_create_self(eboot.bin ${SHORT_NAME}.elf)

vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${CMAKE_SOURCE_DIR}/assets/font.ttf assets/font.ttf
  FILE ${CMAKE_SOURCE_DIR}/assets/mp4p.bin mp4p.bin
  FILE ${CMAKE_SOURCE_DIR}/assets/db.json assets/db.json
  FILE ${CMAKE_SOURCE_DIR}/assets/icon0.png sce_sys/icon0.png
  FILE ${CMAKE_SOURCE_DIR}/assets/bg.png sce_sys/livearea/contents/bg0.png
  FILE ${CMAKE_SOURCE_DIR}/assets/pic0.png sce_sys/pic0.png
  FILE ${CMAKE_SOURCE_DIR}/assets/template.xml sce_sys/livearea/contents/template.xml
)

