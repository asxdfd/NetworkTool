# CMakeList.txt: NetworkTool 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

include_directories("./include")

# 将源代码添加到此项目的可执行文件。
add_executable (NetworkServer "src/NetworkServer.cpp")
add_executable (NetworkClient "src/NetworkClient.cpp")

# TODO: 如有需要，请添加测试并安装目标。
target_link_libraries(NetworkServer
	ws2_32
)

target_link_libraries(NetworkClient
	ws2_32
)
