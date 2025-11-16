# 指定CMake的最低版本要求
cmake_minimum_required(VERSION 3.10)
# 必须放在 project() 之前或之后，指定默认构建类型为 Debug
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
# 强制添加调试信息（即使不小心切换到 Release 也生效）
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")
# 项目名称
project(ServerProject)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 指定头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/include/cppjieba ${PROJECT_SOURCE_DIR}/include/simhash
${PROJECT_SOURCE_DIR}/include/json)

# 查找src目录下所有的源文件
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRCS)

# 生成可执行程序，名为server
add_executable(server ${SRCS})

