# 把.指定路径下的所有源文件名字放入变量名 SRC_LIST 中
aux_source_directory(. SRC_LIST) # 注意空格

# 表示生成可执行文件ChatServer，由SRC_LIST变量所定义的源文件编译而来
add_executable(ChatClient ${SRC_LIST})

# 指定可执行文件链接时所需要依赖的库文件
target_link_libraries(ChatClient pthread)