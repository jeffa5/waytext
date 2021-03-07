{
  description = "waytext";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils = {
      url = "github:numtide/flake-utils";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = import nixpkgs {
            system = system;
          };
        in
        rec
        {
          packages = {
            waytext = pkgs.stdenv.mkDerivation {
              name = "waytext";
              src = ./.;
              buildInputs = with pkgs; [
                meson
                pkgconfig
                cmake
                cairo
                wayland
                wayland-protocols
                scdoc
                ninja
              ];
            };
          };

          defaultPackage = packages.waytext;

          apps = {
            waytext = flake-utils.lib.mkApp {
              drv = packages.waytext;
            };
          };

          defaultApp = apps.waytext;

          devShell = pkgs.mkShell {
            buildInputs = with pkgs;[
              meson
              pkgconfig
              cmake
              cairo
              wayland
              wayland-protocols
              scdoc
              ninja

              rnix-lsp
              nixpkgs-fmt
            ];
          };
        }
      );
}
