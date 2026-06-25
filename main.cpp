
#include "hardware/structs/scb.h"
#include <cstdio>
#include <cstdlib>
#include <hardware/exception.h>
#include <hardware/gpio.h>
#include <pico/stdio.h>
#include <pico/stdlib.h>
#include <shizu.hpp>
#include <time.h>

void *get_psp() {
  void *psp;
  asm volatile("MRS %0, PSP" : "=r"(psp));
  return psp;
}
void *get_msp() {
  void *msp;
  asm volatile("MRS %0, MSP" : "=r"(msp));
  return msp;
}

void test_svc_handler() {
  printf("test_svc_handler\n");
  //  sleep_ms(100);
}

void hardfault_handler() {

  uint32_t volatile exc_lr;
  asm volatile("mov %[lr], lr" : [lr] "=r"(exc_lr));
  // EXC_RETURN bit2: 0=MSP, 1=PSP。例外発生時のスタックポインタを選び、
  // そこに積まれた例外フレーム {r0,r1,r2,r3,r12,lr,pc,xPSR} を読む。
  uint32_t *sf = (exc_lr & 0x4) ? (uint32_t *)get_psp() : (uint32_t *)get_msp();
  while (1) {
    printf("HardFault: exc_lr=%lx psp=%p msp=%p\n", exc_lr, get_psp(),
           get_msp());
    printf("  r0=%08lx r1=%08lx r2=%08lx r3=%08lx\n", sf[0], sf[1], sf[2],
           sf[3]);
    printf("  r12=%08lx lr=%08lx pc=%08lx xpsr=%08lx\n", sf[4], sf[5], sf[6],
           sf[7]);
    sleep_ms(1000);
  }
}

int main(int argc, char const *argv[]) {
  stdio_init_all();
  exception_set_exclusive_handler(HARDFAULT_EXCEPTION, hardfault_handler);
  sleep_ms(2000);
  shizu::init();
  while (1) {
    sleep_ms(500);
    printf("no_return\n");
  }
  return 0;
}
