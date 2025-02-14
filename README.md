# gcc-openmp-loop-bug-repro-2025

The code in `Repro.cpp` shows an apparent GCC miscompilation of an OpenMP `parallel for` loop.

* I suspect that some `double`s that should be truncated to `float`s are not truncated in the OpenMP case.
* The OpenMP loop has only 1 iteration.

Versions used:

* `g++` 12.3.0
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
* Bug gone when changing _either_ of the two lines
  ```c++
  arr[0] = color[0];
  arr[1] = color[1];
  ```
  to `= 0` (nees to be done in the serial and parallel version equally).
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
color = 200000,300000,0
i = 1 ; color = 200000,300000,0 ; arr = 200000,300000,0

Parallel loop:
color = 200000,300000.00030000001,0
i = 1 ; color = 200000,300000.00030000001,0 ; arr = 200000,300000.00030000001,0

colors_parallel[1][0] = 200000.00020000001,300000.00030000001,0
colors_serial  [1][0] = 200000,300000,0

colors serial==parallel? = 0

BUG? YES
```

`BUG? YES` indicates the bug being present.


* In the serial loop, the variable `color` when printed contains value `300000` but in the parallel loop it's `300000.00030000001`.
  * There is an additional weirdness:
    When changing from
    ```c++
    #if 0
    void reproFunSerial() {
    ```
    to
    ```c++
    #if 1
    void reproFunSerial() {
    ```
    then the above changes to `300000`.
    That is even though `reproFunSerial()` is an unused function.
* In the parallel loop, the output `color = 200000,300000.00030000001,0` is already weird:
  Why does the `200000` get apparently narrowed to `float`, but the `300000.00030000001` has `double` precision?
* But in `colors_parallel[1][0]` it's `double` precision again: `200000.00020000001`
  * It is really weird that this happens only for 1 of multiple `array` elements.
* `colors_parallel` and `colors_serial` do not match.

Explanation guesses:

* Maybe the bug is that the `double` -> `float` narrowing is somehow forgotten in the OpenMP loop?
* Maybe it's a combination of "excess precision" (as covered by https://gcc.gnu.org/bugzilla/show_bug.cgi?id=323), OpenMP, and vectorization?


## Running on Ubuntu

When building with `g++-12` from Ubuntu 24.04, the bug seems to go away.

```
Ubuntu GCC version:  g++-12 (Ubuntu 12.3.0-17ubuntu1) 12.3.0
Nix    GCC version:  gcc (GCC) 12.3.0
```

Ubuntu output:

```
Serial loop:
color = 200000,300000,0
i = 1 ; color = 200000,300000,0 ; arr = 200000,300000,0

Parallel loop:
color = 200000,300000,0
i = 1 ; color = 200000,300000,0 ; arr = 200000,300000,0

colors_parallel[1][0] = 200000,300000,0
colors_serial  [1][0] = 200000,300000,0

colors serial==parallel? = 1

BUG? no
```
