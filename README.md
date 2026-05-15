# `nvgview`

`nvgview` is an NVGIF image viewer for Popcorn OS. It just shows them on the screen.

## Usage

`nvgview file.nvg`

Where file.nvg is the NVGIF you want to view.

## How does this work?

`nvgif.h` is the combination of the `nvgif.h` and `nvgif.c` from the [offical NVGIF C implementation](https://github.com/tiash-and-cats/nvgif/tree/master/c), ported to Popcorn OS. If `nvg_IMPLEMENTATION` is defined, the implementation is included, otherwise the header is included, almost in the style of STB. This `nvgif.h` gets included twice in `main.c`, once with `nvg_IMPLEMENTATION` undefined at the start of the file, the second with it defined at the end, after `pop_main`.

The API of the NVGIF implementation is almost unchanged, except:

- All functions take a `pop_Services*` as their first argument.
- `nvg_error` and `nvg_get_errval` don't exist.
- Instead of including `stdlib.h`, `stdarg.h`, `stdint.h` and `stdio.h`, it includes `popcorn.h`.

You can find the docs for the original [here](https://tiash-and-cats.github.io/nvgif/implementations/c.html).

## Compilation

Simply run `make` with GCC and GMake installed.

## Licensing

NVGIF uses the MIT license, so `nvgview` uses it as well.