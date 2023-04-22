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

#include "keyboard_handler/keyboard_handler_base.hpp"
#include <atomic>
#include <sstream>
#include <string>

// 定义一个常量表达式，表示无效的回调句柄（Define a constant expression representing an invalid callback handle）
KEYBOARD_HANDLER_PUBLIC
constexpr KeyboardHandlerBase::callback_handle_t KeyboardHandlerBase::invalid_handle;

/**
 * @brief 添加按键按下回调函数（Add key press callback function）
 *
 * @param[in] callback 回调函数（Callback function）
 * @param[in] key_code 按键代码（Key code）
 * @param[in] key_modifiers 按键修饰符（Key modifiers）
 * @return 返回新的回调句柄（Return the new callback handle）
 */
KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerBase::callback_handle_t KeyboardHandlerBase::add_key_press_callback(
  const callback_t & callback,
  KeyboardHandlerBase::KeyCode key_code,
  KeyboardHandlerBase::KeyModifiers key_modifiers)
{
  // 如果回调为空或初始化未成功，则返回无效句柄（If the callback is empty or initialization is unsuccessful, return an invalid handle）
  if (callback == nullptr || !is_init_succeed_) {
    return invalid_handle;
  }
  // 使用互斥锁保护回调列表（Protect the callback list with a mutex lock）
  std::lock_guard<std::mutex> lk(callbacks_mutex_);
  // 获取新的回调句柄（Get a new callback handle）
  callback_handle_t new_handle = get_new_handle();
  // 将新的回调数据添加到回调列表中（Add the new callback data to the callback list）
  callbacks_.emplace(KeyAndModifiers{key_code, key_modifiers}, callback_data{new_handle, callback});
  // 返回新的回调句柄（Return the new callback handle）
  return new_handle;
}

/**
 * @brief 重载逻辑与操作符（Overload logical AND operator）
 *
 * @param[in] left 左操作数（Left operand）
 * @param[in] right 右操作数（Right operand）
 * @return 返回两个按键修饰符的逻辑与结果（Return the logical AND result of two key modifiers）
 */
KEYBOARD_HANDLER_PUBLIC
bool operator&&(
  const KeyboardHandlerBase::KeyModifiers & left, const KeyboardHandlerBase::KeyModifiers & right)
{
  using KeyModifiers = KeyboardHandlerBase::KeyModifiers;
  /* *INDENT-OFF* */
  return static_cast<std::underlying_type_t<KeyModifiers>>(left) &
         static_cast<std::underlying_type_t<KeyModifiers>>(right);
  /* *INDENT-ON* */
}

/**
 * @brief 重载按位或操作符（Overload bitwise OR operator）
 *
 * @param[in] left 左操作数（Left operand）
 * @param[in] right 右操作数（Right operand）
 * @return 返回两个按键修饰符的按位或结果（Return the bitwise OR result of two key modifiers）
 */
KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerBase::KeyModifiers
operator|(KeyboardHandlerBase::KeyModifiers left, const KeyboardHandlerBase::KeyModifiers & right)
{
  using KeyModifiers = KeyboardHandlerBase::KeyModifiers;
  /* *INDENT-OFF* */
  left = static_cast<KeyModifiers>(
    static_cast<std::underlying_type_t<KeyModifiers>>(left) |
    static_cast<std::underlying_type_t<KeyModifiers>>(right));
  /* *INDENT-ON* */
  return left;
}

/**
 * @brief 重载自增操作符（Overload increment operator）
 *
 * @param[in, out] key_code 按键代码（Key code）
 * @return 返回自增后的按键代码（Return the incremented key code）
 */
KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerBase::KeyCode & operator++(KeyboardHandlerBase::KeyCode & key_code)
{
  using KeyCode = KeyboardHandlerBase::KeyCode;
  /* *INDENT-OFF* */
  key_code = static_cast<KeyCode>(static_cast<std::underlying_type_t<KeyCode>>(key_code) + 1);
  /* *INDENT-ON* */
  return key_code;
}

/**
 * @brief 将按键代码转换为字符串（Convert key code to string）
 *
 * @param[in] key_code 按键代码（Key code）
 * @return 返回对应的字符串（Return the corresponding string）
 */
KEYBOARD_HANDLER_PUBLIC
std::string enum_key_code_to_str(KeyboardHandlerBase::KeyCode key_code)
{
  // 遍历 ENUM_KEY_TO_STR_MAP 查找对应的字符串（Traverse ENUM_KEY_TO_STR_MAP to find the corresponding string）
  for (auto & it : ENUM_KEY_TO_STR_MAP) {
    if (it.inner_code == key_code) {
      return it.str;
    }
  }
  return std::string();
}

/**
 * \brief 将字符串值转换为键码表示。Translate str value to its keycode representation.
 * \param[in] key_code_str 字符串形式的键码。String key code.
 * \return 键盘处理器基类中的键码。KeyboardHandlerBase::Keycode
 */
KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerBase::KeyCode enum_str_to_key_code(const std::string & key_code_str)
{
  // 遍历 ENUM_KEY_TO_STR_MAP 中的元素。Iterate through the elements in ENUM_KEY_TO_STR_MAP.
  for (auto & it : ENUM_KEY_TO_STR_MAP) {
    // 如果找到匹配的字符串，返回对应的内部键码。If the matching string is found, return the corresponding inner code.
    if (it.str == key_code_str) {
      return it.inner_code;
    }
  }
  // 如果没有找到匹配的字符串，返回未知键码。If no matching string is found, return UNKNOWN keycode.
  return KeyboardHandlerBase::KeyCode::UNKNOWN;
}

/**
 * \brief 将键修饰符枚举值转换为字符串。Convert enum key modifiers to string.
 * \param[in] key_modifiers 键盘处理器基类中的键修饰符。KeyModifiers from KeyboardHandlerBase.
 * \return 转换后的字符串。Converted string.
 */
KEYBOARD_HANDLER_PUBLIC
std::string enum_key_modifiers_to_str(KeyboardHandlerBase::KeyModifiers key_modifiers)
{
  using KeyModifiers = KeyboardHandlerBase::KeyModifiers;
  std::stringstream ss;
  // 判断是否有 SHIFT 修饰符并添加到字符串流。Check for SHIFT modifier and add it to the stringstream.
  if (key_modifiers && KeyModifiers::SHIFT) {
    ss << "SHIFT";
  }
  // 判断是否有 CTRL 修饰符并添加到字符串流。Check for CTRL modifier and add it to the stringstream.
  if (key_modifiers && KeyModifiers::CTRL) {
    ss.str().empty() ? ss << "CTRL" : ss << " CTRL";
  }
  // 判断是否有 ALT 修饰符并添加到字符串流。Check for ALT modifier and add it to the stringstream.
  if (key_modifiers && KeyModifiers::ALT) {
    ss.str().empty() ? ss << "ALT" : ss << " ALT";
  }
  // 返回转换后的字符串。Return the converted string.
  return ss.str();
}

/**
 * \brief 删除键按下回调。Delete key press callback.
 * \param[in] handle 回调句柄。Callback handle.
 */
KEYBOARD_HANDLER_PUBLIC
void KeyboardHandlerBase::delete_key_press_callback(const callback_handle_t & handle) noexcept
{
  std::lock_guard<std::mutex> lk(callbacks_mutex_);
  // 遍历回调列表。Iterate through the callbacks list.
  for (auto it = callbacks_.begin(); it != callbacks_.end(); ++it) {
    // 如果找到匹配的句柄，删除对应的回调。If the matching handle is found, delete the corresponding callback.
    if (it->second.handle == handle) {
      callbacks_.erase(it);
      return;
    }
  }
}

/**
 * \brief 获取新的回调句柄。Get a new callback handle.
 * \return 新的回调句柄。New callback handle.
 */
KeyboardHandlerBase::callback_handle_t KeyboardHandlerBase::get_new_handle()
{
  static std::atomic<callback_handle_t> handle_count{0};
  // 原子地增加句柄计数并返回新的句柄。Atomically increment the handle count and return the new handle.
  return handle_count.fetch_add(1, std::memory_order_relaxed) + 1;
}
