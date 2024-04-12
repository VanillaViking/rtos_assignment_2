{ pkgs ? import <nixpkgs> { } }:

with pkgs;

mkShell {
buildInputs = [ gnumake gcc ]; # your dependencies here
}
