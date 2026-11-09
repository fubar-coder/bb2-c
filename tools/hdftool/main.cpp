/*
 * hdftool - read and adapt AmigaDOS hardfiles (see hdf.h): one line about
 * an image (info), a directory (list), a file (cat), the file system
 * (check), an edited copy (edit).  README.md here; the subcommands are in
 * cmd_hdf.cpp.
 */
#include "cli.hpp"
#include "cmds.h"

int main(int argc, char **argv)
{
    CLI::App app{"AmigaDOS hardfiles: an RDB hardfile with one partition or a raw partition, DOS\\0 to "
                 "DOS\\3. A PATH or DIR goes from the root, separated by '/'.",
                 "hdftool"};
    int status = 0;

    app.require_subcommand(1);
    cmd_info(app, status);
    cmd_list(app, status);
    cmd_cat(app, status);
    cmd_check(app, status);
    cmd_edit(app, status);
    return cli::run(app, argc, argv, status);
}
