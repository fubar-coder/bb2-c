/* cmake/check-saveds.cmake: __saveds must load a4 here */
int g;
__saveds int f(void) { return g; }
