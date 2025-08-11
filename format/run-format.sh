#!/usr/bin/env bash
set -e

# --- 1. Check if clang-format-16 exists ---
CLANG_FORMAT_CMD="clang-format-16"

if ! command -v "$CLANG_FORMAT_CMD" &>/dev/null; then
    echo "clang-format-16 not found. Installing..."

    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Ubuntu/Debian
        sudo apt update
        sudo apt install -y clang-format-16
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        if ! command -v brew &>/dev/null; then
            echo "Homebrew not found. Please install Homebrew first."
            exit 1
        fi
        brew install llvm@16

        # Get brew's llvm@16 bin directory
        LLVM_PATH="$(brew --prefix llvm@16)/bin/clang-format"
        if [[ -x "$LLVM_PATH" ]]; then
            CLANG_FORMAT_CMD="$LLVM_PATH"
        else
            echo "Failed to find clang-format in llvm@16 bin directory."
            exit 1
        fi
    else
        echo "Unsupported OS. Please install clang-format-16 manually."
        exit 1
    fi
else
    echo "clang-format-16 is already installed."
fi

# --- 2. Run clang-format on all .cpp, .hpp, .c, .h files ---
echo "Running $CLANG_FORMAT_CMD on source files..."
find . -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" \) \
    -exec "$CLANG_FORMAT_CMD" -i {} +

echo "Formatting complete!"
