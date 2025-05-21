# CString Library 🚀

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Windows](https://img.shields.io/badge/Platform-Windows-0078d7.svg)](https://en.wikipedia.org/wiki/Microsoft_Windows)
[![C](https://img.shields.io/badge/Language-C-00599C.svg)](https://en.wikipedia.org/wiki/C_(programming_language))

Thread-safe dynamic string implementation for C with Windows API integration

## 📚 Table of Contents
- [Features](#-features)
- [Installation](#-installation)
- [Usage](#-usage)
- [Documentation](#-documentation)
- [Examples](#-examples)
- [Contributing](#-contributing)
- [License](#-license)

## 🌟 Features

- **Thread-safe** operations using CRITICAL_SECTION
- **Automatic memory management** with dynamic resizing
- **Multiple creation methods**:
  - From C strings
  - From wide character strings
  - From binary buffers
  - From other CString objects
- **Conversion utilities**:
  - Wide <-> Multibyte conversion
  - Upper/lower case conversion
  - Whitespace trimming
- **Advanced string operations**:
  - Tokenization with escape characters
  - Zone-aware parsing
  - Substring operations
- **Secure memory handling** with SecureZeroMemory
- **Cross-platform** Windows API implementation

## 📦 Installation

1. Clone the repository:
```bash
git clone https://github.com/yourusername/cstring.git
```
2. Include the header in your project:
```c
#include "includes/cstr.h"
```
3. Link with Windows libraries (automatic in most cases)

## 🛠 Usage

### Basic example
```c
CString str;
cstr_create_from_chars(&str, "Hello");
cstr_append_chars(&str, " World!");

printf("%s\n", cstr_data(&str)); // Hello World!

cstr_destroy(&str);
```

### Tokenization
```c
size_t pos = 0;
CString str, token;
cstr_create_from_chars(&str, "Hello, \"my 'world!'\"");

while (cstr_tokenize_ex(&str, &token, " ", "\"\"", "\\", &pos))
{
    printf("Token: %s\n", cstr_data(&token));
    cstr_destroy(&token);
}

cstr_destroy(&str);
```
Output:
```
Token: Hello, 
Token: "my 'world!'"
```

## Documentation

|Type|File|Location|
|----|----|--------|
|🌐 **HTML** | Interactive web-version | [docs/html/index.html](docs/html/index.html) |
|📄 **PDF** | Printable version | [docs/index.pdf](docs/index.pdf)
|📦 **XML** | Doxygen XML output | [docs/index.xml](docs/index.xml)
|📖 **DocBook** | DocBook source | [docs/docbook.xml](docs/docbook.xml)
|📝 **RTF** | Rich Text Format | [docs/index.rtf](docs/index.rtf)

## 🤝 Contributing

1. Fork the project
2. Create your feature branch:
```bash
git checkout -b feature/AmazingFeature
```
3. Commit your changes:
```bash
git commit -m 'Add some AmazingFeature'
```
4. Push to the branch:
```bash
git push origin feature/AmazingFeature
```
5. Open a Pull Request

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.
