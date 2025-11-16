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
include_directories(
    ${PROJECT_SOURCE_DIR}/include 
    ${PROJECT_SOURCE_DIR}/include/cppjieba 
    ${PROJECT_SOURCE_DIR}/include/simhash
    ${PROJECT_SOURCE_DIR}/include/json
)

# 查找src目录下所有的源文件
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRCS)

# 生成可执行程序，名为server
add_executable(server ${SRCS})

# ####################################### 新增链接配置 #######################################
# # 1. 指定库文件所在目录（绝对路径，根据你的路径直接写死，避免相对路径出错）
# link_directories(/home/marisa/code1/search-engine/data/third_party)

# # 2. 链接该目录下的所有库（静态库.a + 动态库.so）
# # 原理：使用 find_library 查找目录下所有库，通过 foreach 逐个链接
# file(GLOB THIRD_PARTY_LIBS 
#     /home/marisa/code1/search-engine/data/third_party/*.a  # 匹配所有静态库
#     /home/marisa/code1/search-engine/data/third_party/*.so # 匹配所有动态库
# )
# target_link_libraries(server PRIVATE ${THIRD_PARTY_LIBS})
# ###########################################################################################
