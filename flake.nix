{
  description = "GTK";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05"; # 可换成 unstable

  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
  in
  {
    devShells.${system} = {
      gtk3 = pkgs.mkShell {
        buildInputs = [ pkgs.gcc pkgs.gtk3 pkgs.pkg-config ];
        shellHook = ''
          echo "GTK3 C dev shell ready"
          echo 'Build with: gcc demo.c -o demo `pkg-config --cflags --libs gtk+-3.0`'
          echo "Run with: ./demo"
        '';
      };
      default = pkgs.mkShell {
          buildInputs = [ pkgs.gcc pkgs.gtk4 pkgs.pkg-config ];
          shellHook = ''
          echo "GTK4 C dev shell ready"
          echo 'Build with: gcc rest.c -o rest `pkg-config --cflags --libs gtk4`'
          echo "Run with: ./rest"
          '';
      };
    };
  };
}
