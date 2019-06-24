#Create the EvntlEvntlConsistColl library.

# Grab the .c and .cpp files
file (GLOB_RECURSE EVNTLCONSISTCOLL_C_CPP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx")
# Grab the C/C++ headers
file (GLOB_RECURSE EVNTLCONSISTCOLL_C_CPP_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${PROJECT_SOURCE_DIR}/include/*.hxx")
set (EVNTLCONSISTCOLL_C_CPP_FILES "${EVNTLCONSISTCOLL_C_CPP_SOURCE};${EVNTLCONSISTCOLL_C_CPP_HEADERS}")

add_library (EvntlConsistColl ${EVNTLCONSISTCOLL_C_CPP_FILES})

# Make sure the compiler can find include files for the library
# when other libraries or executables link to Hello
message ("crr source dir is ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories (EvntlConsistColl
				            PUBLIC "${PROJECT_SOURCE_DIR}/include"
               			    "${CMAKE_CURRENT_SOURCE_DIR}" )

# install the library
# 'make install' to the correct locations (provided by GNUInstallDirs).
install(TARGETS EvntlConsistColl EXPORT
	    ARCHIVE  DESTINATION ${CMAKE_INSTALL_DIR}/lib
	    LIBRARY  DESTINATION ${CMAKE_INSTALL_DIR}/lib)
install(DIRECTORY ../include DESTINATION ${CMAKE_INSTALL_DIR})

