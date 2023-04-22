// Copyright 2021 Apex.AI, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef KEYBOARD_HANDLER__KEYBOARD_HANDLER_UNIX_IMPL_HPP_
#define KEYBOARD_HANDLER__KEYBOARD_HANDLER_UNIX_IMPL_HPP_

#ifndef _WIN32
#include "keyboard_handler/visibility_control.hpp"
#include "keyboard_handler_base.hpp"
#include <atomic>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <thread>
#include <tuple>
#include <unordered_map>

/// \brief Unix (Posix) 特定实现的键盘处理器类。 (Unix (Posix) specific implementation of keyboard handler class.)
/// \note 设计和实现限制：(Design and implementation limitations:)
/// 不能正确检测 CTRL + 0..9 数字键。 (Can't correctly detect CTRL + 0..9 number keys.)
/// 不能正确检测与 F1..F12 和其他控制键一起使用的 CTRL, ALT, SHIFT 修饰符。 (Can't correctly detect CTRL, ALT, SHIFT modifiers with F1..F12 and other control keys.)
/// 检测到的将只是 CTRL + key，而不是 CTRL + SHIFT + key。 (Instead of CTRL + SHIFT + key will be detected only CTRL + key.)
/// 在同时按下多个键修饰符时，某些键可能会被错误地检测。 (Some keys might be incorrectly detected with multiple key modifiers pressed at the same time.)
class KeyboardHandlerUnixImpl : public KeyboardHandlerBase
{
public:
  /// \brief 定义 isattyFunction 的类型别名 (Define type alias for isattyFunction)
  using isattyFunction = std::function<int(int)>;

  /// \brief 定义 tcgetattrFunction 的类型别名 (Define type alias for tcgetattrFunction)
  using tcgetattrFunction = std::function<int(int, struct termios *)>;

  /// \brief 定义 tcsetattrFunction 的类型别名 (Define type alias for tcsetattrFunction)
  using tcsetattrFunction = std::function<int(int, int, const struct termios *)>;

  /// \brief 定义 readFunction 的类型别名 (Define type alias for readFunction)
  using readFunction = std::function<ssize_t(int, void *, size_t)>;

  /// \brief 定义 signal_handler_type 的类型别名 (Define type alias for signal_handler_type)
  using signal_handler_type = void (*)(int);

  /// \brief 默认构造函数 (Default constructor)
  KEYBOARD_HANDLER_PUBLIC
  KeyboardHandlerUnixImpl();

  /// \brief 带有是否安装 SIGINT 信号处理器选项的构造函数 (Constructor with option to not install signal handler for SIGINT)
  /// \param install_signal_handler 如果为 true，则安装 SIGINT 信号处理器，否则不安装。 (if true, signal handler for SIGINT will be installed, otherwise not.)
  /// \note 如果 install_signal_handler 为 false，则调用者代码应在信号到达导致的进程终止时调用静态 KeyboardHandlerUnixImpl::restore_buffer_mode_for_stdin()。 (In case if install_signal_handler is false, caller code should call static KeyboardHandlerUnixImpl::restore_buffer_mode_for_stdin() in case of process termination caused by signal arrival.)
  KEYBOARD_HANDLER_PUBLIC
  explicit KeyboardHandlerUnixImpl(bool install_signal_handler);

  /// \brief 析构函数 (destructor)
  KEYBOARD_HANDLER_PUBLIC
  virtual ~KeyboardHandlerUnixImpl();

  /// \brief 将指定的按键组合转换为终端响应按键按下所返回的相应已注册字符序列。 (Translates specified key press combination to the corresponding registered sequence of characters returning by terminal in response to the pressing keyboard keys.)
  /// \param key_code 枚举值，对应于某个预定义的按键组合。 (Value from enum which corresponds to some predefined key press combination.)
  /// \return 返回终端期望返回的字符序列字符串。 (Returns string with sequence of characters expecting to be returned by terminal.)
  KEYBOARD_HANDLER_PUBLIC
  std::string get_terminal_sequence(KeyboardHandlerUnixImpl::KeyCode key_code);

  /// \brief 恢复 stdin 的缓冲模式 (Restore buffer mode for stdin)
  KEYBOARD_HANDLER_PUBLIC
  static bool restore_buffer_mode_for_stdin();

  /// \brief 获取旧的 SIGINT 信号处理器 (Get old SIGINT signal handler)
  KEYBOARD_HANDLER_PUBLIC
  static signal_handler_type get_old_sigint_handler();

protected:
  /// \brief 构造函数，包含系统功能引用。单元测试所需。
  /// \param read_fn 系统 read(int, void *, size_t) 函数的引用
  /// \param isatty_fn 系统 isatty(int) 函数的引用
  /// \param tcgetattr_fn 系统 tcgetattr(int, struct termios *) 函数的引用
  /// \param tcsetattr_fn 系统 tcsetattr(int, int, const struct termios *) 函数的引用
  /// \brief Constructor with references to the system functions. Required for unit tests.
  /// \param read_fn Reference to the system read(int, void *, size_t) function
  /// \param isatty_fn Reference to the system isatty(int) function
  /// \param tcgetattr_fn Reference to the system tcgetattr(int, struct termios *) function
  /// \param tcsetattr_fn Reference to the system tcsetattr(int, int, const struct termios *) function
  KEYBOARD_HANDLER_PUBLIC
  KeyboardHandlerUnixImpl(
    const readFunction & read_fn,
    const isattyFunction & isatty_fn,
    const tcgetattrFunction & tcgetattr_fn,
    const tcsetattrFunction & tcsetattr_fn,
    bool install_signal_handler = true);

  /// \brief 输入解析器
  /// \param buff 从 std::in 读出的按键后的空终止缓冲区
  /// \param read_bytes 缓冲区长度（以字节为单位），不包括空终止符
  /// \return 元组键码和代码修饰符掩码
  /// \brief Input parser
  /// \param buff null terminated buffer read out from std::in after key press
  /// \param read_bytes length of the buffer in bytes without null terminator
  /// \return tuple key code and code modifiers mask
  std::tuple<KeyCode, KeyModifiers> parse_input(const char * buff, ssize_t read_bytes);

  /// \brief 用于将 KeyCode 枚举值映射到终端返回的预期字符序列的数据类型。
  /// \brief Data type for mapping KeyCode enum value to the expecting sequence of characters returning by terminal.
  struct KeyMap
  {
    KeyCode inner_code;
    const char * terminal_sequence;
  };

  /// \brief 默认静态定义的查找表，用于对应 KeyCode 枚举值和终端返回的预期字符序列。
  /// \brief Default statically defined lookup table for corresponding KeyCode enum values and expecting sequence of characters returning by terminal.
  static const KeyMap DEFAULT_STATIC_KEY_MAP[];

  /// \brief DEFAULT_STATIC_KEY_MAP 的长度，以元素个数为单位。
  /// \brief Length of DEFAULT_STATIC_KEY_MAP measured in number of elements.
  static const size_t STATIC_KEY_MAP_LENGTH;

private:
  static void on_signal(int signal_number);

  static struct termios old_term_settings_;
  static tcsetattrFunction tcsetattr_fn_;
  static signal_handler_type old_sigint_handler_;
  bool install_signal_handler_ = false;

  std::thread key_handler_thread_;
  static std::atomic_bool exit_;
  const int stdin_fd_;
  std::unordered_map<std::string, KeyCode> key_codes_map_;
  std::exception_ptr thread_exception_ptr{nullptr};
};

#endif // #ifndef _WIN32
#endif // KEYBOARD_HANDLER__KEYBOARD_HANDLER_UNIX_IMPL_HPP_
