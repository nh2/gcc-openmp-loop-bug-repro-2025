# gcc-openmp-loop-bug-repro-2025

The code in `Repro.cpp` shows an apparent GCC miscompilation of an OpenMP `parallel for` loop.

* I suspect that some `double`s that should be truncated to `float`s are not truncated in the OpenMP case.
* The OpenMP loop has only 1 iteration.
* It involves the use of `protobuf`, though I suspect it's probably independent of protobuf. But I could not minimise `protobuf` away and still have the repro work.

Versions used:

* `g++` 12.3.0
* `protobuf` 24.4.0
* Linux `x86_64`


## Reproduce with pinned versions using Nix

To get the exact same versions I used: On any Linux distro, [install Nix](https://nixos.org/download/), then run:

```sh
NIX_PATH=nixpkgs=https://github.com/NixOS/nixpkgs/archive/b134951a4c9f3c995fd7be05f3243f8ecd65d798.tar.gz nix-shell shell.nix --run ./repro.sh
```


## Observations

The compiler invocation is in [`repro.sh`](./repro.sh).

Why I'm reasonably sure this is a compiler bug:

* The OpenMP loop has only 1 iteration. So data races are not really possible.
* Bug gone when removing `-std=c++20`.
* Bug gone when compiling with `-O1`, or adding `-fno-tree-slp-vectorize` to `-O2`.
* Bug gone when commenting out _either_ of the two lines
  ```c++
  colorProto->set_x(color[2]);
  colorProto->set_y(color[1]);
  ```
  This further supports the idea the bug has something to do with vectorization.
* Bug gone with UBsan
* Bug gone with Tsan
* Bug gone with clang
* Bug gone with GCC `13.2.0` (but unclear if fixed or just lucky heuristics changes).
* Clean in `valgrind` (but reproducing the bug).


## Explaining the output

The code in `Repro.cpp` runs the same loop twice (once without OpenMP, once with), and compares the the results.
They should be equal, but are not.

Further, some prints of variables are done inside the loop.

Output:

```
Serial loop:
i = 1 ; color = 0,200000,300000 ; colorProto = 300000,200000,0

Parallel loop:
i = 1 ; color = 0,200000.00020000001,300000.00030000001 ; colorProto = 300000.00030000001,200000.00020000001,0

colors_parallel[1][0] = 0,200000,300000
colors_serial  [1][0] = 0,200000,300000

colorProtoV3s_serial   = [300000,200000,0]
colorProtoV3s_parallel = [300000.00030000001,200000.00020000001,0]
  equal = [0,0,1]

colors        serial==parallel? = 1
colorProtoV3s serial==parallel? = 0

BUG? YES
```

`BUG? YES` indicates the bug being present.


* In the serial loop, the variable `color` when printed contains value `200000` but in the parallel loop it's `200000.00020000001`.
* However, the same variable put into a vector is then only `20000` (for both serial and parallel).
  * See `colors_parallel[1][0]` and `colors_serial  [1][0]`.
* But the same variable put into a proto is `200000.00020000001` again (only for parallel).
  * See `colorProtoV3s_parallel`.
* The above by itself already seems bugged.
* As a result, we also observe that the `colors` vector is equal across the 2 loops, but the `colorProtoV3s` vector is not.

Maybe the bug is that the `double` -> `float` narrowing is somehow forgotten in the OpenMP loop?


## Trying to reproduce without Nix's GCC

When building with `g++-12` from Ubuntu 24.04, the bug seems to go away.

`LD_LIBRARY_PATH` is set below to use Ubuntu's `libstdc++`, `libgomp.so`, and `libgcc_s.so`; otherwise `./repro` program would crash at load time with `Floating point exception`.

But all other libraries (e.g. `protobuf`) are still loaded from their Nix store paths.

```sh
NIX_PATH=nixpkgs=https://github.com/NixOS/nixpkgs/archive/b134951a4c9f3c995fd7be05f3243f8ecd65d798.tar.gz nix-shell shell.nix --run '/usr/bin/g++-12 -Wall -I. -fopenmp gen/Repro.pb.cc Repro.cpp $(pkg-config --cflags --libs protobuf) -o repro -std=c++20 -O2 && LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu ./repro'
```

This may potentially suggest that the problem occurs only with Nix's GCC.
