{
  description = "waytext";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      packages = {
        waytext = pkgs.stdenv.mkDerivation {
          name = "waytext";
          src = ./.;
          buildInputs = with pkgs; [
            meson
            pkg-config
            cmake
            cairo
            wayland
            wayland-protocols
            wayland-scanner
            scdoc
            ninja
          ];
        };
      };

      apps = {
        waytext = flake-utils.lib.mkApp {
          drv = self.packages.${system}.waytext;
        };
      };

      devShells.default = pkgs.mkShell {
        inputsFrom = [self.packages.${system}.waytext];
      };
    })
    // {
      overlays.default = final: _prev: self.packages;
    };
}
