# 内嵌 googletest 时禁用其安装规则，避免向安装前缀写入测试框架文件
set(INSTALL_GTEST OFF CACHE BOOL "Disable googletest install rules" FORCE)
add_subdirectory(googletest)
