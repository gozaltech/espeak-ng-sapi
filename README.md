# eSpeak-NG SAPI5 Wrapper

A native Windows SAPI5 wrapper for the eSpeak-NG text-to-speech engine, enabling eSpeak-NG voices to work with any SAPI5-compatible application including screen readers (NVDA, JAWS, Windows Narrator) and book reader applications (Balabolka, Bookworm).

> **Note:** This is early-stage software under active development. Some bugs and issues may be expected. Please report any problems you encounter!

## Features

- **Native Multi-Architecture Support** - Full x86 and x64 support on Windows
- **100+ Languages and Voices** - Access to all eSpeak-NG voices and language variants
- **SAPI5 Integration** - Works seamlessly with any SAPI5-compatible Windows application
- **GUI Configuration Tool** - Easy-to-use interface for voice customization
- **Configurable Voice Profiles** - Customize rate, pitch, volume, intonation, and word gap per voice

## Download

Download the latest release from the [Releases](../../releases) page.

## Building from Source

```batch
build_all.bat
```

**Requirements:**
- Windows 10 or later
- Visual Studio 2022+ or LLVM Clang-CL
- CMake 4.0+
- Ninja build system

## Contributing

Contributions are welcome! Here's how you can help:

### Reporting Issues

Found a bug or have a feature request? Please [open an issue](../../issues) with:
- Clear description of the problem or feature
- Steps to reproduce (for bugs)
- Your Windows version and architecture (x86/x64)
- Relevant logs if available

### Submitting Pull Requests

1. Fork the repository
2. Create a feature branch (`git checkout -b feature-name`)
3. Make your changes
4. Test on both x86 and x64 architectures
5. Commit your changes
6. Push to your branch
7. [Create a Pull Request](../../pulls)

Please ensure your code follows the existing style.

## Support the Project

If you find this project helpful, consider supporting its development:

[![Donate with PayPal](https://img.shields.io/badge/Donate-PayPal-blue.svg)](https://paypal.me/gozaltech)

Your support helps maintain and improve this project!

## License

This SAPI5 wrapper is open source software.
eSpeak-NG is licensed under GPL v3.

## Credits

- **eSpeak-NG Project:** https://github.com/espeak-ng/espeak-ng
- **Original eSpeak:** Jonathan Duddington
