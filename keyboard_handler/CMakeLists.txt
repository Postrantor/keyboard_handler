cmake_minimum_required(VERSION 3.8)
project(keyboard_handler)

# 查找依赖项 (Find dependencies)
find_package(ament_cmake REQUIRED)

# 如果没有设置 CMAKE_CXX_STANDARD，将其设置为 14 (If CMAKE_CXX_STANDARD is not set, set it
# to 14)
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

# 对于 Windows，默认提供 min 和 max 宏。我们应该只使用来自 stl 的 min 和 max (For Windows, min and
# max macros are supplied by default. We should only use min and max from stl)
if(WIN32)
  add_definitions(-DNOMINMAX)
endif()

# 添加共享库 (Add shared library)
add_library(
  ${PROJECT_NAME} SHARED
  src/keyboard_handler_base.cpp src/default_unix_key_map.cpp
  src/default_windows_key_map.cpp src/keyboard_handler_unix_impl.cpp
  src/keyboard_handler_windows_impl.cpp)

# 如果编译器是 GNUCXX 或者 Clang，添加编译选项 (If the compiler is GNUCXX or Clang, add compile
# options)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 如果编译器是 Clang，添加编译选项 (If the compiler is Clang, add compile options)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wthread-safety)
endif()

# 设置目标包含目录 (Set target include directories)
target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)

# 使可见性宏使用 dllexport 而不是 dllimport，这在构建 dll 时适用，但在使用它时不适用 (Causes the visibility
# macros to use dllexport rather than dllimport, which is appropriate when
# building the dll but not consuming it)
target_compile_definitions(${PROJECT_NAME}
                           PRIVATE "KEYBOARD_HANDLER_BUILDING_LIBRARY")

# 安装 include 目录 (Install include directory)
install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})

# 安装目标库文件 (Install target library files)
install(
  TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES
  DESTINATION include)

# 在未来停止导出旧式 CMake 变量 (Stop exporting old-style CMake variables in the future)
ament_export_include_directories("include/${PROJECT_NAME}")
ament_export_libraries(${PROJECT_NAME})

# 导出目标 (Export targets)
ament_export_targets(export_${PROJECT_NAME})

# 如果构建测试 (If building tests)
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  find_package(ament_cmake_gtest)
  find_package(ament_cmake_gmock REQUIRED)
  ament_lint_auto_find_test_dependencies()

  # 设置键盘处理器测试源文件 (Set keyboard handler test sources)
  set(keyboard_handler_test_sources test/keyboard_handler_unix_tests.cpp
                                    test/keyboard_handler_windows_tests.cpp)

  # 添加 gmock 测试 (Add gmock test)
  ament_add_gmock(test_keyboard_handler ${keyboard_handler_test_sources})
  target_link_libraries(test_keyboard_handler ${PROJECT_NAME})
endif()

# 打包 (Package)
ament_package()
