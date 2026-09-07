cmake_minimum_required (VERSION 3.24)
PROJECT(superCluster C CXX)

include("${CMAKE_SOURCE_DIR}/cmake/CMakeModules/setupBase.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/CMakeModules/findosg.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/CMakeModules/findosgEarth.cmake")

add_executable(superCluster ./src/ClusterNode.h ./src/ClusterNode.cpp ./src/main.cpp)

target_compile_definitions(superCluster
    PRIVATE
     SCENECORE_EXPORTS   
)

LINK_WITH_VARIABLES(superCluster
	OSGTEXT_LIBRARY
	OSGDB_LIBRARY
	OSGUTIL_LIBRARY 
	OSGGA_LIBRARY
	OSGVIEWER_LIBRARY
	OSG_LIBRARY        
	OPENTHREADS_LIBRARY
    
	OSGEARTH_LIBRARY         
	OSGEARTHUTIL_LIBRARY
	OSGEARTHFEATURES_LIBRARY  
	OSGEARTHSYMBOLOGY_LIBRARY
	OSGEARTHANNOTATION_LIBRARY
)

target_include_directories(superCluster PRIVATE
    ${OSG_INCLUDE_DIR}
    ${OSGEARTH_INCLUDE_DIR}
)