
#add executable called "bcast" that is built from the source file
file(GLOB_RECURSE BCAST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable (bcast ${BCAST_SOURCES})

target_include_directories (bcast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link the executable to the Hello library. Since the Hello library has
# public include directories we will use those link directories when building

target_link_libraries (bcast 
                       LINK_PUBLIC "-static-libstdc++"
                       ConsistColl 
		       ${GPI2_LIBRARIES} 
		       pthread
                       ibverbs
		       rt)
