## HGE CMake script including HGEHELP and examples
##
project( HGE )
cmake_minimum_required( VERSION 2.8 )

##----------------------------------------
## Edit this to match your configuration
##----------------------------------------

SET( DIRECTX_SDK_DIR "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)" )

##----------------------------------------
## End of manual configuration section
## try to minimize tampering with code below unless you know how CMake works
##----------------------------------------

# This is always 9, version 8 is not supported and will not work
SET( HGE_DIRECTX_VER "9" )



add_definitions( "-DHGE_DIRECTX_VER=${HGE_DIRECTX_VER}" "-DHGE_UNICODE=${HGE_UNICODE}" )
# disable warnings on STD non conformance and security issues
add_definitions( "-D_CRT_SECURE_NO_WARNINGS" "-D_CRT_NONSTDC_NO_WARNINGS" "-D_SCL_SECURE_NO_WARNINGS" "-D_WIN32_WINNT=0x0501" "/MP" )
## add_definitions( /MDd )

SET( OUTPUT_DIRECTORY_DLL "${HGE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
SET( OUTPUT_DIRECTORY_STATIC "${HGE_SOURCE_DIR}/linklib/${CMAKE_BUILD_TYPE}")
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY_DLL}")
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY_DLL}")
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${OUTPUT_DIRECTORY_DLL}")
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY_DLL}")
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY_DLL}")
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${OUTPUT_DIRECTORY_DLL}")
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY_STATIC}")
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY_STATIC}")
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${OUTPUT_DIRECTORY_STATIC}")
SET( CMAKE_INCLUDE_CURRENT_DIR ON )


#---------------------------------
add_subdirectory( src/zlib-1.2.5 )
add_subdirectory( src/libPNG )

#--- HGE core library and helpers ---
add_subdirectory( src/helpers )
add_subdirectory( src/core )

#--- HGE TOOLS - comment out of not needed ---
add_subdirectory( src/fontconv )
add_subdirectory( src/fonted )
add_subdirectory( src/particleed )
add_subdirectory( src/pngopt )
add_subdirectory( src/texasm )

#--- HGE examples ---
add_subdirectory( tutorials/tutorial01 )
add_subdirectory( tutorials/tutorial02 )
add_subdirectory( tutorials/tutorial03 )
add_subdirectory( tutorials/tutorial04 )
add_subdirectory( tutorials/tutorial05 )
add_subdirectory( tutorials/tutorial06 )
add_subdirectory( tutorials/tutorial07 )
add_subdirectory( tutorials/tutorial08 )

#if( ${HGE_DIRECTX_VER} MATCHES "9" )
    add_subdirectory( tutorials/tutorial05_shaders )
#endif()
