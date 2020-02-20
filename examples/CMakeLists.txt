
#add executable called "bcast" that is built from the source file
file(GLOB_RECURSE BCAST_SOURCES bcast.cpp queue.cpp)
add_executable (bcast ${BCAST_SOURCES})

target_include_directories (bcast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link the executable to the library. Since the library has
# public include directories we will use those link directories when building
target_link_libraries (bcast 
                       LINK_PUBLIC "-static-libstdc++"
                       EvntConsistColl 
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
                       EvntConsistColl 
		               ${GPI2_LIBRARIES} 
		               pthread
                       ibverbs
		               rt)


#add executable called "reduce_bench" that is built from the source file
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DN_SA=10 -DM_SZ=1000000")
file(GLOB_RECURSE REDUCE_SOURCES reduce_bench.cpp queue.cpp now.cpp)
add_executable (reduce_bench ${REDUCE_SOURCES})

target_include_directories (reduce_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries (reduce_bench
                       LINK_PUBLIC "-static-libstdc++"
                       EvntConsistColl 
		               ${GPI2_LIBRARIES} 
		               pthread
                       ibverbs
		               rt)


#add executable called "allreduce_bench" that is built from the source file
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DN_SA=10 -DM_SZ=1000000")
file(GLOB_RECURSE ALLREDUCE_SOURCES allreduce_bench.cpp queue.cpp now.cpp)
add_executable (allreduce_bench ${ALLREDUCE_SOURCES})

target_include_directories (allreduce_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries (allreduce_bench
                       LINK_PUBLIC "-static-libstdc++"
                       EvntConsistColl 
		               ${GPI2_LIBRARIES} 
		               pthread
                       ibverbs
		               rt)

