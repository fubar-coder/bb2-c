# fake_cli11

The fake program of the README check (`cmake/check-readme.cmake`).

```
Usage: fake_cli11 [OPTIONS] SUBCOMMAND

The fake program of the help and README checks.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  alpha                       the first subcommand
  beta                        the second subcommand
```

```
Usage: fake_cli11 alpha [OPTIONS] FILE

the first subcommand

POSITIONALS:
  FILE TEXT REQUIRED          the input

OPTIONS:
  -h,     --help              Print this help message and exit
  -x                          a flag
```

The help of `beta` is missing.
