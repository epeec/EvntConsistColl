#Create the ConsistentCollectives library.
# The extension is already found. Any number of sources could be listed here.

# Grab the .c and .cpp files
file (GLOB_RECURSE CONSISTCOLL_C_CPP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx")
# Grab the C/C++ headers
file (GLOB_RECURSE CONSISTCOLL_C_CPP_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${PROJECT_SOURCE_DIR}/include/*.hxx")
set (CONSISTCOLL_C_CPP_FILES "${CONSISTCOLL_C_CPP_SOURCE};${CONSISTCOLL_C_CPP_HEADERS}")

add_library (ConsistColl ${CONSISTCOLL_C_CPP_FILES})

# Make sure the compiler can find include files for the library
# when other libraries or executables link to Hello
message ("crr source dir is ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories (ConsistColl
				PUBLIC "${PROJECT_SOURCE_DIR}/include"
               			"${CMAKE_CURRENT_SOURCE_DIR}" )

# install the library
# 'make install' to the correct locations (provided by GNUInstallDirs).
install(TARGETS ConsistColl EXPORT
	ARCHIVE  DESTINATION ${CMAKE_INSTALL_DIR}/lib
	LIBRARY  DESTINATION ${CMAKE_INSTALL_DIR}/lib)
install(DIRECTORY ../include DESTINATION ${CMAKE_INSTALL_DIR})

