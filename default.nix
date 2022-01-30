{ lib, stdenv, fetchFromGitHub
, meson, ninja, pkg-config, scdoc
, cmake
, fetchzip
, openssl
, getconf
, libsodium
, wayland, wayland-protocols, libxkbcommon, cairo, gdk-pixbuf, pam
}:
stdenv.mkDerivation rec {
  pname = "schlock";
  version = "0.1";

  src = ./.;

  postPatch = ''
    substituteInPlace meson.build \
      --replace "version: '1.4'" "version: '${version}'"
  '';

  nativeBuildInputs = [ meson ninja pkg-config scdoc cmake ];
  buildInputs = [
    cairo
    gdk-pixbuf
    libsodium
    libxkbcommon
    wayland
    wayland-protocols
  ];

  mesonFlags = [
    "-Dgdk-pixbuf=enabled" # "-Dman-pages=enabled"
  ];

  meta = with lib; {
    description = "Touchscreen locker for Wayland";
    longDescription = ''
      schlock is a fork of Swaylock adapted for touchscreen devices.
    '';
    inherit (src.meta) homepage;
    license = licenses.mit;
    # platforms = platforms.linux;
    # maintainers = with maintainers; [ primeos ];
  };
}
