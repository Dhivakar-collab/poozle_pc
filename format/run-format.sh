#!/usr/bin/env bash
set -e

CLANG_FORMAT_CMD=""

# 1. First, check if clang-format or clang-format-16 is already available in PATH
if command -v clang-format &>/dev/null; then
    CLANG_FORMAT_CMD="clang-format"
elif command -v clang-format-16 &>/dev/null; then
    CLANG_FORMAT_CMD="clang-format-16"
fi

# 2. If not found, run OS-specific fallback installation
if [ -z "$CLANG_FORMAT_CMD" ]; then
    install_clang_format_linux() {
        echo "Installing clang-format-16 on Linux..."
        sudo apt update
        sudo apt install -y clang-format-16
        CLANG_FORMAT_CMD="clang-format-16"
    }

    install_clang_format_macos() {
        echo "Installing clang-format-16 on macOS..."
        if ! command -v brew &>/dev/null; then
            echo "Homebrew not found. Please install Homebrew first."
            exit 1
        fi
        brew install llvm@16
        CLANG_FORMAT_CMD="$(brew --prefix llvm@16)/bin/clang-format"
    }

    install_clang_format_windows() {
        echo "Installing clang-format on Windows (MSYS2/MinGW64)..."
        if ! command -v pacman &>/dev/null; then
            echo "MSYS2 pacman not found. Please install MSYS2 from https://www.msys2.org/"
            exit 1
        fi
        pacman -S --needed --noconfirm mingw-w64-x86_64-clang-tools-extra
        CLANG_FORMAT_CMD="clang-format"
    }

    case "$OSTYPE" in
        linux-gnu*)   install_clang_format_linux ;;
        darwin*)      install_clang_format_macos ;;
        msys*|cygwin*|win32) install_clang_format_windows ;;
        *)
            echo "Unsupported OS: $OSTYPE"
            exit 1
            ;;
    esac
fi

echo "Using $($CLANG_FORMAT_CMD --version)"

echo "Running $CLANG_FORMAT_CMD on source files..."
find . -type f \( -iname "*.cpp" -o -iname "*.hpp" -o -iname "*.c" -o -iname "*.h" \) \
    -exec "$CLANG_FORMAT_CMD" -i {} +

echo "Formatting complete!"