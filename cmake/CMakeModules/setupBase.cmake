set(OSG_BASE_DIR "" CACHE PATH "osg dependency base dir")
set(OSGEARTH_BASE_DIR "" CACHE PATH "osgEarth dependency base dir")

MACRO( FINDLIBRARY MYLIBRARY MYLIBRARYNAME )

FIND_LIBRARY(${MYLIBRARY}
    NAMES
        ${MYLIBRARYNAME}
    HINTS
        ${OSG_BASE_DIR}
        ${OSGEARTH_BASE_DIR}
    PATH_SUFFIXES
        /lib/
    NO_DEFAULT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
     )

ENDMACRO(FINDLIBRARY LIBRARY LIBRARYNAME)

MACRO(LINK_WITH_VARIABLES TRGTNAME)
    FOREACH(varname ${ARGN})
        IF(${varname}_DEBUG)
            IF(${varname}_RELEASE)
                TARGET_LINK_LIBRARIES(${TRGTNAME} optimized "${${varname}_RELEASE}" debug "${${varname}_DEBUG}")
            ELSE(${varname}_RELEASE)
                TARGET_LINK_LIBRARIES(${TRGTNAME} optimized "${${varname}}" debug "${${varname}_DEBUG}")
            ENDIF(${varname}_RELEASE)
        ELSE(${varname}_DEBUG)
            TARGET_LINK_LIBRARIES(${TRGTNAME} ${${varname}} )
        ENDIF(${varname}_DEBUG)
    ENDFOREACH(varname)
ENDMACRO(LINK_WITH_VARIABLES TRGTNAME)