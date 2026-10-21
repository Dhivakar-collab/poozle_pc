#!/usr/bin/env bash
set -e

CLANG_FORMAT_CMD=""

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
    echo "Checking for LLVM 16 on Windows..."
    
    # Path to standalone LLVM 16 binary
    LLVM16_PATH="/c/Program Files/LLVM/bin/clang-format.exe"

    if [ -f "$LLVM16_PATH" ]; then
        CLANG_FORMAT_CMD="$LLVM16_PATH"
    else
        echo "Error: LLVM 16 not found at '$LLVM16_PATH'."
        echo "Please install LLVM 16 via winget by running:"
        echo "  winget install LLVM.LLVM --version 16.0.6"
        exit 1
    fi
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

echo "Using $($CLANG_FORMAT_CMD --version)"

echo "Running clang-format-16 across repository..."
find . -type f \( -iname "*.cpp" -o -iname "*.hpp" -o -iname "*.c" -o -iname "*.h" \) \
    -exec "$CLANG_FORMAT_CMD" -i {} +

echo "Formatting complete!"