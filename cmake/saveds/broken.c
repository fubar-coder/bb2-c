/* cmake/check-saveds.cmake, test toolchain_saveds_broken: no __saveds,
   so no a4 load - the check must fail */
int g;
int f(void) { return g; }
