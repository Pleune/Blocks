if command -v nix-shell; then
  exec  nix-shell --run make
else
  exec make
fi
