#include "Alphabet.hpp"

constexpr char BASE_ALPHABET_SIZE = 150;

const char ALPHABET_SIZE = BASE_ALPHABET_SIZE + RESERVED_SYMBOLS.size();

char get_position(char c) { return (c) + RESERVED_SYMBOLS.size(); }
