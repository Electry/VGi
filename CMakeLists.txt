cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(VGi)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

add_executable(${PROJECT_NAME}
  main.c
  osd.c
  core/appinfo.c
  core/graphics.c
  core/graphics_rt.c
  core/graphics_cs.c
  core/graphics_dss.c
  core/graphics_tex.c
  core/graphics_scenes.c
  core/graphics_misc.c
  core/memory.c
  core/threads.c
  core/input.c
  core/device.c
)

target_link_libraries(${PROJECT_NAME}
  gcc
  taihen_stub
  taihenModuleUtils_stub
  SceAppMgr_stub
  SceDisplay_stub
  SceCtrl_stub
  SceKernelModulemgr_stub
  SceSysmem_stub
  SceLibc_stub
  ScePower_stub
  SceIofilemgr_stub
  SceLibKernel_stub
  SceGxm_stub
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES LINK_FLAGS "-nostdlib"
)

vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME}
  CONFIG ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.yml
)
