with import <nixpkgs> {
  # overlays = [
  #   (self: super: {
  #     stdenv = super.gcc12Stdenv;
  #   })
  # ];
};

let myStdenv = gcc12Stdenv;
in
{
  myEnv = myStdenv.mkDerivation {
    name = "my-env";
    buildInputs = [
      (protobuf.override (old: {
        stdenv = myStdenv;
        # Need to also override `protobuf`'s dependencies with the older GCC,
        # otherwise building `protobuf` itself gives linker errors.
        abseil-cpp = pkgs.abseil-cpp.override { stdenv = myStdenv; };
        gtest = pkgs.gtest.override { stdenv = myStdenv; };
      }))
      pkg-config
    ];
  };
}
