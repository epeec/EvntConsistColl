#Create the ConsistentCollectives library.
# The extension is already found. Any number of sources could be listed here.
add_library ( ConsistColl
    consistent.collectives.cxx
 )

# Make sure the compiler can find include files for the library
# when other libraries or executables link to Hello
message ("crr source dir is ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories (ConsistColl
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include 
               ${CMAKE_CURRENT_SOURCE_DIR} )

# install the library
# 'make install' to the correct locations (provided by GNUInstallDirs).
install(TARGETS ConsistColl EXPORT
	ARCHIVE  DESTINATION ${CMAKE_INSTALL_DIR}/lib
	LIBRARY  DESTINATION ${CMAKE_INSTALL_DIR}/lib)
install(DIRECTORY ../include DESTINATION ${CMAKE_INSTALL_DIR})

