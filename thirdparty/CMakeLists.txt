# 内嵌 googletest 时禁用其安装规则，避免向安装前缀写入测试框架文件
set(INSTALL_GTEST OFF CACHE BOOL "Disable googletest install rules" FORCE)
add_subdirectory(googletest)

# 内嵌 google benchmark 时禁用其测试/安装/工具规则，避免污染安装前缀
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install rules" FORCE)
set(BENCHMARK_ENABLE_DOXYGEN OFF CACHE BOOL "Disable benchmark docs" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "Disable benchmark docs install" FORCE)
set(BENCHMARK_INSTALL_TOOLS OFF CACHE BOOL "Disable benchmark tools install" FORCE)
add_subdirectory(benchmark)
