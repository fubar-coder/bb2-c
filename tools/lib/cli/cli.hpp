/*
 * cli.hpp - the command-line shell of the host programs (CLI11;
 * docs/development.md, "Host programs").
 *
 * A program's main() declares its subcommands (or, with one mode, its
 * options) on a CLI::App and returns cli::run().  A subcommand binds its
 * options to its own state and sets the status in its callback; CLI11
 * calls it after the whole command line was parsed, so a usage error runs
 * nothing.
 *
 * cli::run():
 *   -h, --help (also after a subcommand)    the help on stdout, 0
 *   a command line CLI11 rejects, a name    the message and the way to
 *   that is no subcommand                   the help on stderr, 2
 *   otherwise                               the status the callback set
 * The help has one shape in every program (cli::Formatter): the Usage:
 * line first, then the description, the arguments, the options, the
 * subcommands and the footer, at a fixed width (the terminal does not
 * change it), without colour.
 */
#ifndef CLI_HPP
#define CLI_HPP

#include <CLI/CLI.hpp>

#include <exception>
#include <iostream>
#include <memory>
#include <string>

namespace cli {

/* CLI11's formatter, with the Usage: line first */
class Formatter : public CLI::Formatter {
public:
    Formatter()
    {
        column_width(30);
        right_column_width(65);
        description_paragraph_width(80);
        footer_paragraph_width(80);
    }

    std::string make_help(const CLI::App *app, std::string name, CLI::AppFormatMode mode) const override
    {
        if (mode == CLI::AppFormatMode::Sub)
            return make_expanded(app, mode);
        std::string usage = make_usage(app, name);   // "\nNAME ...\n\n"
        usage.erase(0, usage.find_first_not_of('\n'));
        usage.erase(usage.find_last_not_of('\n') + 1);
        std::string out = "Usage: " + usage + "\n";
        std::string desc = make_description(app);
        if (!desc.empty()) {
            std::stringstream para;
            CLI::detail::streamOutAsParagraph(para, desc, get_description_paragraph_width(), "");
            std::string d = para.str();
            d.erase(d.find_last_not_of('\n') + 1);
            out += "\n" + d + "\n";
        }
        out += make_positionals(app);
        out += make_groups(app, mode);
        out += make_subcommands(app, mode);
        std::string footer = make_footer(app);   // "\nFOOTER\n"
        if (!footer.empty()) {
            std::stringstream para;
            CLI::detail::streamOutAsParagraph(para, footer, get_footer_paragraph_width());
            out += para.str();
        }
        return out;
    }
};

/* "PROGRAM SUB" (PROGRAM for the program itself): the prefix of messages */
inline std::string name(const CLI::App &app)
{
    std::string n = app.get_name();
    for (const CLI::App *p = app.get_parent(); p; p = p->get_parent())
        n = p->get_name() + " " + n;
    return n;
}

/* the formatter for APP and its subcommands */
inline void format(CLI::App &app, const std::shared_ptr<Formatter> &f)
{
    app.formatter(f);
    for (CLI::App *sub : app.get_subcommands({}))
        format(*sub, f);
}

/* parses argv, runs the selected subcommand; the exit status (above) */
inline int run(CLI::App &app, int argc, char **argv, int &status)
{
    format(app, std::make_shared<Formatter>());
    const std::string hint = "Run with --help for more information.";
    if (argc > 1 && argv[1][0] != '-' && !app.get_subcommands({}).empty()
        && !app.get_subcommand_no_throw(argv[1])) {
        std::cerr << app.get_name() << ": " << argv[1] << " is no subcommand\n" << hint << "\n";
        return 2;
    }
    try {
        app.parse(argc, argv);
    } catch (const CLI::CallForHelp &e) {
        app.exit(e);
        return 0;
    } catch (const CLI::CallForAllHelp &e) {
        app.exit(e);
        return 0;
    } catch (const CLI::ParseError &e) {
        app.exit(e);
        return 2;
    } catch (const std::exception &e) {
        std::cerr << app.get_name() << ": " << e.what() << "\n" << hint << "\n";
        return 2;
    }
    return status;
}

}

#endif
