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

#ifdef _WIN32
#include "keyboard_handler/keyboard_handler_windows_impl.hpp"
#include <conio.h>
#include <exception>
#include <io.h>
#include <iostream>
#include <stdio.h>
#include <tuple>
#include <windows.h>

/**
 * @brief 构造函数，用于创建 KeyboardHandlerWindowsImpl 对象。 (Constructor for creating a KeyboardHandlerWindowsImpl object.)
 *
 * @param _isatty 函数指针，检查文件描述符是否引用终端设备。 (Function pointer to check if the file descriptor references a terminal device.)
 * @param _kbhit 函数指针，检查键盘是否有按键事件。 (Function pointer to check if a keyboard key event is present.)
 * @param _getch 函数指针，获取按下的键的字符。 (Function pointer to get the character of the pressed key.)
 */
KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerWindowsImpl::KeyboardHandlerWindowsImpl()
: KeyboardHandlerWindowsImpl(_isatty, _kbhit, _getch)
{
  // 构造函数为空，因为已经在初始化列表中完成了所有工作。 (The constructor body is empty because all work is done in the initialization list.)
}

/**
 * @brief 构造 KeyboardHandlerWindowsImpl 类的实例 (Constructs an instance of the KeyboardHandlerWindowsImpl class)
 *
 * @param isatty_fn 一个函数指针，用于检查文件描述符是否表示终端设备 (A function pointer to check if a file descriptor represents a terminal device)
 * @param kbhit_fn 一个函数指针，用于检查键盘缓冲区中是否有按键事件 (A function pointer to check if there are key events in the keyboard buffer)
 * @param getch_fn 一个函数指针，用于从键盘缓冲区获取按键事件 (A function pointer to get key events from the keyboard buffer)
 */
KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerWindowsImpl::KeyboardHandlerWindowsImpl(
  const isattyFunction & isatty_fn, const kbhitFunction & kbhit_fn, const getchFunction & getch_fn)
: exit_(false)
{
  // 检查 isatty_fn 是否为空 (Check if isatty_fn is empty)
  if (isatty_fn == nullptr) {
    throw std::invalid_argument("KeyboardHandlerWindowsImpl isatty_fn must be non-empty.");
  }
  // 检查 kbhit_fn 是否为空 (Check if kbhit_fn is empty)
  if (kbhit_fn == nullptr) {
    throw std::invalid_argument("KeyboardHandlerWindowsImpl kbhit_fn must be non-empty.");
  }
  // 检查 getch_fn 是否为空 (Check if getch_fn is empty)
  if (getch_fn == nullptr) {
    throw std::invalid_argument("KeyboardHandlerWindowsImpl getch_fn must be non-empty.");
  }

  // 初始化 key_codes_map_ (Initialize key_codes_map_)
  for (size_t i = 0; i < STATIC_KEY_MAP_LENGTH; i++) {
    key_codes_map_.emplace(
      DEFAULT_STATIC_KEY_MAP[i].win_key_code, DEFAULT_STATIC_KEY_MAP[i].inner_code);
  }

  // 检查是否可以从标准输入处理按键事件 (Check if we can handle key press events from standard input)
  if (!isatty_fn(_fileno(stdin))) {
    // 如果 stdin 不是真正的终端或控制台设备（重定向到文件或管道），则无法在此处进行键盘处理。
    // (If stdin is not a real terminal or console device (redirected to file or pipe), can't do much here with keyboard handling.)
    std::cerr << "stdin is not a terminal or console device. Keyboard handling disabled.";
    return;
  }

  // 标记初始化成功 (Mark initialization as successful)
  is_init_succeed_ = true;

  /*!
 * \brief 键盘处理线程 (Keyboard handling thread)
 * \details 该线程用于检测按键并调用相应的回调函数 (This thread is used to detect key presses and invoke corresponding callbacks)
 */
  key_handler_thread_ = std::thread([=]() {
    try {
      do { // 循环以持续检查按键 (Loop to continuously check for key presses)
        // 检查是否有按键按下 (Check if a key has been pressed)
        if (kbhit_fn()) {
          // 初始化 WinKeyCode 和 KeyModifiers (Initialize WinKeyCode and KeyModifiers)
          WinKeyCode win_key_code{WinKeyCode::NOT_A_KEY, WinKeyCode::NOT_A_KEY};
          KeyModifiers key_modifiers = KeyModifiers::NONE;

          // 获取按下的键的 ASCII 码 (Get the ASCII code of the pressed key)
          int ch = getch_fn();
          win_key_code.first = ch;

          // 检查是否同时按下了 Alt 键 (Check if the Alt key was pressed at the same time)
          if (::GetAsyncKeyState(VK_MENU) & 0x8000) {
            key_modifiers = KeyModifiers::ALT;
          }

          // 当读取功能键或箭头键时，getch 函数必须调用两次；
          // 第一次调用返回 0 或 0xE0，第二次调用返回实际的键码。
          // When reading a function key or an arrow key, getch function must be called twice;
          // the first call returns 0 or 0xE0, and the second call returns the actual key code.
          // https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2012/078sfkak(v=vs.110)
          if (ch == 0 || ch == 0xE0) { // 0xE0 == 224
            // 对于 F1 - F10 键，ch == 0；对于所有其他控制键，ch == 0xE0。
            // ch == 0 for F1 - F10 keys, ch == 0xE0 for all other control keys.
            ch = getch_fn();
            win_key_code.second = ch;
          }

          // 将 WinKeyCode 转换为 KeyCode 和 KeyModifiers (Convert WinKeyCode to KeyCode and KeyModifiers)
          auto key_code_and_modifiers = win_key_code_to_enums(win_key_code);
          KeyCode pressed_key_code = std::get<0>(key_code_and_modifiers);
          key_modifiers = key_modifiers | std::get<1>(key_code_and_modifiers);

#ifdef PRINT_DEBUG_INFO
          // 打印调试信息 (Print debug information)
          std::cout << "Pressed first key code = " << win_key_code.first << ". ";
          std::cout << "Second code = " << win_key_code.second << ".";
          auto modifiers_str = enum_key_modifiers_to_str(key_modifiers);
          std::cout << " Detected as pressed key: " << modifiers_str;
          if (!modifiers_str.empty()) {
            std::cout << " + ";
          }
          std::cout << "'" << enum_key_code_to_str(pressed_key_code) << "'" << std::endl;
#endif
          // 锁定回调互斥体并执行相应的回调函数 (Lock the callbacks mutex and execute corresponding callback functions)
          std::lock_guard<std::mutex> lk(callbacks_mutex_);
          auto range = callbacks_.equal_range(KeyAndModifiers{pressed_key_code, key_modifiers});
          for (auto it = range.first; it != range.second; ++it) {
            it->second.callback(pressed_key_code, key_modifiers);
          }

          // 等待 0.1 秒以让其他线程使用处理器资源 (Wait for 0.1 sec to yield processor resources for other threads)
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      } while (!exit_.load()); // 当 exit_ 为 true 时，退出循环 (Exit the loop when exit_ is true)
    } catch (
      ...) { // 捕获异常并将其存储在 thread_exception_ptr 中 (Catch exceptions and store them in thread_exception_ptr)
      thread_exception_ptr = std::current_exception();
    }
  });
}

/**
 * @brief 键盘处理器 Windows 实现的析构函数 (Destructor of KeyboardHandlerWindowsImpl)
 *
 * @details 当对象被销毁时，此析构函数会确保线程正确地关闭并释放资源 (When the object is destroyed, this destructor ensures that the thread is properly closed and resources are released)
 */
KeyboardHandlerWindowsImpl::~KeyboardHandlerWindowsImpl()
{
  exit_ =
    true; // 将退出标志设置为 true，以便在关键处理程序线程中检查 (Set the exit flag to true so it can be checked in the key handler thread)

  // 如果键处理程序线程可连接，则等待它完成 (If the key handler thread is joinable, wait for it to complete)
  if (key_handler_thread_.joinable()) {
    key_handler_thread_.join();
  }

  try {
    // 如果线程异常指针不为空，则重新抛出异常 (If the thread exception pointer is not null, rethrow the exception)
    if (thread_exception_ptr != nullptr) {
      std::rethrow_exception(thread_exception_ptr);
    }
  } catch (const std::exception & e) { // 捕获并处理已知异常 (Catch and handle known exceptions)
    std::cerr << "Caught exception \"" << e.what() << "\"\n";
  } catch (...) { // 捕获并处理未知异常 (Catch and handle unknown exceptions)
    std::cerr << "Caught unknown exception" << std::endl;
  }
}

/**
 * @brief 将 Windows 键码转换为 KeyCode 和 KeyModifiers 枚举值的元组 (Converts Windows key code to a tuple of KeyCode and KeyModifiers enums)
 *
 * @param[in] win_key_code 一个包含 Windows 键码的 std::pair 对象 (A std::pair object containing the Windows key code)
 * @return 一个包含 KeyCode 和 KeyModifiers 枚举值的元组 (A tuple containing KeyCode and KeyModifiers enums)
 */
KEYBOARD_HANDLER_PUBLIC
std::tuple<KeyboardHandlerBase::KeyCode, KeyboardHandlerBase::KeyModifiers>
KeyboardHandlerWindowsImpl::win_key_code_to_enums(WinKeyCode win_key_code) const
{
  // 初始化按键代码和按键修饰符 (Initialize pressed key code and key modifiers)
  KeyCode pressed_key_code = KeyCode::UNKNOWN;
  KeyModifiers key_modifiers = KeyModifiers::NONE;

  // 处理 F1..F10 的 CTRL 组合键 (Handle CTRL combination keys for F1..F10)
  if (win_key_code.first == 0 && win_key_code.second >= 94 && win_key_code.second <= 103) {
    win_key_code.second -= 35; // F1..F10
    key_modifiers = key_modifiers | KeyModifiers::CTRL;
  }

  // 处理 F11 和 F12 的 CTRL 组合键 (Handle CTRL combination keys for F11 and F12)
  if (win_key_code.first == 0xE0 && (win_key_code.second == 138 || win_key_code.second == 137)) {
    win_key_code.second -= 4; // F11 and F12
    key_modifiers = key_modifiers | KeyModifiers::CTRL;
  }

  // 处理 F1..F10 的 SHIFT 组合键 (Handle SHIFT combination keys for F1..F10)
  if (win_key_code.first == 0 && win_key_code.second >= 84 && win_key_code.second <= 93) {
    win_key_code.second -= 25; // F1..F10
    key_modifiers = key_modifiers | KeyModifiers::SHIFT;
  }

  // 处理 F11 和 F12 的 SHIFT 组合键 (Handle SHIFT combination keys for F11 and F12)
  if (win_key_code.first == 0xE0 && (win_key_code.second == 135 || win_key_code.second == 136)) {
    win_key_code.second -= 2; // F11 and F12
    key_modifiers = key_modifiers | KeyModifiers::SHIFT;
  }

  // 处理 A 到 Z 的按键并添加 SHIFT 修饰符 (Handle keys from A to Z and add SHIFT modifier)
  if (win_key_code.first >= 'A' && win_key_code.first <= 'Z') {
    win_key_code.first += 32;
    key_modifiers = key_modifiers | KeyModifiers::SHIFT;
  }

  // 在 key_codes_map_ 中查找按键代码 (Search for the key code in key_codes_map_)
  auto key_map_it = key_codes_map_.find(win_key_code);
  if (key_map_it != key_codes_map_.end()) {
    pressed_key_code = key_map_it->second;
  }

  // 如果在 key_codes_map_ 中未找到按键代码，尝试查找小写字符的 CTRL 组合键 (If the key code is not found in key_codes_map_, try searching for CTRL combination keys with lowercase characters)
  if (
    pressed_key_code == KeyCode::UNKNOWN && win_key_code.second == WinKeyCode::NOT_A_KEY &&
    win_key_code.first >= 0 && win_key_code.first <= 26) {
    win_key_code.first += 96; // small chars
    key_modifiers = key_modifiers | KeyModifiers::CTRL;

    auto key_map_it = key_codes_map_.find(win_key_code);
    if (key_map_it != key_codes_map_.end()) {
      pressed_key_code = key_map_it->second;
    }
  }

  // 返回按键代码和按键修饰符的元组 (Return the tuple of pressed key code and key modifiers)
  return std::make_tuple(pressed_key_code, key_modifiers);
}

/**
 * @brief 将枚举键码转换为 Windows 键码 (Converts the enumeration key code to Windows key code)
 *
 * @param[in] key_code 键盘处理器基类中的键码 (Key code from KeyboardHandlerBase)
 * @return WinKeyCode 返回 Windows 键码结构体 (Returns a structure with Windows key codes)
 */
KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerWindowsImpl::WinKeyCode
KeyboardHandlerWindowsImpl::enum_key_code_to_win_code(KeyboardHandlerBase::KeyCode key_code) const
{
  // 遍历 key_codes_map_ 中的每一项 (Iterate through each item in key_codes_map_)
  for (const auto & it : key_codes_map_) {
    // 如果找到与给定 key_code 匹配的项 (If a matching item is found for the given key_code)
    if (it.second == key_code) {
      // 返回匹配项的 Windows 键码 (Return the Windows key code of the matching item)
      return it.first;
    }
  }

  // 如果没有找到匹配的项，返回一个表示无效键的结构体 (If no matching item is found, return a structure representing an invalid key)
  return {WinKeyCode::NOT_A_KEY, WinKeyCode::NOT_A_KEY};
}

#endif // #ifdef _WIN32
