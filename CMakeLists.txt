cmake_minimum_required(VERSION 3.10)

project(MufExprtkBackend)

#disable all warning for third party libs
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_PREFIX_PATH "C:\\Qt\\Qt5.2.0\\5.2.0\\msvc2010_opengl" "C:\\WinDDK\\7600.16385.1\\lib\\win7\\i386")
else(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_PREFIX_PATH "~/Qt/5.10.1/gcc_64/")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)


add_library(MufExprtkBackend SHARED src/mufexprtkbackend.cpp)

install(TARGETS MufExprtkBackend 
	LIBRARY DESTINATION /home/muf/Workspace/StrahCalc/lib)
include_directories(${Qt5Core_INCLUDE_DIRS})       

find_package(Qt5Core)

target_link_libraries(MufExprtkBackend
    ${Qt5Core_QTMAIN_LIBRARIES} Qt5::Core)
