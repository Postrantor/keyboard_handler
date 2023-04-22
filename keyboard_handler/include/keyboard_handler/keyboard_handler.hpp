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

#ifndef KEYBOARD_HANDLER__KEYBOARD_HANDLER_HPP_
#define KEYBOARD_HANDLER__KEYBOARD_HANDLER_HPP_

#ifdef _WIN32
#include "keyboard_handler_windows_impl.hpp" // 引入 Windows 平台的键盘处理实现 (Include the keyboard handler implementation for Windows platform)
/// \brief 为特定实现的键盘处理程序创建别名 (Create an alias for the implementation-specific keyboard handler)
/// \details 使用 KeyboardHandlerWindowsImpl 作为当前平台下的键盘处理程序 (Use KeyboardHandlerWindowsImpl as the keyboard handler for the current platform)
using KeyboardHandler = KeyboardHandlerWindowsImpl;
#else
#include "keyboard_handler_unix_impl.hpp" // 引入 Unix 平台的键盘处理实现 (Include the keyboard handler implementation for Unix platform)
/// \brief 为特定实现的键盘处理程序创建别名 (Create an alias for the implementation-specific keyboard handler)
/// \details 使用 KeyboardHandlerUnixImpl 作为当前平台下的键盘处理程序 (Use KeyboardHandlerUnixImpl as the keyboard handler for the current platform)
using KeyboardHandler = KeyboardHandlerUnixImpl;
#endif                                    // #ifdef _WIN32

#endif // KEYBOARD_HANDLER__KEYBOARD_HANDLER_HPP_
