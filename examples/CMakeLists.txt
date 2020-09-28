
#add executable called "bcast_bench" that is built from the source file
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
file(GLOB_RECURSE REDUCE_SOURCES bcast_bench.cpp)
add_executable (bcast_bench ${REDUCE_SOURCES})

target_include_directories (bcast_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries (bcast_bench
                       LINK_PUBLIC "-static-libstdc++"
                       EvntConsistColl 
		               ${GPI2_LIBRARIES} 
		               pthread
                       ibverbs
		               rt)


#add executable called "reduce_bench" that is built from the source file
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
file(GLOB_RECURSE REDUCE_SOURCES reduce_bench.cpp)
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
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
file(GLOB_RECURSE ALLREDUCE_SOURCES allreduce_bench.cpp)
add_executable (allreduce_bench ${ALLREDUCE_SOURCES})

target_include_directories (allreduce_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries (allreduce_bench
                       LINK_PUBLIC "-static-libstdc++"
                       EvntConsistColl 
		               ${GPI2_LIBRARIES} 
		               pthread
                       ibverbs
		               rt)

