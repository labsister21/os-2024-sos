typedef int uint32_t;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
  __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
  __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
  __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
  __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
  __asm__ volatile("int $0x30");
}

int main(void) {
  char n = '_';
  syscall(0x4, (uint32_t)&n, 0, 0);

  char *c = "Hello, .!";
  c[7] = n;
  for (int i = 0; i < 9; ++i) {
    syscall(0x5, (uint32_t) & (c[i]), 0xF, 0);
  }
  return 0;
}