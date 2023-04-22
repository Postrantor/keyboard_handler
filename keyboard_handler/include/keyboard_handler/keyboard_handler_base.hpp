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

#ifndef KEYBOARD_HANDLER__KEYBOARD_HANDLER_BASE_HPP_
#define KEYBOARD_HANDLER__KEYBOARD_HANDLER_BASE_HPP_

#include "keyboard_handler/visibility_control.hpp"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

// #define PRINT_DEBUG_INFO

/// @class KeyboardHandlerBase
/// @brief 键盘处理器基类，提供处理键盘输入的基本功能
/// @brief Keyboard handler base class, providing basic functionality for handling keyboard input
class KeyboardHandlerBase
{
public:
  /// @enum KeyCode
  /// @brief 可能的按键组合枚举，键盘处理器可以处理这些组合
  /// @brief Enum for possible keys press combinations which keyboard handler is capable to handle.
  enum class KeyCode : uint32_t;

  /// \brief 键盘修饰符枚举，如CTRL、ALT和SHIFT与基础按键一起按下时使用。
  /// \details 以位掩码形式表示的枚举，可以包含多个值。用重载的`|`逻辑或运算符设置多个值，并用`&&`运算符提取。
  /// Enum for key modifiers such as CTRL, ALT and SHIFT pressed along side with base key.
  /// Enum represented as a bitmask and could contain multiple values. Multiple values
  /// can be settled up with overloaded `|` logical OR operator and extracted with `&&` operator.
  enum class KeyModifiers : uint32_t { NONE = 0, SHIFT = 1, ALT = 1 << 1, CTRL = 1 << 2 };

  /// \brief 回调函数类型定义
  /// Type for callback functions
  using callback_t = std::function<void(KeyCode, KeyModifiers)>;
  using callback_handle_t = uint64_t;

  /// \brief 从add_key_press_callback返回的回调句柄，并作为delete_key_press_callback的参数
  /// Callback handle returning from add_key_press_callback and using as an argument for
  /// the delete_key_press_callback
  KEYBOARD_HANDLER_PUBLIC
  static constexpr callback_handle_t invalid_handle = 0;

  /// \brief 添加可调用对象作为指定按键组合的处理程序。
  /// \brief Adding callable object as a handler for specified key press combination.
  /// \param callback 当键码被识别时将被调用的可调用对象。
  /// \param callback Callable which will be called when key_code will be recognized.
  /// \param key_code 来自枚举的值，对应于某个预定义的按键组合。
  /// \param key_code Value from enum which corresponds to some predefined key press combination.
  /// \param key_modifiers 来自枚举的值，对应于与键一起按下的键修饰符。
  /// \param key_modifiers Value from enum which corresponds to the key modifiers pressed along side with key.
  /// \return 如果回调成功添加到键盘处理程序，则返回新创建的回调句柄；如果回调为空或键盘处理程序未成功初始化，则返回 invalid_handle。
  /// \return Return Newly created callback handle if callback was successfully added to the keyboard handler, returns invalid_handle if callback is nullptr or keyboard handler wasn't successfully initialized.
  KEYBOARD_HANDLER_PUBLIC
  callback_handle_t add_key_press_callback(
    const callback_t & callback,
    KeyboardHandlerBase::KeyCode key_code,
    KeyboardHandlerBase::KeyModifiers key_modifiers = KeyboardHandlerBase::KeyModifiers::NONE);

  /// \brief 从键盘处理程序回调列表中删除回调
  /// \brief Delete callback from keyboard handler callback's list
  /// \param handle 从 #add_key_press_callback 返回的回调句柄
  /// \param handle Callback's handle returned from #add_key_press_callback
  KEYBOARD_HANDLER_PUBLIC
  void delete_key_press_callback(const callback_handle_t & handle) noexcept;

protected:
  /**
 * @brief 回调数据结构体 (Callback data structure)
 */
  struct callback_data
  {
    callback_handle_t handle; ///< 句柄 (Handle)
    callback_t callback;      ///< 回调函数 (Callback function)
  };

  /**
 * @brief 键和修饰符结构体 (Key and modifiers structure)
 */
  struct KeyAndModifiers
  {
    KeyCode key_code;           ///< 键码 (Key code)
    KeyModifiers key_modifiers; ///< 键修饰符 (Key modifiers)

    /**
   * @brief 判断两个 KeyAndModifiers 是否相等 (Determine if two KeyAndModifiers are equal)
   *
   * @param rhs 另一个 KeyAndModifiers 对象 (Another KeyAndModifiers object)
   * @return 两个对象是否相等 (Whether the two objects are equal)
   */
    bool operator==(const KeyAndModifiers & rhs) const
    {
      return this->key_code == rhs.key_code && this->key_modifiers == rhs.key_modifiers;
    }

    /**
   * @brief 判断两个 KeyAndModifiers 是否不相等 (Determine if two KeyAndModifiers are not equal)
   *
   * @param rhs 另一个 KeyAndModifiers 对象 (Another KeyAndModifiers object)
   * @return 两个对象是否不相等 (Whether the two objects are not equal)
   */
    bool operator!=(const KeyAndModifiers & rhs) const { return !operator==(rhs); }
  };

  /**
 * @brief 用于 `unordered_map` 的 KeyAndModifiers 的特化哈希函数 (Specialized hash function for `unordered_map` with KeyAndModifiers)
 */
  struct key_and_modifiers_hash_fn
  {
    /**
   * @brief 计算 KeyAndModifiers 的哈希值 (Calculate the hash value of KeyAndModifiers)
   *
   * @param key_and_mod KeyAndModifiers 对象 (KeyAndModifiers object)
   * @return 哈希值 (Hash value)
   */
    std::size_t operator()(const KeyAndModifiers & key_and_mod) const
    {
      using key_undertype = std::underlying_type_t<KeyCode>;
      using mods_undertype = std::underlying_type_t<KeyModifiers>;
      return std::hash<mods_undertype>()(static_cast<mods_undertype>(key_and_mod.key_modifiers)) ^
             (std::hash<key_undertype>()(static_cast<key_undertype>(key_and_mod.key_code)) << 3);
    }
  };

  bool is_init_succeed_ =
    false; ///< 初始化是否成功的标志 (Flag indicating whether initialization was successful)
  std::mutex
    callbacks_mutex_; ///< 用于保护回调函数列表的互斥锁 (Mutex for protecting the callback function list)
  std::unordered_multimap<KeyAndModifiers, callback_data, key_and_modifiers_hash_fn>
    callbacks_; ///< 存储键和修饰符与回调数据之间关系的哈希表 (Hash table storing the relationship between keys and modifiers and callback data)

private:
  static callback_handle_t get_new_handle();
};

enum class KeyboardHandlerBase::KeyCode : uint32_t {
  UNKNOWN = 0,
  EXCLAMATION_MARK,
  QUOTATION_MARK,
  HASHTAG_SIGN,
  DOLLAR_SIGN,
  PERCENT_SIGN,
  AMPERSAND,
  APOSTROPHE,
  OPENING_PARENTHESIS,
  CLOSING_PARENTHESIS,
  STAR,
  PLUS,
  COMMA,
  MINUS,
  DOT,
  RIGHT_SLASH,
  NUMBER_0,
  NUMBER_1,
  NUMBER_2,
  NUMBER_3,
  NUMBER_4,
  NUMBER_5,
  NUMBER_6,
  NUMBER_7,
  NUMBER_8,
  NUMBER_9,
  COLON,
  SEMICOLON,
  LEFT_ANGLE_BRACKET,
  EQUAL_SIGN,
  RIGHT_ANGLE_BRACKET,
  QUESTION_MARK,
  AT,
  LEFT_SQUARE_BRACKET,
  BACK_SLASH,
  RIGHT_SQUARE_BRACKET,
  CARET,
  UNDERSCORE_SIGN,
  GRAVE_ACCENT_SIGN,
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  LEFT_CURLY_BRACKET,
  VERTICAL_BAR,
  RIGHT_CURLY_BRACKET,
  TILDA,
  CURSOR_UP,
  CURSOR_DOWN,
  CURSOR_LEFT,
  CURSOR_RIGHT,
  ESCAPE,
  SPACE,
  ENTER,
  BACK_SPACE,
  DELETE_KEY,
  END,
  PG_DOWN,
  PG_UP,
  HOME,
  INSERT,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  END_OF_KEY_CODE_ENUM
};

/// \brief 逻辑与运算符用于表示位掩码的 KeyModifiers 枚举。
/// \brief Logical AND operator for KeyModifiers enum represented as a bitmask.
/// \return 如果在一个操作数中的测试值出现在另一个操作数给定的位掩码中，则返回 true，否则返回 false。
/// \return true if testing value in one of the operands present in a bitmask given in another
/// operand, otherwise false.
KEYBOARD_HANDLER_PUBLIC
bool operator&&(
  const KeyboardHandlerBase::KeyModifiers & left, const KeyboardHandlerBase::KeyModifiers & right);

/// \brief 逻辑或运算符用于表示位掩码的 KeyModifiers 枚举。
/// \brief Logical OR operator for KeyModifiers enum represented as a bitmask.
/// \param left KeyModifiers 枚举位掩码
/// \param left KeyModifiers enum bitmask
/// \param right 要在位掩码中设置的修饰符值
/// \param right Modifier value to set in bitmask
/// \return 使用来自右侧参数的 settled bit 的新 KeyModifiers 位掩码值
/// \return new KeyModifiers bitmask value with settled bit from right side parameter
KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerBase::KeyModifiers
operator|(KeyboardHandlerBase::KeyModifiers left, const KeyboardHandlerBase::KeyModifiers & right);

/// \brief KeyCode 枚举值的前缀递增运算符
/// \brief Prefix increment operator for KeyCode enum values
KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerBase::KeyCode & operator++(KeyboardHandlerBase::KeyCode & key_code);

/// \brief 用于将 KeyCode 枚举值映射到其字符串表示形式的数据类型。
/// \brief Data type for mapping KeyCode enum value to it's string representation.
struct KeyCodeToStrMap
{
  // 内部 KeyCode 枚举值
  // Inner KeyCode enum value
  KeyboardHandlerBase::KeyCode inner_code;

  // 指向字符串的指针，表示 KeyCode 的字符串形式
  // Pointer to the string representing the KeyCode in string form
  const char * str;
};

/// \brief Lookup table for mapping KeyCode enum value to it's string representation.
static const KeyCodeToStrMap ENUM_KEY_TO_STR_MAP[]{
  {KeyboardHandlerBase::KeyCode::UNKNOWN, "UNKNOWN"},
  {KeyboardHandlerBase::KeyCode::EXCLAMATION_MARK, "!"},
  {KeyboardHandlerBase::KeyCode::QUOTATION_MARK, "QUOTATION_MARK"},
  {KeyboardHandlerBase::KeyCode::HASHTAG_SIGN, "#"},
  {KeyboardHandlerBase::KeyCode::DOLLAR_SIGN, "&"},
  {KeyboardHandlerBase::KeyCode::PERCENT_SIGN, "%"},
  {KeyboardHandlerBase::KeyCode::AMPERSAND, "&"},
  {KeyboardHandlerBase::KeyCode::APOSTROPHE, "'"},
  {KeyboardHandlerBase::KeyCode::OPENING_PARENTHESIS, "("},
  {KeyboardHandlerBase::KeyCode::CLOSING_PARENTHESIS, ")"},
  {KeyboardHandlerBase::KeyCode::STAR, "*"},
  {KeyboardHandlerBase::KeyCode::PLUS, "+"},
  {KeyboardHandlerBase::KeyCode::COMMA, ","},
  {KeyboardHandlerBase::KeyCode::DOT, "."},
  {KeyboardHandlerBase::KeyCode::RIGHT_SLASH, "/"},
  {KeyboardHandlerBase::KeyCode::NUMBER_1, "NUMBER_1"},
  {KeyboardHandlerBase::KeyCode::NUMBER_2, "NUMBER_2"},
  {KeyboardHandlerBase::KeyCode::NUMBER_3, "NUMBER_3"},
  {KeyboardHandlerBase::KeyCode::NUMBER_4, "NUMBER_4"},
  {KeyboardHandlerBase::KeyCode::NUMBER_5, "NUMBER_5"},
  {KeyboardHandlerBase::KeyCode::NUMBER_6, "NUMBER_6"},
  {KeyboardHandlerBase::KeyCode::NUMBER_7, "NUMBER_7"},
  {KeyboardHandlerBase::KeyCode::NUMBER_8, "NUMBER_8"},
  {KeyboardHandlerBase::KeyCode::NUMBER_9, "NUMBER_9"},
  {KeyboardHandlerBase::KeyCode::NUMBER_0, "NUMBER_0"},
  {KeyboardHandlerBase::KeyCode::MINUS, "MINUS"},
  {KeyboardHandlerBase::KeyCode::COLON, ":"},
  {KeyboardHandlerBase::KeyCode::SEMICOLON, ";"},
  {KeyboardHandlerBase::KeyCode::LEFT_ANGLE_BRACKET, "<"},
  {KeyboardHandlerBase::KeyCode::EQUAL_SIGN, "EQUAL_SIGN"},
  {KeyboardHandlerBase::KeyCode::RIGHT_ANGLE_BRACKET, ">"},
  {KeyboardHandlerBase::KeyCode::QUESTION_MARK, "?"},
  {KeyboardHandlerBase::KeyCode::AT, "@"},
  {KeyboardHandlerBase::KeyCode::A, "a"},
  {KeyboardHandlerBase::KeyCode::B, "b"},
  {KeyboardHandlerBase::KeyCode::C, "c"},
  {KeyboardHandlerBase::KeyCode::D, "d"},
  {KeyboardHandlerBase::KeyCode::E, "e"},
  {KeyboardHandlerBase::KeyCode::F, "f"},
  {KeyboardHandlerBase::KeyCode::G, "g"},
  {KeyboardHandlerBase::KeyCode::H, "h"},
  {KeyboardHandlerBase::KeyCode::I, "i"},
  {KeyboardHandlerBase::KeyCode::J, "j"},
  {KeyboardHandlerBase::KeyCode::K, "k"},
  {KeyboardHandlerBase::KeyCode::L, "l"},
  {KeyboardHandlerBase::KeyCode::M, "m"},
  {KeyboardHandlerBase::KeyCode::N, "n"},
  {KeyboardHandlerBase::KeyCode::O, "o"},
  {KeyboardHandlerBase::KeyCode::P, "p"},
  {KeyboardHandlerBase::KeyCode::Q, "q"},
  {KeyboardHandlerBase::KeyCode::R, "r"},
  {KeyboardHandlerBase::KeyCode::S, "s"},
  {KeyboardHandlerBase::KeyCode::T, "t"},
  {KeyboardHandlerBase::KeyCode::U, "u"},
  {KeyboardHandlerBase::KeyCode::V, "v"},
  {KeyboardHandlerBase::KeyCode::W, "w"},
  {KeyboardHandlerBase::KeyCode::X, "x"},
  {KeyboardHandlerBase::KeyCode::Y, "y"},
  {KeyboardHandlerBase::KeyCode::Z, "z"},
  {KeyboardHandlerBase::KeyCode::LEFT_SQUARE_BRACKET, "["},
  {KeyboardHandlerBase::KeyCode::BACK_SLASH, "BACK_SLASH"},
  {KeyboardHandlerBase::KeyCode::RIGHT_SQUARE_BRACKET, "]"},
  {KeyboardHandlerBase::KeyCode::CARET, "^"},
  {KeyboardHandlerBase::KeyCode::UNDERSCORE_SIGN, "_"},
  {KeyboardHandlerBase::KeyCode::GRAVE_ACCENT_SIGN, "`"},
  {KeyboardHandlerBase::KeyCode::LEFT_CURLY_BRACKET, "{"},
  {KeyboardHandlerBase::KeyCode::VERTICAL_BAR, "|"},
  {KeyboardHandlerBase::KeyCode::RIGHT_CURLY_BRACKET, "}"},
  {KeyboardHandlerBase::KeyCode::TILDA, "~"},
  {KeyboardHandlerBase::KeyCode::CURSOR_UP, "CURSOR_UP"},
  {KeyboardHandlerBase::KeyCode::CURSOR_DOWN, "CURSOR_DOWN"},
  {KeyboardHandlerBase::KeyCode::CURSOR_LEFT, "CURSOR_LEFT"},
  {KeyboardHandlerBase::KeyCode::CURSOR_RIGHT, "CURSOR_RIGHT"},
  {KeyboardHandlerBase::KeyCode::ESCAPE, "ESCAPE"},
  {KeyboardHandlerBase::KeyCode::SPACE, "SPACE"},
  {KeyboardHandlerBase::KeyCode::ENTER, "ENTER"},
  {KeyboardHandlerBase::KeyCode::BACK_SPACE, "BACK_SPACE"},
  {KeyboardHandlerBase::KeyCode::DELETE_KEY, "DELETE_KEY"},
  {KeyboardHandlerBase::KeyCode::END, "END"},
  {KeyboardHandlerBase::KeyCode::PG_DOWN, "PG_DOWN"},
  {KeyboardHandlerBase::KeyCode::PG_UP, "PG_UP"},
  {KeyboardHandlerBase::KeyCode::HOME, "HOME"},
  {KeyboardHandlerBase::KeyCode::INSERT, "INSERT"},
  {KeyboardHandlerBase::KeyCode::F1, "F1"},
  {KeyboardHandlerBase::KeyCode::F2, "F2"},
  {KeyboardHandlerBase::KeyCode::F3, "F3"},
  {KeyboardHandlerBase::KeyCode::F4, "F4"},
  {KeyboardHandlerBase::KeyCode::F5, "F5"},
  {KeyboardHandlerBase::KeyCode::F6, "F6"},
  {KeyboardHandlerBase::KeyCode::F7, "F7"},
  {KeyboardHandlerBase::KeyCode::F8, "F8"},
  {KeyboardHandlerBase::KeyCode::F9, "F9"},
  {KeyboardHandlerBase::KeyCode::F10, "F10"},
  {KeyboardHandlerBase::KeyCode::F11, "F11"},
  {KeyboardHandlerBase::KeyCode::F12, "F12"},
};

/// \brief 将 KeyCode 枚举值转换为其字符串表示形式。Translate KeyCode enum value to it's string representation.
/// \param key_code 来自枚举的值，对应于某个预定义的按键组合。Value from enum which corresponds to some predefined key press combination.
/// \return 在 ENUM_KEY_TO_STR_MAP 查找表中指定枚举值对应的字符串。String corresponding to the specified enum value in ENUM_KEY_TO_STR_MAP lookup table.
KEYBOARD_HANDLER_PUBLIC
std::string enum_key_code_to_str(KeyboardHandlerBase::KeyCode key_code);

/// \brief 将 str 值转换为其 keycode 表示形式。Translate str value to it's keycode representation.
/// \param String key_code_str 字符串表示的按键代码。String representation of a keycode.
/// \return KeyboardHandlerBase::Keycode 对应的按键代码枚举值。Corresponding keycode enum value.
KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerBase::KeyCode enum_str_to_key_code(const std::string & key_code_str);

/// \brief 将 KeyModifiers 枚举值转换为其字符串表示形式。Translate KeyModifiers enum value to it's string representation.
/// \param key_modifiers 按键修饰符位掩码。Bitmask with key modifiers.
/// \return 指定枚举值对应的字符串。String corresponding to the specified enum value.
KEYBOARD_HANDLER_PUBLIC
std::string enum_key_modifiers_to_str(KeyboardHandlerBase::KeyModifiers key_modifiers);

#endif // KEYBOARD_HANDLER__KEYBOARD_HANDLER_BASE_HPP_
