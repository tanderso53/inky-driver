if (NOT TARGET pimoroni-inky-driver)
	add_library(pimoroni-inky-driver INTERFACE)

	target_sources(pimoroni-inky-driver INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/inky.c)

	target_include_directories(pimoroni-inky-driver INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/include)
endif()
