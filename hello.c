int main() {
  int a = 8192;
  if (a == 8192)
    if (a > 2)
      if (a < 3) {
        a = 1024;
        return 8192;
      }
      else
        if (a > 4)
          if (a < 5)
            return a + 1;
          else
            return a + 2;
  return -1;
}