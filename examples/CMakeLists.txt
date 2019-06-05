#add executable called "bcast" that is built from the source file
add_executable (bcast bcast.cpp)

message(" crr src dir:  ${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories (bcast PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link the executable to the Hello library. Since the Hello library has
# public include directories we will use those link directories when building
# helloDemo

target_link_libraries (bcast 
                           LINK_PUBLIC "-static-libstdc++"
                           ConsistColl 
			   ${GPI2_LIBRARIES} 
			   pthread
                           ibverbs
			   rt)
