// Performance-Aware-Programming Course
// https://www.computerenhance.com/p/table-of-contents
//
// Microsoft interview questions from 1994
// Question 2: Null terminated string copy

#include <stdio.h>

// Copy null terminated string from source to destination
void str_cpy(const char *src, char *dst) {
  if (!src || !dst) {
    return;
  }

  while ((*dst++ = *src++)) {
  }
}

int main(void) {
  {
    const char *src = "";
    char dst[] = "";

    printf("Before\nsrc: '%s'\ndst: '%s'\n", src, dst);
    str_cpy(src, dst);
    printf("After\nsrc: '%s'\ndst: '%s'\n\n", src, dst);
  }
  {
    const char *src = "";
    char dst[] = "a";

    printf("Before\nsrc: '%s'\ndst: '%s'\n", src, dst);
    str_cpy(src, dst);
    printf("After\nsrc: '%s'\ndst: '%s'\n\n", src, dst);
  }
  {
    const char *src = "asd";
    char dst[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    printf("Before\nsrc: '%s'\ndst: '%s'\n", src, dst);
    str_cpy(src, dst);
    printf("After\nsrc: '%s'\ndst: '%s'\n\n", src, dst);
  }
  {
    const char *src = "a";
    char dst[] = "b";

    printf("Before\nsrc: '%s'\ndst: '%s'\n", src, dst);
    str_cpy(src, dst);
    printf("After\nsrc: '%s'\ndst: '%s'\n\n", src, dst);
  }
  {
    const char *src = "0123456789ABCDEF0123456789ABCDEF";
    char dst[] = "XXXXXXXXXXXXXXX"; // X * 15

    printf("Before\nsrc: '%s'\ndst: '%s'\n", src, dst);
    str_cpy(src, dst);
    printf("After\nsrc: '%s'\ndst: '%s'\n\n", src, dst);
  }
  {
    const char *src = "0123456789ABCDEF0123456789ABCDEF";
    char dst[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"; // X * 31

    printf("Before\nsrc: '%s'\ndst: '%s'\n", src, dst);
    str_cpy(src, dst);
    printf("After\nsrc: '%s'\ndst: '%s'\n\n", src, dst);
  }
  {
    const char *src = "0123456789";
    char dst[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"; // X * 31

    printf("Before\nsrc: '%s'\ndst: '%s'\n", src, dst);
    str_cpy(src, dst);
    printf("After\nsrc: '%s'\ndst: '%s'\n\n", src, dst);
  }
  return 0;
}
