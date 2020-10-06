##########################################################################################
#                                                                                        #
# OpenSpace                                                                              #
#                                                                                        #
# Copyright (c) 2014-2018                                                                #
#                                                                                        #
# Permission is hereby granted, free of charge, to any person obtaining a copy of this   #
# software and associated documentation files (the "Software"), to deal in the Software  #
# without restriction, including without limitation the rights to use, copy, modify,     #
# merge, publish, distribute, sublicense, and/or sell copies of the Software, and to     #
# permit persons to whom the Software is furnished to do so, subject to the following    #
# conditions:                                                                            #
#                                                                                        #
# The above copyright notice and this permission notice shall be included in all copies  #
# or substantial portions of the Software.                                               #
#                                                                                        #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,    #
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A          #
# PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT     #
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF   #
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE   #
# OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                          #
##########################################################################################

set(CMAKE_FRAMEWORK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext)
set(external_dir ${CMAKE_FRAMEWORK_DIR}/external)

if(NOT EXISTS ${CMAKE_FRAMEWORK_DIR})
execute_process(
		#COMMAND git clone ${PROJECT_SOURCE_DIR}/.. ${CMAKE_FRAMEWORK_DIR}
		COMMAND git clone https://github.com/dtorban/cmake_fw.git ${CMAKE_FRAMEWORK_DIR}
)
endif()

include(${OPENSPACE_CMAKE_EXT_DIR}/module_definition.cmake)

set(HEADER_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/vislinkmodule.h
  ${CMAKE_CURRENT_SOURCE_DIR}/rendering/renderabletest.h
  ${CMAKE_CURRENT_SOURCE_DIR}/rendering/renderablevislink.h
  ${CMAKE_CURRENT_SOURCE_DIR}/rendering/VLOpenSpaceProcLoader.h
)
source_group("Header Files" FILES ${HEADER_FILES})

set(SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/vislinkmodule.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rendering/renderabletest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rendering/renderablevislink.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rendering/VLOpenSpaceProcLoader.cpp
)
source_group("Source Files" FILES ${SOURCE_FILES})

# --------------------- Dependencies -----------------------
include(${CMAKE_FRAMEWORK_DIR}/project/project.cmake)

add_external(VisLinkLib
	GIT_REPOSITORY https://github.com/dtorban/VisLink.git
	LIB_NAME VisLink
)

set(DEP_LINK_DIRS ${DEP_LINK_DIRS} ${external_dir}/VisLinkLib/install/lib)
set(DEP_INCLUDES ${DEP_INCLUDES} ${external_dir}/VisLinkLib/src/src)

set(vislink_external_dir ${external_dir}/VisLinkLib/src/cmake/framework/external)
message("VISLINK_EXTERNAL --- ${vislink_external_dir}")

set(DEP_LINK_DIRS ${DEP_LINK_DIRS} 
  ${vislink_external_dir}/SandBox/build/install/lib
)
message(${vislink_external_dir}/SandBox/src/src)
set(DEP_INCLUDES ${DEP_INCLUDES} 
  ${vislink_external_dir}/SandBox/src/src
  ${vislink_external_dir}/SandBox/cmake/framework/external/glm/src
)

set(DEP_LINK_DIRS ${DEP_LINK_DIRS} 
  ${vislink_external_dir}/SandBox/src/cmake/framework/external/glfw3/build/install/lib
)
set(DEP_INCLUDES ${DEP_INCLUDES} 
  ${vislink_external_dir}/SandBox/src/cmake/framework/external/glfw3/build/install/include
)

if(MSVC)
  set(DEP_LIBS ${DEP_LIBS} optimized glfw3.lib debug glfw3d.lib PARENT_SCOPE)
  #set(DEP_LIBS ${DEP_LIBS} optimized glfw3.lib debug glfw3d.lib PARENT_SCOPE)
  set(DEP_LIBS ${DEP_LIBS} optimized sandbox.lib debug sandboxd.lib PARENT_SCOPE)
  #set(DEP_LIBS ${DEP_LIBS} sandboxd.lib PARENT_SCOPE)
else()
  set(DEP_LIBS ${DEP_LIBS} optimized libglfw3.a debug libglfw3d.a PARENT_SCOPE)
  set(DEP_LIBS ${DEP_LIBS} optimized libsandbox.a debug libsandboxd.a PARENT_SCOPE)
endif()

#set(DEP_LIBS ${DEP_LIBS} sandboxd glfw3d)

find_package(Vulkan REQUIRED)
message("vk Found" ${Vulkan_FOUND} ${Vulkan_INCLUDE_DIRS} ${Vulkan_LIBRARIES})
include_directories(${Vulkan_INCLUDE_DIRS})
set(DEP_LIBS ${DEP_LIBS} ${Vulkan_LIBRARIES})


# --------------------- Executable -----------------------

link_directories(${DEP_LINK_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_FRAMEWORK_DIR}/external/VisLinkLib/src/src/VisLink)
include_directories(${CMAKE_FRAMEWORK_DIR}/external/VisLinkLib/src/src)

create_new_module(
  "VisLink"
  vislink_module
  STATIC
  ${HEADER_FILES} ${SOURCE_FILES} ${DEP_SOURCEFILES}
)

set(PROJECT_NAME ${vislink_module})
addStandardInfo(PUBLIC)

set(VisLink_LibDirs ${DEP_LINK_DIRS} CACHE INTERNAL "")
