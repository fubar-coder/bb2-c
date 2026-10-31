/*
 * junit.h - reading the JUnit XML files ctest writes (--output-junit,
 * the presets' outputJUnitFile): the subset ctest uses.
 *
 *   <?xml ...?>
 *   <testsuite name=... tests=... timestamp=... ...>
 *     <testcase name="N" classname="N" time="SEC" status="run|fail|notrun">
 *       <failure message=.../> | <skipped message=.../>
 *       <properties>...</properties>
 *       <system-out>the output of the test</system-out>
 *     </testcase>
 *   </testsuite>
 *
 * The XML reader is minimal but strict: elements, attributes in "" or '',
 * the five named entities and character references, comments, processing
 * instructions, a DOCTYPE without internal subset, CDATA.  Anything else
 * (a tag not closed, end tags that do not match, a bad entity, text after
 * the root) is malformed: the file is refused with a message and its line.
 * A testcase without name or time, or with a time that is no number, is
 * reported and left out; the rest of the file counts.
 */
#ifndef JUNIT_H
#define JUNIT_H

#include <stddef.h>

enum junit_status { JUNIT_PASSED, JUNIT_FAILED, JUNIT_SKIPPED };

struct junit_case {
    char *name;
    double time;                /* seconds */
    enum junit_status status;   /* status="run" passed, "notrun" or <skipped> skipped,
                                   anything else (or <failure>) failed */
    char *out;                  /* <system-out>, the entities resolved; "" without */
};

struct junit_file {
    char *timestamp;            /* of the testsuite, "" without */
    struct junit_case *cases;
    size_t ncases;
    char **problems;            /* testcases left out, why (with the line) */
    size_t nproblems;
};

/* reads text (len bytes): 0 with *f filled (problems maybe), -1 if the
   file is malformed or memory ran out (a message in err; *f empty) */
int junit_parse(const char *text, size_t len, struct junit_file *f, char *err, size_t errlen);
void junit_free(struct junit_file *f);

#endif
