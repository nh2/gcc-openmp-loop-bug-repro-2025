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
    ];
  };
}
