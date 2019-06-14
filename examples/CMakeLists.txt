
#add executable called "bcast" that is built from the source file
file(GLOB_RECURSE BCAST_SOURCES bcast.cpp queue.cpp)
add_executable (bcast ${BCAST_SOURCES})

target_include_directories (bcast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link the executable to the library. Since the library has
# public include directories we will use those link directories when building
target_link_libraries (bcast 
                       LINK_PUBLIC "-static-libstdc++"
                       EvntlConsistColl 
		               ${GPI2_LIBRARIES} 
		               pthread
                       ibverbs
		               rt)


#add executable called "reduce" that is built from the source file
file(GLOB_RECURSE REDUCE_SOURCES reduce.cpp queue.cpp)
add_executable (reduce ${REDUCE_SOURCES})

target_include_directories (reduce PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries (reduce 
                       LINK_PUBLIC "-static-libstdc++"
                       EvntlConsistColl 
		               ${GPI2_LIBRARIES} 
		               pthread
                       ibverbs
		               rt)
