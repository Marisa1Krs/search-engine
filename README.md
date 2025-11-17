.项目开发环境
Linux: Ubuntu18.04
G++: Version 4.8.4
Vim: Version 8.0

2.系统目录结构
src/：存放系统的源文件（*.cpp/*.cc） 。
Include/：存放系统的头文件（*.hpp） 。
bin/：存放系统的可执行程序
conf/myconf.conf：存放系统程序中所需的相关配置信息
dict/: 存放词典
data/dictIndex.dat: 存放单词所在位置的索引库
data/newripepage.dat：存放网页库
data/newoffset.dat：存放网页的偏移库
data/invertIndex.dat：存放倒排索引库
log/:存放日志文件

3.系统需求
3.1 系统运行过程图
3.2 需求分析
1）客户端发送关键字给服务器，获取相关关键字
2）客户端发送查询关键字给服务器，获取相关网页信息
3）服务器根据关键字，返回给客户端相关关键字
4）服务器根据关键字，返回给客户端相关网页