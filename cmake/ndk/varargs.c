/* The test of the shadow headers' varargs wrappers (cmake/ndk-test.cmake,
   ctest ndk_varargs_inline): proto/intuition.h and one varargs form; with
   the shadow headers only its wrapper __OpenWindowTags_va is emitted, with
   the code the NDK's own header gives. */
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <proto/intuition.h>

struct Window *open_window(void)
{
    return OpenWindowTags(NULL, WA_Width, 100, TAG_END);
}
