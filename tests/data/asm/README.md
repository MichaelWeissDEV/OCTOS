Real compiler output used as parser corpus. Regenerate with (Apple clang 17, ELF x86-64 target):

    clang++ --target=x86_64-unknown-linux-gnu -S -fno-asynchronous-unwind-tables -masm=att -g -O0 corpus.cpp -o clang_x86_64_att_O0.s

GCC output is NOT part of this corpus: no GNU GCC is available on the development host (the
`gcc` binary on macOS is Apple clang). GCC-style constructs are covered by hand-written
fixtures in the unit tests and must be re-verified against a real gcc image.
