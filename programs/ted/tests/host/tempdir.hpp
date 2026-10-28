// tempdir.hpp - a temporary directory for the host tests' files, removed
// at the end (it must be empty by then).
#ifndef TED_TEMPDIR_HPP
#define TED_TEMPDIR_HPP

#include <cstdlib>
#include <string>
#include <unistd.h>

struct tempdir {
    std::string path;

    tempdir()
    {
        const char *tmp = std::getenv("TMPDIR");
        std::string tpl = std::string(tmp && *tmp ? tmp : "/tmp") + "/ted-host-XXXXXX";

        if (mkdtemp(tpl.data()))
            path = tpl;
    }
    ~tempdir()
    {
        if (!path.empty())
            rmdir(path.c_str());
    }
    tempdir(const tempdir &) = delete;
    tempdir &operator=(const tempdir &) = delete;
};

#endif
