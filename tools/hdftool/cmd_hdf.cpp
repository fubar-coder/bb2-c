/*
 * hdftool info, list, cat, check, edit - read and adapt AmigaDOS hardfiles
 * (see hdf.h): the setup of the emulator's Workbench hardfiles from Amiga
 * Forever (cmake/setup-amiga-forever.cmake, AMIGA-EMULATION-SETUP.md).
 *
 *   hdftool info IMAGE
 *   hdftool list IMAGE [DIR]
 *   hdftool cat IMAGE PATH
 *   hdftool check IMAGE
 *   hdftool edit IN OUT [--partition] OP...
 *     --write PATH TEXT     write a file (create or replace)
 *     --write-hex PATH HEX  the same, the contents as hexadecimal digits
 *     --replace PATH OLD NEW  replace text that occurs exactly once
 *     --copy FROM TO        copy a file inside the image
 *
 * edit applies the operations in memory, in the order given, checks the
 * file system and only then writes OUT, which must not exist (never
 * overwritten); --partition writes only the partition, a raw image
 * without the RDB.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "hdf.h"
}
#include "cmds.h"

static uint8_t *load(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    uint8_t *buf = NULL;
    long n;

    if (!f || fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) {
        fprintf(stderr, "hdftool: %s: %s\n", path, strerror(errno));
        if (f)
            fclose(f);
        return NULL;
    }
    buf = (uint8_t *)malloc(n ? (size_t)n : 1);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "hdftool: %s: cannot read\n", path);
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *size = (size_t)n;
    return buf;
}

/* OUT, created new (O_EXCL); removed again if it cannot be written whole */
static int save(const char *path, const uint8_t *data, size_t size)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    size_t done = 0;
    ssize_t w;

    if (fd < 0) {
        fprintf(stderr, "hdftool: %s: %s\n", path, strerror(errno));
        return -1;
    }
    while (done < size) {
        w = write(fd, data + done, size - done);
        if (w <= 0) {
            fprintf(stderr, "hdftool: %s: %s\n", path, strerror(errno));
            close(fd);
            unlink(path);
            return -1;
        }
        done += (size_t)w;
    }
    if (close(fd)) {
        fprintf(stderr, "hdftool: %s: %s\n", path, strerror(errno));
        unlink(path);
        return -1;
    }
    return 0;
}

/* the operations of SUB in the order given, then the check and OUT */
static int edit(struct hdf *h, const CLI::App &sub, const char *out, int partition)
{
    char err[300];
    int r;

    for (const cli::occurrence &oc : cli::occurrences(sub)) {
        const std::string op = oc.option->get_name();
        const std::vector<std::string> &v = oc.values;

        if (op == "--write")
            r = hdf_write(h, v[0].c_str(), v[1].c_str(), v[1].size(), err, sizeof err);
        else if (op == "--write-hex")
            r = hdf_write_hex(h, v[0].c_str(), v[1].c_str(), err, sizeof err);
        else if (op == "--replace")
            r = hdf_replace(h, v[0].c_str(), v[1].c_str(), v[2].c_str(), err, sizeof err);
        else if (op == "--copy")
            r = hdf_copy(h, v[0].c_str(), v[1].c_str(), err, sizeof err);
        else
            continue;
        if (r) {
            fprintf(stderr, "hdftool: %s\n", err);
            return 1;
        }
    }
    if (hdf_check(h, err, sizeof err)) {
        fprintf(stderr, "hdftool: after the changes: %s\n", err);
        return 1;
    }
    if (partition)
        r = save(out, h->data + h->offset, (size_t)h->nblocks * HDF_BLOCK);
    else
        r = save(out, h->data, h->size);
    return r ? 1 : 0;
}

/* the subcommands: what they do with the opened image */
enum which { INFO, LIST, CAT, CHECK, EDIT };

struct options {
    std::string image, arg, out;
    bool partition = false;
    std::vector<std::string> write, write_hex, replace, copy;
};

static int run(const CLI::App &sub, enum which w, const options &o)
{
    struct hdf h;
    char err[300], name[HDF_NAMEMAX + 2];
    const char *image = o.image.c_str();
    uint8_t *img, *buf;
    size_t size, len;
    int r = 1;

    if (!(img = load(image, &size)))
        return 1;
    if (hdf_open(&h, img, size, err, sizeof err)) {
        fprintf(stderr, "hdftool: %s: %s\n", image, err);
        free(img);
        return 1;
    }
    switch (w) {
    case INFO:
        hdf_volume_name(&h, name);
        printf("%s%s%s DOS\\%u blocks %u reserved %u offset %zu root %u free %u volume %s\n",
               h.rdb ? "rdb" : "raw", h.rdb ? " " : "", h.partname, h.dostype & 0xFF, h.nblocks,
               h.reserved, h.offset, h.root, hdf_free_blocks(&h), name);
        r = 0;
        break;
    case CHECK:
        r = hdf_check(&h, err, sizeof err) != 0;
        if (r)
            fprintf(stderr, "hdftool: %s: %s\n", image, err);
        break;
    case LIST:
        r = hdf_list(&h, o.arg.c_str(), stdout, err, sizeof err) != 0;
        if (r)
            fprintf(stderr, "hdftool: %s\n", err);
        break;
    case CAT:
        if (hdf_read(&h, o.arg.c_str(), &buf, &len, err, sizeof err)) {
            fprintf(stderr, "hdftool: %s\n", err);
        } else {
            r = fwrite(buf, 1, len, stdout) != len;
            free(buf);
        }
        break;
    case EDIT:
        r = edit(&h, sub, o.out.c_str(), o.partition);
        break;
    }
    hdf_close(&h);
    free(img);
    return r;
}

/* the subcommand NAME on IMAGE */
static CLI::App *add(CLI::App &app, int &status, enum which w, const char *name, const char *description,
                     const char *image)
{
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand(name, description);

    sub->add_option(w == EDIT ? "IN" : "IMAGE", o->image, image)->required();
    if (w == LIST)
        sub->add_option("DIR", o->arg, "the directory, from the root (default: the root)");
    if (w == CAT)
        sub->add_option("PATH", o->arg, "the file, from the root")->required();
    if (w == EDIT) {
        sub->add_option("OUT", o->out, "the image written; it must not exist")->required();
        sub->add_flag("--partition", o->partition, "write only the partition, a raw image without the RDB");
        cli::add_repeated(*sub, "--write", o->write, 2, "write a file (create or replace)")
            ->type_name("PATH TEXT");
        cli::add_repeated(*sub, "--write-hex", o->write_hex, 2, "the same, the contents as hexadecimal digits")
            ->type_name("PATH HEX");
        cli::add_repeated(*sub, "--replace", o->replace, 3, "replace text that occurs exactly once")
            ->type_name("PATH OLD NEW");
        cli::add_repeated(*sub, "--copy", o->copy, 2, "copy a file inside the image")->type_name("FROM TO");
    }
    sub->callback([sub, w, o, &status] { status = run(*sub, w, *o); });
    return sub;
}

void cmd_info(CLI::App &app, int &status)
{
    add(app, status, INFO, "info", "one line about the image: RDB or raw, dostype, blocks, volume", "the hardfile")
        ->footer("rdb NAME or raw, the dostype, the number of blocks, the reserved blocks, the "
                 "byte offset of the partition, the root block, the free blocks and the volume "
                 "name. Exit status 0, 1 the image cannot be read or is no hardfile it knows, 2 "
                 "usage error.");
}

void cmd_list(CLI::App &app, int &status)
{
    add(app, status, LIST, "list", "a directory of the image, one entry per line", "the hardfile")
        ->footer("In hash table order, NAME SIZE for a file, NAME/ for a directory. Exit status "
                 "0, 1 the image cannot be read or is no hardfile it knows, or DIR is no "
                 "directory, 2 usage error.");
}

void cmd_cat(CLI::App &app, int &status)
{
    add(app, status, CAT, "cat", "a file of the image to stdout", "the hardfile")
        ->footer("Exit status 0, 1 the image cannot be read or is no hardfile it knows, or PATH "
                 "is no file, 2 usage error.");
}

void cmd_check(CLI::App &app, int &status)
{
    add(app, status, CHECK, "check", "the file system of the image", "the hardfile")
        ->footer("Checksums and types of all blocks reachable from the root, no block used "
                 "twice, every used block allocated in the bitmap and every allocated block "
                 "used. Exit status 0, 1 the image cannot be read or the check fails, 2 usage "
                 "error.");
}

void cmd_edit(CLI::App &app, int &status)
{
    add(app, status, EDIT, "edit", "the operations applied to the image IN, the result checked and written to OUT",
        "the hardfile")
        ->footer("The operations are applied in memory in the order given; then the file system "
                 "is checked and OUT written, which must not exist (it is never overwritten). "
                 "Exit status 0, 1 IN cannot be read, an operation or the check fails, OUT exists "
                 "or cannot be written, 2 usage error.");
}
