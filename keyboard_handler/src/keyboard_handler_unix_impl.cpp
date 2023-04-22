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

#ifndef _WIN32
#include "keyboard_handler/keyboard_handler_unix_impl.hpp"
#include <algorithm>
#include <csignal>
#include <exception>
#include <iostream>
#include <string>
#include <tuple>
#include <unistd.h>

std::atomic_bool KeyboardHandlerUnixImpl::exit_{false};
// 原子布尔类型变量，表示是否退出。Atomic boolean variable indicating whether to exit.
struct termios KeyboardHandlerUnixImpl::old_term_settings_ = {};
// 存储旧的终端设置。Store the old terminal settings.

KeyboardHandlerUnixImpl::tcsetattrFunction KeyboardHandlerUnixImpl::tcsetattr_fn_ = tcsetattr;
// 定义 tcsetattr 函数指针。Define a function pointer for tcsetattr.

KeyboardHandlerUnixImpl::signal_handler_type KeyboardHandlerUnixImpl::old_sigint_handler_ = SIG_DFL;
// 存储旧的 SIGINT 信号处理器。Store the old SIGINT signal handler.

/**
 * @brief 处理接收到的信号。
 * @param signal_number 信号编号。
 *
 * Handle received signals.
 * @param signal_number The signal number.
 */
void KeyboardHandlerUnixImpl::on_signal(int signal_number)
{
  auto old_sigint_handler = KeyboardHandlerUnixImpl::get_old_sigint_handler();
  // 获取旧的 SIGINT 信号处理器。Get the old SIGINT signal handler.

  // 恢复标准输入的缓冲模式。Restore buffer mode for stdin.
  if (old_sigint_handler == SIG_DFL) {
    if (KeyboardHandlerUnixImpl::restore_buffer_mode_for_stdin()) {
      _exit(EXIT_SUCCESS);
    } else {
      _exit(EXIT_FAILURE);
    }
  } else {
    exit_ = true;
    KeyboardHandlerUnixImpl::restore_buffer_mode_for_stdin();
  }

  if (
    (old_sigint_handler != SIG_ERR) && (old_sigint_handler != SIG_IGN) &&
    (old_sigint_handler != SIG_DFL)) {
    old_sigint_handler(signal_number);
  }
}

KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerUnixImpl::KeyboardHandlerUnixImpl()
: KeyboardHandlerUnixImpl(read, isatty, tcgetattr, tcsetattr)
{
  // 默认构造函数。Default constructor.
}

KEYBOARD_HANDLER_PUBLIC
/**
 * @brief 构造函数。
 * @param install_signal_handler 是否安装信号处理器。
 *
 * Constructor.
 * @param install_signal_handler Whether to install the signal handler.
 */
KeyboardHandlerUnixImpl::KeyboardHandlerUnixImpl(bool install_signal_handler)
: KeyboardHandlerUnixImpl(read, isatty, tcgetattr, tcsetattr, install_signal_handler)
{
}

/**
 * @brief 解析输入缓冲区的按键信息 (Parse the key information in the input buffer)
 *
 * @param[in] buff 输入缓冲区指针 (Pointer to the input buffer)
 * @param[in] read_bytes 缓冲区中已读取字节的数量 (Number of bytes read in the buffer)
 * @return std::tuple<KeyboardHandlerBase::KeyCode, KeyboardHandlerBase::KeyModifiers> 按键代码和修饰符组合 (Combination of key code and modifiers)
 */
std::tuple<KeyboardHandlerBase::KeyCode, KeyboardHandlerBase::KeyModifiers>
KeyboardHandlerUnixImpl::parse_input(const char * buff, ssize_t read_bytes)
{
  // 打印调试信息 (Print debug info)
#ifdef PRINT_DEBUG_INFO
  std::cout << "Read " << read_bytes << " bytes: ";
  if (read_bytes > 1) {
    std::cout << "[] = {";
    for (ssize_t i = 0; i < read_bytes; ++i) {
      std::cout << static_cast<int>(buff[i]) << ", ";
    }
    std::cout << "'\0'};";
  } else {
    std::cout << " : " << static_cast<int>(buff[0]) << " : '" << buff[0] << "'";
  }
  std::cout << std::endl;
#endif

  // 初始化按键代码和修饰符变量 (Initialize key code and modifier variables)
  KeyCode pressed_key_code = KeyCode::UNKNOWN;
  KeyModifiers key_modifiers = KeyModifiers::NONE;

  // 将输入缓冲区转换为字符串以进行搜索 (Convert the input buffer to a string for searching)
  std::string buff_to_search = buff;
  ssize_t bytes_in_keycode = read_bytes;

  // 检查是否按下了 Alt 键 (Check if the Alt key was pressed)
  if (read_bytes == 2 && buff[0] == 27) {
    key_modifiers = KeyModifiers::ALT;
    buff_to_search = buff[1];
    bytes_in_keycode = 1;
  }

  // 检查是否按下了 Shift 键 (Check if the Shift key was pressed)
  if (bytes_in_keycode == 1 && buff_to_search[0] >= 'A' && buff_to_search[0] <= 'Z') {
    char original_key_code = buff_to_search[0];
    original_key_code += 32; // 转换为小写字母 (Convert to lowercase letter)
    buff_to_search = original_key_code;
    key_modifiers = key_modifiers | KeyModifiers::SHIFT;
  }

  // 在 key_codes_map_ 中查找对应的按键代码 (Find the corresponding key code in key_codes_map_)
  auto key_map_it = key_codes_map_.find(buff_to_search);
  if (key_map_it != key_codes_map_.end()) {
    pressed_key_code = key_map_it->second;
  }

  // 如果在 key_codes_map_ 中未找到，尝试查找 Ctrl 键 (If not found in key_codes_map_, try searching for the Ctrl key)
  if (
    pressed_key_code == KeyCode::UNKNOWN && bytes_in_keycode == 1 &&
    static_cast<signed char>(buff_to_search[0]) >= 0 && buff_to_search[0] <= 26) {
    char original_key_code = buff_to_search[0];
    original_key_code += 96; // small chars
    buff_to_search = original_key_code;
    key_modifiers = key_modifiers | KeyModifiers::CTRL;

    auto key_map_it = key_codes_map_.find(buff_to_search);
    if (key_map_it != key_codes_map_.end()) {
      pressed_key_code = key_map_it->second;
    }
  }

  // 返回按键代码和修饰符组合 (Return the combination of key code and modifiers)
  return std::make_tuple(pressed_key_code, key_modifiers);
}

/**
 * @brief 构造函数，用于初始化 KeyboardHandlerUnixImpl 类的实例
 *        Constructor for initializing an instance of the KeyboardHandlerUnixImpl class.
 *
 * @param read_fn 读取终端输入的函数，不能为空
 *                Function to read terminal input, must be non-empty.
 * @param isatty_fn 检查文件描述符是否指向终端设备的函数，不能为空
 *                  Function to check if a file descriptor points to a terminal device, must be non-empty.
 * @param tcgetattr_fn 获取终端属性的函数，不能为空
 *                     Function to get terminal attributes, must be non-empty.
 * @param tcsetattr_fn 设置终端属性的函数，不能为空
 *                     Function to set terminal attributes, must be non-empty.
 * @param install_signal_handler 是否安装信号处理器
 *                               Whether to install a signal handler.
 */
KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerUnixImpl::KeyboardHandlerUnixImpl(
  const readFunction & read_fn,
  const isattyFunction & isatty_fn,
  const tcgetattrFunction & tcgetattr_fn,
  const tcsetattrFunction & tcsetattr_fn,
  bool install_signal_handler)
: stdin_fd_(fileno(stdin)) // 初始化标准输入文件描述符
                           // Initialize standard input file descriptor
{
  // 检查 read_fn 是否为空
  // Check if read_fn is empty
  if (read_fn == nullptr) {
    throw std::invalid_argument("KeyboardHandlerUnixImpl read_fn must be non-empty.");
  }
  // 检查 isatty_fn 是否为空
  // Check if isatty_fn is empty
  if (isatty_fn == nullptr) {
    throw std::invalid_argument("KeyboardHandlerUnixImpl isatty_fn must be non-empty.");
  }
  // 检查 tcgetattr_fn 是否为空
  // Check if tcgetattr_fn is empty
  if (tcgetattr_fn == nullptr) {
    throw std::invalid_argument("KeyboardHandlerUnixImpl tcgetattr_fn must be non-empty.");
  }
  // 检查 tcsetattr_fn 是否为空
  // Check if tcsetattr_fn is empty
  if (tcsetattr_fn == nullptr) {
    throw std::invalid_argument("KeyboardHandlerUnixImpl tcsetattr_fn must be non-empty.");
  }
  // 设置 tcsetattr_fn_
  // Set tcsetattr_fn_
  tcsetattr_fn_ = tcsetattr_fn;

  // 初始化 key_codes_map_，将默认的终端序列映射到内部代码
  // Initialize key_codes_map_, map default terminal sequences to internal codes
  for (size_t i = 0; i < STATIC_KEY_MAP_LENGTH; i++) {
    key_codes_map_.emplace(
      DEFAULT_STATIC_KEY_MAP[i].terminal_sequence, DEFAULT_STATIC_KEY_MAP[i].inner_code);
  }

  /**
   * @brief 检查我们是否可以从标准输入处理按键。Check if we can handle key press from standard input.
   */
  if (!isatty_fn(stdin_fd_)) {
    /**
     * 如果stdin不是一个真正的终端（重定向到文本文件或管道），则无法在此处使用键盘处理。
     * If stdin is not a real terminal (redirected to text file or pipe), can't do much here with keyboard handling.
     */
    std::cerr << "stdin is not a terminal device. Keyboard handling disabled.";
    return;
  }

  struct termios new_term_settings;

  /**
   * 获取终端属性。Get the terminal attributes.
   */
  if (tcgetattr_fn(stdin_fd_, &old_term_settings_) == -1) {
    throw std::runtime_error("Error in tcgetattr(). errno = " + std::to_string(errno));
  }

  if (install_signal_handler) {
    /**
     * 设置信号处理器以返回。Setup signal handler to return.
     * 在异常程序终止时，将终端设置为原始（缓冲）模式。Set the terminal to its original (buffered) mode in case of abnormal program termination.
     */
    old_sigint_handler_ = std::signal(SIGINT, KeyboardHandlerUnixImpl::on_signal);
    if (old_sigint_handler_ == SIG_ERR) {
      throw std::runtime_error("Error. Can't install SIGINT handler");
    }
  }
  install_signal_handler_ = install_signal_handler;

  new_term_settings = old_term_settings_;

  /**
   * 将stdin设置为无缓冲模式，以便直接从stdin读取。Set stdin to unbuffered mode for reading directly from stdin.
   * 禁用规范输入和禁用回显。Disable canonical input and disable echo.
   */
  new_term_settings.c_lflag &= ~(ICANON | ECHO);
  new_term_settings.c_cc[VMIN] =
    0; // 0表示纯粹的超时驱动读出。0 means purely timeout-driven readout.
  new_term_settings.c_cc[VTIME] =
    1; // 自read()调用开始以来，最多等待0.1秒。Wait maximum for 0.1 sec since the start of the read() call.

  /**
   * 设置终端属性。Set the terminal attributes.
   */
  if (tcsetattr_fn_(stdin_fd_, TCSANOW, &new_term_settings) == -1) {
    throw std::runtime_error("Error in tcsetattr(). errno = " + std::to_string(errno));
  }
  is_init_succeed_ = true;

  /**
 * @brief key_handler_thread_ 是一个线程，用于处理从标准输入读取的按键事件。
 * key_handler_thread_ is a thread for handling key events read from standard input.
 */
  key_handler_thread_ = std::thread([=]() {
    try {
      // 定义缓冲区长度为 10
      // Define the buffer length as 10
      static constexpr size_t BUFF_LEN = 10;
      // 初始化字符缓冲区
      // Initialize the character buffer
      char buff[BUFF_LEN] = {0};
      do {
        // 从标准输入读取按键信息
        // Read key information from standard input
        ssize_t read_bytes = read_fn(stdin_fd_, buff, BUFF_LEN);
        // 如果读取出错且错误不是 EAGAIN（资源暂时不可用），则抛出异常
        // If an error occurs while reading and the error is not EAGAIN (resource temporarily unavailable), throw an exception
        if (read_bytes < 0 && errno != EAGAIN) {
          throw std::runtime_error("Error in read(). errno = " + std::to_string(errno));
        }

        // 如果读取到的字节数为 0，则什么都不做。0 表示 read() 被超时返回。
        // If the number of bytes read is 0, do nothing. 0 means read() returned by timeout.
        if (read_bytes == 0) {
          // Do nothing. 0 means read() returned by timeout.
        } else { // read_bytes > 0
          // 设置缓冲区末尾的空字符
          // Set the null character at the end of the buffer
          buff[std::min(BUFF_LEN - 1, static_cast<size_t>(read_bytes))] = '\0';

          // 解析输入的按键信息
          // Parse the input key information
          auto key_code_and_modifiers = parse_input(buff, read_bytes);

          // 获取按键代码和按键修饰符
          // Get the key code and key modifiers
          KeyCode pressed_key_code = std::get<0>(key_code_and_modifiers);
          KeyModifiers key_modifiers = std::get<1>(key_code_and_modifiers);

#ifdef PRINT_DEBUG_INFO
          // 打印调试信息
          // Print debug information
          auto modifiers_str = enum_key_modifiers_to_str(key_modifiers);
          std::cout << "pressed key: " << modifiers_str;
          if (!modifiers_str.empty()) {
            std::cout << " + ";
          }
          std::cout << "'" << enum_key_code_to_str(pressed_key_code) << "'" << std::endl;
#endif
          // 使用互斥锁保护回调函数
          // Use a mutex lock to protect the callback function
          std::lock_guard<std::mutex> lk(callbacks_mutex_);
          // 查找并执行匹配的回调函数
          // Find and execute the matching callback function
          auto range = callbacks_.equal_range(KeyAndModifiers{pressed_key_code, key_modifiers});
          for (auto it = range.first; it != range.second; ++it) {
            it->second.callback(pressed_key_code, key_modifiers);
          }
        }
      } while (!exit_.load());
    } catch (...) {
      // 捕获异常并存储到 thread_exception_ptr
      // Catch exceptions and store them in thread_exception_ptr
      thread_exception_ptr = std::current_exception();
    }

    // 恢复标准输入的缓冲模式
    // Restore the buffer mode for standard input
    if (!restore_buffer_mode_for_stdin()) {
      if (thread_exception_ptr == nullptr) {
        try {
          throw std::runtime_error(
            "Error in tcsetattr old_term_settings. errno = " + std::to_string(errno));
        } catch (...) {
          thread_exception_ptr = std::current_exception();
        }
      } else {
        std::cerr << "Error in tcsetattr old_term_settings. errno = " + std::to_string(errno)
                  << std::endl;
      }
    }
  });
}

/**
 * @brief 销毁 KeyboardHandlerUnixImpl 对象 (Destructor for KeyboardHandlerUnixImpl)
 */
KeyboardHandlerUnixImpl::~KeyboardHandlerUnixImpl()
{
  // 如果需要安装信号处理器 (If signal handler installation is required)
  if (install_signal_handler_) {
    // 恢复旧的 SIGINT 信号处理器 (Restore old SIGINT signal handler)
    signal_handler_type old_sigint_handler = std::signal(SIGINT, old_sigint_handler_);
    // 如果无法恢复旧的 SIGINT 信号处理器，输出错误信息 (If the old SIGINT signal handler cannot be restored, output an error message)
    if (old_sigint_handler == SIG_ERR) {
      std::cerr << "Error. Can't install old SIGINT handler" << std::endl;
    }
    // 如果旧的 SIGINT 信号处理器不等于我们自定义的处理器，说明有其他处理器覆盖了我们的处理器 (If the old SIGINT signal handler is not equal to our custom handler, it means that another handler has overridden our handler)
    if (old_sigint_handler != KeyboardHandlerUnixImpl::on_signal) {
      std::cerr << "Error. Can't return old SIGINT handler, someone override our signal handler"
                << std::endl;
      // 返回被覆盖的信号处理器 (Return the overridden signal handler)
      std::signal(SIGINT, old_sigint_handler);
    }
  }
  // 设置退出标志为 true (Set exit flag to true)
  exit_ = true;
  // 如果键处理线程可连接，则连接该线程 (If the key handling thread is joinable, join it)
  if (key_handler_thread_.joinable()) {
    key_handler_thread_.join();
  }

  // 检查线程异常 (Check for thread exceptions)
  try {
    if (thread_exception_ptr != nullptr) {
      std::rethrow_exception(thread_exception_ptr);
    }
  } catch (const std::exception & e) {
    std::cerr << "Caught exception: \"" << e.what() << "\"\n";
  } catch (...) {
    std::cerr << "Caught unknown exception" << std::endl;
  }
}

/**
 * @brief 获取终端序列 (Get terminal sequence)
 * @param key_code 键盘按键编码 (Keyboard key code)
 * @return 终端序列字符串 (Terminal sequence string)
 */
KEYBOARD_HANDLER_PUBLIC
std::string
KeyboardHandlerUnixImpl::get_terminal_sequence(KeyboardHandlerUnixImpl::KeyCode key_code)
{
  std::string ret_str{};
  // 遍历 key_codes_map_ 查找对应的终端序列 (Iterate through key_codes_map_ to find the corresponding terminal sequence)
  for (const auto & it : key_codes_map_) {
    if (it.second == key_code) {
      return it.first;
    }
  }
  // 如果没有找到对应的终端序列，返回空字符串 (If no corresponding terminal sequence is found, return an empty string)
  return ret_str;
}

/**
 * @brief 为标准输入恢复缓冲模式 (Restore buffer mode for stdin)
 * @return 是否成功恢复缓冲模式 (Whether the buffer mode was successfully restored)
 */
bool KeyboardHandlerUnixImpl::restore_buffer_mode_for_stdin()
{
  // 使用 tcsetattr_fn_ 函数为标准输入恢复缓冲模式 (Use the tcsetattr_fn_ function to restore buffer mode for stdin)
  if (tcsetattr_fn_(fileno(stdin), TCSANOW, &old_term_settings_) == -1) {
    return false;
  }
  return true;
}

// 定义一个名为 KeyboardHandlerUnixImpl 的类，其中包含一个成员函数 get_old_sigint_handler()
// Define a class named KeyboardHandlerUnixImpl, which contains a member function get_old_sigint_handler()

/**
 * @brief 获取旧的 SIGINT 信号处理器 (Get the old SIGINT signal handler)
 * 此函数用于获取之前设置的 SIGINT 信号处理器。SIGINT 信号通常在用户按下 Ctrl+C 时发送。
 * This function is used to get the previously set SIGINT signal handler. The SIGINT signal is usually sent when the user presses Ctrl+C.
 *
 * @return 返回旧的 SIGINT 信号处理器对象 (Returns the old SIGINT signal handler object)
 * 此返回值是一个 signal_handler_type 类型的对象，表示旧的 SIGINT 信号处理器。
 * This return value is an object of type signal_handler_type, representing the old SIGINT signal handler.
 */
KeyboardHandlerUnixImpl::signal_handler_type KeyboardHandlerUnixImpl::get_old_sigint_handler()
{
  // 返回旧的 SIGINT 信号处理器
  // Return the old SIGINT signal handler
  // 这一行代码返回类的私有成员变量 old_sigint_handler_，它是一个 signal_handler_type 类型的对象。
  // This line of code returns the private member variable old_sigint_handler_ of the class, which is an object of type signal_handler_type.
  return old_sigint_handler_;
}

#endif // #ifndef _WIN32
