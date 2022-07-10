#pragma once 

#include "graphics.hpp"

class Console {
  public:
    // 縦25行、横80文字
    static const int kRows = 25, kColumns = 80;
    Console(PixelWriter& fg_color, const PixelColor& bg_color);
    void PutString(const char* s);

  private:
    void Newline();
  
    PixelWriter& writer_;
    const PixelColor fg_color_, bg_color_;
    char buffer_[kRows][kColumns + 1]; // 改行文字を含めるため
    int cursor_row_, cursor_column_;
};
