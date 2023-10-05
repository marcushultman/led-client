#include "font/font.h"

int main() {
  std::string input;

  auto page = font::TextPage::create();
  page->setText("FOO");
  assert(page->sprites().size() == 3);

  return 0;
}
