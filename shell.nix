{ pkgs ? import <nixpkgs> { } }:

with pkgs;

mkShell {
buildInputs = [ gcc gnumake gdb ]; # your dependencies here
}
