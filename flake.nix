{
  description = "waytext";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils = {
      url = "github:numtide/flake-utils";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }: let
    system = "x86_64-linux";
    pkgs = import nixpkgs {
      inherit system;
    };
  in rec
  {
    packages.${system} = {
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

    apps.${system} = {
      waytext = flake-utils.lib.mkApp {
        drv = packages.${system}.waytext;
      };
    };

    overlays.default = final: _prev: self.packages.${system};

    devShells.${system}.default = pkgs.mkShell {
      inputsFrom = [self.packages.${system}.waytext];
    };
  };
}
