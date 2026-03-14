#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$ROOT_DIR/raytracer"
REF_DIR="$ROOT_DIR/references"
OUT_DIR="$ROOT_DIR/regression_out"

mkdir -p "$REF_DIR" "$OUT_DIR"

SCENES=("spheres" "metalglass" "cornell")
WIDTH=640
HEIGHT=360
SPP=16
DEPTH=6

if [[ ! -x "$BIN" ]]; then
  echo "raytracer binary not found. Build first:"
  echo "  g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o raytracer"
  exit 1
fi

if [[ "${1:-}" == "--generate-reference" ]]; then
  for scene in "${SCENES[@]}"; do
    echo "Generating reference for ${scene}..."
    "$BIN" --scene "$scene" --width "$WIDTH" --height "$HEIGHT" --spp "$SPP" --max-depth "$DEPTH" \
      --output-base "$REF_DIR/${scene}_ref"
  done
  echo "Reference generation complete."
  exit 0
fi

for scene in "${SCENES[@]}"; do
  ref="$REF_DIR/${scene}_ref.ppm"
  out="$OUT_DIR/${scene}_new"

  if [[ ! -f "$ref" ]]; then
    echo "Missing reference: $ref"
    echo "Run: ./quality_regression.sh --generate-reference"
    exit 2
  fi

  echo "Rendering ${scene} for regression..."
  "$BIN" --scene "$scene" --width "$WIDTH" --height "$HEIGHT" --spp "$SPP" --max-depth "$DEPTH" \
    --output-base "$out"

  echo "Comparing ${scene}..."
  "$BIN" --compare "$ref" "${out}.ppm" --rmse-threshold 0.035 --ssim-threshold 0.96
done

echo "Regression pass."
