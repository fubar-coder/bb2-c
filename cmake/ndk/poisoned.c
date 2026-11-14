/* The test of the shadow headers' poison (cmake/ndk-test.cmake, ctest
   ndk_poisoned_call): BuildSysRequest takes its negative text in a3 and
   has no safe form, so a library may not call it; with the shadow
   headers this does not compile. */
#include <intuition/intuition.h>
#include <proto/intuition.h>

struct Window *request(struct IntuiText *body, struct IntuiText *pos,
                       struct IntuiText *neg)
{
    return BuildSysRequest(NULL, body, pos, neg, 0, 320, 64);
}
