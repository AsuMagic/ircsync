cmake_minimum_required(VERSION 2.5)

project(ircsync)
add_executable(${PROJECT_NAME}
	"main.cpp"
	"irc.cpp"
)

target_compile_options(${PROJECT_NAME} PUBLIC
	"-std=c++1z"
	"-pthread"
	"-Wall"
	"-Wextra"
)

target_link_libraries(${PROJECT_NAME}
	boost_system
	pthread
	fmt
)
