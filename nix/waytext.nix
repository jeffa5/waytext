{pkgs}:
pkgs.stdenv.mkDerivation {
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
}
