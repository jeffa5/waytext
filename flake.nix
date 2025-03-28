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
        waytext = pkgs.callPackage ./nix/waytext.nix {};
      };

      apps = {
        waytext = flake-utils.lib.mkApp {
          drv = self.packages.${system}.waytext;
        };
      };

      formatter = pkgs.alejandra;

      devShells.default = pkgs.mkShell {
        inputsFrom = [self.packages.${system}.waytext];
      };
    })
    // {
      overlays.default = final: _prev: {
        waytext = final.callPackage ./nix/waytext.nix {};
      };
    };
}
