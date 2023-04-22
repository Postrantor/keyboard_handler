---
translate by baidu@2023-04-22 17:14:19
...

# Keyboard handler

Package providing ability to handle keyboard input via simple interface with callbacks.

> 包提供了通过带有回调的简单界面处理键盘输入的能力。

## Goal

We need to be able to handle keyboard input in unified way with cross-platform implementation.

> 我们需要能够以跨平台实现的统一方式处理键盘输入。

## Design requirements:

- Subscribe to keyboard events via callbacks

> -通过回调订阅键盘事件

- Subscriptions work with key modifiers (e.g. Shift+F1)

> -订阅使用键修饰符（例如 Shift+F1）

- Multiple clients can subscribe to the same event

> -多个客户端可以订阅同一事件

## Design Proposal

The following pseudocode suggests the high level API for aforementioned design proposal.

> 以下伪代码为上述设计方案提供了高级 API。

```cpp
    ClientClass1 client1;
    ClientClass2 client2;
    KeyboardHandler keyboard_handler;
    client1.register_key_press_callbacks(keyboard_handler);
    client2.register_callbacks_for_keyboard_handler(keyboard_handler);
```

Inside client class registering for the callbacks could be organized as simple as:

> 在客户端类内部注册回调可以通过以下方式进行组织：

```cpp
    ClientClass1::register_key_press_callbacks(KeyboardHandler & keyboard_handler)
    {
      keyboard_handler.add_key_press_callback(callback_fn, KeyboardHandler::KeyCode::CURSOR_UP);
      keyboard_handler.add_key_press_callback(callback_fn,
                                              KeyboardHandler::KeyCode::A,
                                              KeyboardHandler::KeyModifiers::CTRL);
    }
```

To be able to handle multiple events in one callback function and have ability to distinguish them inside, this callback function should have input parameter indicating which key combination handling in the current call. For instance it could be implemented as simple as:

> 为了能够在一个回调函数中处理多个事件，并能够在内部区分它们，这个回调函数应该有一个输入参数，指示当前调用中处理的键组合。例如，它可以简单地实现为：

```cpp
    void callback_function(KeyboardHandler::KeyCode key_code,
                           KeyboardHandler::KeyModifiers key_modifiers)
    {
      using KeyCode = KeyboardHandler::KeyCode;
      switch (key_code) {
        case KeyCode::CURSOR_UP:
          std::cout << "callback with key code = CURSOR_UP" << std::endl;
        break;
        case KeyCode::A:
          if (key_modifiers && KeyboardHandler::KeyModifiers::CTRL) {
            std::cout << "callback for pressed key combination = CTRL + a" << std::endl;
          }
        break;
        default:
         std::cout << "callback with key = '" << enum_key_code_to_str(key_code) << "' ";
         std::cout << "and key modifiers = " << enum_key_modifiers_to_str(key_modifiers) << "\n";
        break;
      }
    }
```

## Consideration of using C++ versus Python for cross-platform implementation

At the very early design discussions was proposed to use Python as cross-platform implementation for keyboard handling. From the first glance it looks attractive to use Python since it's relatively new language with reach built-in utility functions available on multiple platforms and OSes. However after some research and consideration we come to the conclusion that using Python is not better than C or C++ for this case. And here is the rational for that:

> 在早期的设计讨论中，有人提议使用 Python 作为键盘处理的跨平台实现。乍一看，使用 Python 似乎很有吸引力，因为它是一种相对较新的语言，具有可在多个平台和操作系统上使用的内置实用程序函数。然而，经过一些研究和考虑，我们得出的结论是，在这种情况下，使用 Python 并不比 C 或 C++好。这是合理的：

1.  It turns out that there are no built-in and cross-platform utility functions for handling input from keyboard in Python. All what you can find is a third party libraries with similar compile time division for Windows and Unix platforms. Most of the libraries not mature enough and have low quality.

> 1.事实证明，Python 中没有用于处理键盘输入的内置和跨平台实用程序功能。您所能找到的只是针对 Windows 和 Unix 平台具有类似编译时间划分的第三方库。大多数图书馆还不够成熟，质量也很低。

2.  Most of the ROS2 code written in C++ and it will be much easy to support and use solution written on the same language.

> 2.大多数 ROS2 代码都是用 C++编写的，支持和使用用同一语言编写的解决方案会容易得多。

3.  Some of the POSIX compatible OSes doesn't have Python interpreter. For instance [QNX for safety](https://blackberry.qnx.com/en/software-solutions/embedded-software/qnx-os-for-safety)

> 3.一些与 POSIX 兼容的操作系统没有 Python 解释器。例如[QNX 表示安全](https://blackberry.qnx.com/en/software-solutions/embedded-software/qnx-os-for-safety)

## Specific of handling input from keyboard on different platforms

Processing of the keyboard handling is differ for different operating systems. There are two major different approaches:

> 对于不同的操作系统，键盘处理的处理是不同的。有两种主要的不同方法：

1.  Unix/MacOS aka POSIX "compatible" OSes

> 1.Unix/MacOS，又名 POSIX“兼容”操作系统

2.  Windows family OSes

> 2.Windows 系列操作系统

POSIX compatible OSes approach based on readout from standard input linked to the terminal. Pressed key combination represented as sequence of 8 bit values. Number of the 8 bit values in sequence could vary from pressed key combination and usually in range of 1-8 chars. Note: There are no '\\0' value representing null terminator in providing sequence of bytes. i.e. those byte sequence could be treated as "C" strings if add null terminator at the end.

> 基于从连接到终端的标准输入读取的 POSIX 兼容操作系统方法。按下的组合键表示为 8 位值的序列。按顺序排列的 8 位值的数量可能因按键组合而异，通常在 1-8 个字符的范围内。注意：在提供字节序列时，没有表示 null 终止符的“\\0”值。即，如果在末尾添加 null 终止符，则这些字节序列可以被视为“C”字符串。

In Windows family OSes available mechanism for keyboard handling derived from DOS which is based on polling "`int kbhit()`" system function call until it will return non zero value indicating that some key was pressed. After that need to read integer value(s) from the standard input to determine which key was pressed. Pressed key combinations could be represented as one or two integer values. Simple alphanumerical keys represented as single integer. Control keys and specific function keys represented as two integer values. Note: for some key combinations first integer value could contain '\\0' value representing null terminator in regular strings. i.e it is not possible to treat such sequence of bytes as regular strings.

> 在 Windows 系列操作系统中，可用于键盘处理的机制源自 DOS，该机制基于轮询“int kbhit（）”系统函数调用，直到它返回非零值，指示某个键被按下。之后，需要从标准输入中读取整数值，以确定按下了哪个键。按下的键组合可以表示为一个或两个整数值。用单个整数表示的简单字母数字键。控制键和特定功能键表示为两个整数值。注意：对于某些键组合，第一个整数值可以包含“\\0”值，该值表示常规字符串中的 null 终止符。即不可能将这样的字节序列视为规则字符串。

### Specific of handling input from keyboard on POSIX compatible platforms

POSIX systems support two basic modes of input: canonical and non-canonical. In canonical input processing mode, terminal input is processed in lines terminated by newline '`\n`{=tex}', EOF, or EOL characters. No input can be read until an entire line has been typed by the user. Most programs use canonical input mode, because this gives the user a way to edit input line by line and this is what many of users accustom to see when typing commands in terminal window.\

> POSIX 系统支持两种基本的输入模式：规范和非规范。在规范输入处理模式中，终端输入以换行符“`\n`｛=tex｝”、EOF 或 EOL 字符结尾的行进行处理。在用户键入整行之前，无法读取任何输入。大多数程序使用规范输入模式，因为这为用户提供了一种逐行编辑输入的方式，而这正是许多用户在终端窗口中键入命令时习惯看到的\

However to be able to handle single key press, terminal input need to be switched to the noncanonical mode. By design keyboard handler will switch current terminal session to the noncanonical mode during construction and return it to the canonical mode in destructor.

> 然而，为了能够处理单按键按下，终端输入需要切换到非经典模式。根据设计，键盘处理程序将在构造过程中将当前终端会话切换到非规范模式，并在析构函数中将其返回到规范模式。

## Handling abnormal program termination via Ctrl+C

By design keyboard handler not providing ability to transfer `Ctrl+C` key press event to its clients via callbacks. It could be considered as current design limitation.\

> 根据设计，键盘处理程序不提供通过回调将“Ctrl+C”按键事件传输到其客户端的能力。这可以被认为是当前的设计限制\

On POSIX compatible platforms when user pressing `Ctrl+C` OS generates specific `SIGINT` signal and if this signal became unhandled current process will be terminated.\

> 在兼容 POSIX 的平台上，当用户按下“Ctrl+C”时，操作系统会生成特定的“SIGINT”信号，如果该信号未经处理，则当前进程将终止\

On POSIX compatible platforms keyboard handler will register it's own signal handler for the `SIGINT` to be able to return terminal input in canonical mode after abnormal program termination via pressing `Ctrl+C`. This signal handling could overlap with registered specific `SIGINT` handler on upper level or inside keyboard handler's clients. It could cause side effects and considering as current design and implementation limitations.

> 在兼容 POSIX 的平台上，键盘处理程序将为“SIGINT”注册自己的信号处理程序，以便能够在异常程序终止后通过按下“Ctrl+C”以规范模式返回终端输入。这种信号处理可能与上层或键盘处理程序客户端内部注册的特定“SIGINT”处理程序重叠。它可能会造成副作用，并被视为当前的设计和实施限制。

## Handling cases when client's code subscribed to the key press event got destructed before keyboard handler

There are two options to properly handle this case:

> 有两种选择可以正确处理这种情况：

1.  Keep callback handle returning from `KeyboardHandler::add_key_press_callback(..)` and use it in client destructor to delete callback via explicit call to the `KeyboardHandler::delete_key_press_callback(const callback_handle_t & handle)`

> 1.保留从`KeyboardHandler:：add_key_press_callback（..）`返回的回调句柄，并在客户端析构函数中使用它，通过显式调用`Keyboardinder:：delete_key_press _callback删除回调（const callback_handle_t&handle）`

2.  Use lambda with `weak_ptr` to the client instance as callback. This approach have assumption that client will be instantiated as shared pointer and this shared pointer will be available during callback registration.\

> 2.对客户端实例使用带有“weak_ptr”的 lambda 作为回调。这种方法假设客户端将被实例化为共享指针，并且该共享指针将在回调注册期间可用\

    Here are two different approaches for this option:

    1.  Using factory design pattern to create instance of client:

        ```cpp
        KeyboardHandler keyboard_handler;
        std::shared_ptr<Recorder> recorder = Recorder::create();
        recorder->register_callbacks(keyboard_handler);
        ```

        where `Recorder` class could be defined as:

        ```cpp
        class Recorder
        {
        public:
          const Recorder & operator=(const Recorder &) = delete;
          Recorder(const Recorder &) = delete;

          static std::shared_ptr<Recorder> create()
          {
            auto recorder_shared_ptr = std::shared_ptr<Recorder>(new Recorder);
            recorder_shared_ptr->weak_self_ = recorder_shared_ptr;
            return recorder_shared_ptr;
          }

          void register_callbacks(KeyboardHandler & keyboard_handler)
          {
            auto callback = [recorder_weak_ptr = weak_self_](KeyboardHandler::KeyCode key_code,
                KeyboardHandler::KeyModifiers key_modifiers) {
              auto recorder_shared_ptr = recorder_weak_ptr.lock();
              if (recorder_shared_ptr) {
                recorder_shared_ptr->callback_func(key_code, key_modifiers);
              } else {
                std::cout << "Object for assigned callback was deleted" << std::endl;
              }
            };
            keyboard_handler.add_key_press_callback(callback, KeyboardHandler::KeyCode::CURSOR_UP);
          }

        private:
          std::weak_ptr<Recorder> weak_self_;
          Recorder();
          void callback_func(KeyboardHandler::KeyCode key_code,
                             KeyboardHandler::KeyModifiers key_modifiers);
        }
        ```

    2.  Using [`shared_from_this()`](https://en.cppreference.com/w/cpp/memory/enable_shared_from_this/enable_shared_from_this). Note in this case client class should be inherited from [`std::enable_shared_from_this`](https://en.cppreference.com/w/cpp/memory/enable_shared_from_this/enable_shared_from_this) e.g.

        ```cpp
        class Player : public std::enable_shared_from_this<Player>
        {
        public:
          Player();
          void register_callbacks(KeyboardHandler & keyboard_handler)
          {
            std::weak_ptr<Player> player_weak_ptr(shared_from_this());
            auto callback = [player_weak_ptr](KeyboardHandler::KeyCode key_code,
                                              KeyboardHandler::KeyModifiers key_modifiers) {
              auto player_shared_ptr = player_weak_ptr.lock();
              if (player_shared_ptr) {
                player_shared_ptr->callback_func(key_code, key_modifiers);
              } else {
                std::cout << "Object for assigned callback was deleted" << std::endl;
              }
            };
            keyboard_handler.add_key_press_callback(callback, KeyboardHandler::KeyCode::CURSOR_UP);
          }
        ```

        `Player` class could be instantiated as `std::shared_ptr<Player> player_shared_ptr(new Player());`

## Handling cases when standard input from terminal or console redirected to the file or stream

By design keyboard handler rely on the assumption that it will poll on keypress event and then readout pressed keys from standard input. When standard input redirected to be read from the file or stream, keypress even will not happened and logic inside keyboard handler will be ill-formed and could lead to the deadlock. To avoid this scenario keyboard handler perform check in constructor if current terminal or console input bind to the real HW and was not redirected to be read from file or stream.

> 根据设计，键盘处理程序依赖于这样一种假设，即它将轮询按键事件，然后从标准输入中读出按下的按键。当标准输入被重定向为从文件或流中读取时，甚至不会发生按键，键盘处理程序内部的逻辑也会不正确，并可能导致死锁。为了避免这种情况，如果当前终端或控制台输入绑定到真正的硬件，并且没有重定向到从文件或流中读取，则键盘处理程序执行签入构造函数。

Note. Code executing under the `gtest` framework using redirection of the standard input.

From first glance it would be obvious if keyboard handler would throw exception if detected that standard input doesn't bind to the real HW. However it will require special handling inside client code which will use it. Especially when it will be running under the gtest. It would be ok if keyboard handler creating after it's clients. But in cases if for some reason keyboard handler should be created before it's clients, whole code which is depend on it is going to be skipped during unit test. For instance:

> 乍一看，如果检测到标准输入并没有绑定到真正的硬件，键盘处理程序是否会抛出异常是显而易见的。然而，它需要在使用它的客户端代码内部进行特殊处理。尤其是当它将在 gtest 下运行时。如果键盘处理程序在客户端之后创建，那就没问题了。但在某些情况下，如果出于某种原因，键盘处理程序应该在它的客户端之前创建，那么在单元测试期间，依赖于它的整个代码将被跳过。例如：

```cpp
    try {
      KeyboardHandler keyboard_handler;
      // .. Some other complicated logic maybe instantiating client in a separate thread
      std::shared_ptr<Client> client = Client::create();
      client->register_callbacks(keyboard_handler);
      // Use client to test it's internal functionality
    catch (...) {
      // Handle exceptions
    }
```

As you can see it will be impossible to separate exception handling for the case if keyboard handler will throw exception when it's running under the `gtest`.

> 正如您所看到的，如果键盘处理程序在“gtest”下运行时会抛出异常，那么就不可能将异常处理分离出来。

To be able smoothly use keyboard handler and it's dependent clients code under the `gtest` it was chosen design approach when keyboard handler not throwing exception on construction if standard input doesn't bind to the real HW. Instead of throwing exception keyboard handler will go in to the special safe state where he will aware about this situation and keyboard handling will be disabled.

> 为了能够在“gtest”下顺利使用键盘处理程序及其依赖的客户端代码，选择了当键盘处理程序在标准输入未绑定到实际硬件的情况下不抛出构造异常时的设计方法。键盘处理程序不会抛出异常，而是进入特殊的安全状态，在这种状态下他会意识到这种情况，并且键盘处理将被禁用。

Note: By design `KeyboardHandler::add_key_press_callback(..)` API call will return `false` in this case to indicate that handling keypress is not possible in this case.

## Known issues

Due to the current design and implementation limitations keyboard handler has following known issues:

> 由于当前的设计和实现限制，键盘处理程序存在以下已知问题：

- Some keys might be incorrectly detected with multiple key modifiers pressed at the same time.

> -在同时按下多个键修饰符的情况下，可能会错误地检测到某些键。

- Keyboard handler not able to correctly detect `CTRL` + `0..9` number keys.

> -键盘处理程序无法正确检测`CTRL`+`0..9`数字键。

- Instead of `CTRL` + `SHIFT` + `key` will be detected only `CTRL` + `key`.

> -将只检测到`CTRL`+`key`，而不是`CTRL`+`SHIFT`+`key`。

- Unix(POSIX) implementation can't correctly detect `CTRL`, `ALT`, `SHIFT` modifiers with `F1..F12` and other control keys.

> -Unix（POSIX）实现无法正确检测带有“F1.F12”和其他控制键的“CTRL”、“ALT”、“SHIFT”修饰符。

- Windows implementation not able to detect `CTRL` + `ALT` + `key` combinations.

> -Windows 实现无法检测`CTRL`+`ALT`+`key`组合。

- Windows implementation not able to detect `ALT` + `F1..12` keys.

> -Windows 实现无法检测`ALT`+`F1..12`键。
