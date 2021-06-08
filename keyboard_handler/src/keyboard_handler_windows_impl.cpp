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
#include <stdio.h>
#include <conio.h>
#include <io.h>
#include <iostream>
#include "keyboard_handler/keyboard_handler_windows_impl.hpp"

KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerBase::KeyCode
KeyboardHandlerWindowsImpl::win_key_code_to_enum(const WinKeyCode & win_key_code) const
{
  auto key_map_it = key_codes_map_.find(win_key_code);
  if (key_map_it != key_codes_map_.end()) {
    return key_map_it->second;
  }
  return KeyboardHandlerBase::KeyCode::UNKNOWN;
}

KEYBOARD_HANDLER_PUBLIC
KeyboardHandlerWindowsImpl::WinKeyCode
KeyboardHandlerWindowsImpl::enum_key_code_to_win_code(KeyboardHandlerBase::KeyCode key_code) const
{
  for (const auto & it : key_codes_map_) {
    if (it.second == key_code) {
      return it.first;
    }
  }
  return {WinKeyCode::NOT_A_KEY, WinKeyCode::NOT_A_KEY};
}

KeyboardHandlerWindowsImpl::KeyboardHandlerWindowsImpl()
: exit_(false)
{
  for (size_t i = 0; i < STATIC_KEY_MAP_LENGTH; i++) {
    key_codes_map_.emplace(
      DEFAULT_STATIC_KEY_MAP[i].win_key_code,
      DEFAULT_STATIC_KEY_MAP[i].inner_code);
  }

  // Check if we can handle key press from std input
  if (!_isatty(_fileno(stdin))) {
    // If stdin is not a real terminal or console (redirected to file or pipe ) can't do much here
    // with keyboard handling.
    std::cerr << "stdin is not a terminal or console device. Keyboard handling disabled.";
    return;
  }

  is_init_succeed_ = true;

  key_handler_thread_ = std::thread(
    [&]() {
      do {
        if (_kbhit()) {
          WinKeyCode win_key_code{WinKeyCode::NOT_A_KEY, WinKeyCode::NOT_A_KEY};
          int ch = _getch();
          win_key_code.first = ch;
          // When reading a function key or an arrow key, each function must be called twice;
          // the first call returns 0 or 0xE0, and the second call returns the actual key code.
          // https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2012/078sfkak(v=vs.110)
          if (ch == 0 || ch == 0xE0) {  // 0xE0 == 224
            // ch == 0 for F1 - F10 keys, ch == 0xE0 for all other control keys.
            ch = _getch();
            win_key_code.second = ch;
          }

          KeyCode pressed_key_code = win_key_code_to_enum(win_key_code);

          std::lock_guard<std::mutex> lk(callbacks_mutex_);
          auto range = callbacks_.equal_range(pressed_key_code);
          for (auto it = range.first; it != range.second; ++it) {
            it->second(it->first);
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      } while (!exit_.load());
    });
}

KeyboardHandlerWindowsImpl::~KeyboardHandlerWindowsImpl()
{
  exit_ = true;
  if (key_handler_thread_.joinable()) {
    key_handler_thread_.join();
  }
}
#endif  // #ifdef _WIN32
