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

#ifndef KEYBOARD_HANDLER__KEYBOARD_HANDLER_WINDOWS_IMPL_HPP_
#define KEYBOARD_HANDLER__KEYBOARD_HANDLER_WINDOWS_IMPL_HPP_

#include "keyboard_handler/visibility_control.hpp"
#include "keyboard_handler_base.hpp"
#include <atomic>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <unordered_map>

/// \brief Windows 特定的键盘处理器类实现。 (Windows specific implementation of keyboard handler class.)
/// \note 设计和实现限制： (Design and implementation limitations:)
/// 无法检测 CTRL + ALT 组合。 (Can't detect CTRL + ALT combinations.)
/// 无法检测 CTRL + 0..9 数字键。 (Can't detect CTRL + 0..9 number keys.)
/// 无法检测 ALT + F1..12 按键。 (Can't detect ALT + F1..12 keys.)
/// 而不是检测 CTRL + SHIFT + 键，只会检测到 CTRL + 键。 (Instead of CTRL + SHIFT + key, only CTRL + key will be detected.)
/// 当同时按下多个键修饰符时，某些键可能被错误地检测到。 (Some keys might be incorrectly detected with multiple key modifiers pressed at the same time.)

class KeyboardHandlerWindowsImpl : public KeyboardHandlerBase
{
public:
  // 定义 isattyFunction 类型为一个接受整数参数并返回整数的函数对象。 (Define isattyFunction type as a function object taking an integer argument and returning an integer.)
  using isattyFunction = std::function<int(int)>;

  // 定义 kbhitFunction 类型为一个不接受参数并返回整数的函数对象。 (Define kbhitFunction type as a function object taking no arguments and returning an integer.)
  using kbhitFunction = std::function<int(void)>;

  // 定义 getchFunction 类型为一个不接受参数并返回整数的函数对象。 (Define getchFunction type as a function object taking no arguments and returning an integer.)
  using getchFunction = std::function<int(void)>;

  /// \brief 数据类型，表示_getch()函数响应按键时返回的键码。(Data type for representing key codes returning by _getch() function in response to the pressing keyboard keys.)
  /// \details 在按下键盘键组合时，Windows操作系统可以返回最多两个整数值。如果只返回一个值，它将放在`first`字段中，第二个字段将用NOT_A_KEY值初始化。(Windows OS could return up to two integer values in response to the pressing keyboard key combination. If Windows returning only one value it will be placed in `first` field and second field will be initialized with NOT_A_KEY value.)
  struct WinKeyCode
  {
    int first;  ///< 第一个键码。(The first key code.)
    int second; ///< 第二个键码。(The second key code.)

    /// \brief 预定义值，对应于WinKeyCode元素的无效或空值。(Predefined value which is corresponding to the invalid or empty value for WinKeyCode elements.)
    static constexpr int NOT_A_KEY = -1;

    /// \brief 比较两个WinKeyCodes相等的操作符。(Equality operator for comparison of two WinKeyCodes.)
    /// \param rhs 右侧用于比较的值。(Value for comparison from right hand side.)
    /// \return 如果两个键码相等，则返回true，否则返回false。(Returns true if two key codes are equal, otherwise false.)
    bool operator==(const WinKeyCode & rhs) const
    {
      if (first == rhs.first && second == rhs.second) {
        return true;
      } else {
        return false;
      }
    }

    /// \brief 比较两个WinKeyCodes不相等的操作符。(Not equal operator for comparison of two WinKeyCodes.)
    /// \param rhs 右侧用于比较的值。(Value for comparison from right hand side.)
    /// \return 如果两个键码不相等，则返回true，否则返回false。(Returns true if two key codes are not equal, otherwise false.)
    bool operator!=(const WinKeyCode & rhs) const { return !operator==(rhs); }
  };

  /// \brief 默认构造函数 (Default constructor)
  ///
  KEYBOARD_HANDLER_PUBLIC
  KeyboardHandlerWindowsImpl();

  /// \brief 析构函数 (Destructor)
  ///
  KEYBOARD_HANDLER_PUBLIC
  virtual ~KeyboardHandlerWindowsImpl();

  /// \brief 将 WinKeyCode 转换为键代码和键修饰符枚举值。 (Translates WinKeyCode to the key code and key modifiers enum values.)
  /// \param win_key_code Windows操作系统返回的按下键盘键的键代码。 (Key codes returning by Windows OS in response to the pressing keyboard keys.)
  /// \return 键代码和代码修饰符掩码的元组。 (tuple key code and code modifiers mask.)
  KEYBOARD_HANDLER_PUBLIC
  std::tuple<KeyboardHandlerBase::KeyCode, KeyboardHandlerBase::KeyModifiers>
  win_key_code_to_enums(WinKeyCode win_key_code) const;

  /// \brief 将内部定义的 KeyCode 枚举值转换为内部查找表中相应的 WinKeyCode。 (Translates internally defined KeyCode enum value to the corresponding WinKeyCode registered in inner look up table.)
  /// \param key_code 内部定义的 KeyCode 枚举值。 (Internally defined KeyCode enum value.)
  /// \return 返回内部查找表中与输入的 KeyCode 枚举值相对应的 WinKeyCode 值。如果在内部 LUT 中未找到相应的 WinKeyCode 值，则返回带有 NOT_A_KEY 值的 WinKeyCode。 (Returns WinKeyCode value corresponding to the input KeyCode enum value in the inner lookup table. If corresponding WinKeyCode value not found in inner LUT will return WinKeyCode with NOT_A_KEY values.)
  KEYBOARD_HANDLER_PUBLIC
  WinKeyCode enum_key_code_to_win_code(KeyboardHandlerBase::KeyCode key_code) const;

protected:
  /// \brief 构造函数，带有对系统功能的引用。单元测试所需。
  /// \brief Constructor with references to the system functions. Required for unit tests.
  /// \param isatty_fn 系统 _isatty(int) 函数的引用
  /// \param isatty_fn Reference to the system _isatty(int) function
  /// \param kbhit_fn 系统 _kbhit(void) 函数的引用
  /// \param kbhit_fn Reference to the system _kbhit(void) function
  /// \param getch_fn 系统 _getch(void) 函数的引用
  /// \param getch_fn Reference to the system _getch(void) function
  KEYBOARD_HANDLER_PUBLIC
  KeyboardHandlerWindowsImpl(
    const isattyFunction & isatty_fn,
    const kbhitFunction & kbhit_fn,
    const getchFunction & getch_fn);

  /// \brief WinKeyCode 键的 `unordered_map` 的专用哈希函数
  /// \brief Specialized hash function for `unordered_map` with WinKeyCode keys
  struct win_key_code_hash_fn
  {
    std::size_t operator()(const WinKeyCode & key_code) const
    {
      // 对键码的第一部分进行哈希，并与对键码的第二部分进行左移 1 位的哈希结果进行异或操作
      return std::hash<int>()(key_code.first) ^ (std::hash<int>()(key_code.second) << 1);
    }
  };

  /// \brief 映射 KeyCode 枚举值到 _getch() 返回的按下键盘键时期望的键码序列的数据类型
  /// \brief Data type for mapping KeyCode enum value to the expecting sequence of key codes
  /// returning by _getch() in response to the pressing keyboard keys.
  struct KeyMap
  {
    KeyboardHandlerBase::KeyCode inner_code;
    WinKeyCode win_key_code;
  };

  /// \brief 默认静态定义的查找表，用于对应 KeyCode 枚举值和按下键盘键时 _getch() 返回的期望键码序列。
  /// \brief Default statically defined lookup table for corresponding KeyCode enum value and
  /// expecting sequence of key codes returning by _getch() in response to the pressing keyboard
  /// keys.
  static const KeyMap DEFAULT_STATIC_KEY_MAP[];

  /// \brief DEFAULT_STATIC_KEY_MAP 的长度（以元素数量计算）。
  /// \brief Length of DEFAULT_STATIC_KEY_MAP measured in number of elements.
  static const size_t STATIC_KEY_MAP_LENGTH;

private:
  std::thread key_handler_thread_;
  std::atomic_bool exit_;
  std::unordered_map<WinKeyCode, KeyCode, win_key_code_hash_fn> key_codes_map_;
  std::exception_ptr thread_exception_ptr = nullptr;
};

#endif // KEYBOARD_HANDLER__KEYBOARD_HANDLER_WINDOWS_IMPL_HPP_
