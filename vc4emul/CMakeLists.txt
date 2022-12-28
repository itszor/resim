
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
else(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter -Woverloaded-virtual -std=c++0x")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")


include_directories(${CMAKE_CURRENT_BINARY_DIR})


set(SRC
	main.cpp
)


include_directories(../libresim/include)

add_executable(vc4emul ${SRC} )
target_link_libraries(vc4emul libresim)

