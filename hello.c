int f1(int a0[], int a1[][2]) {
  a0[0] = 1;
  a1[0][1] = a0[0] + 1;
  a1[1][0] = a1[0][1] + 1;
  return 0;
}

int main() {
  int a0[3];
  int a1[3][2];
  f1(a0, a1);
  putint(a0[0]);
  putch(10);
  putint(a1[0][1]);
  putch(10);
}
