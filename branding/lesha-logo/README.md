# Лёша VPN logo (ЛЁША monogram)

Generators for the fork's logo assets (FORK.md §6). The glyph is a ligature of Л, Ё, Ш and А
(`v1_row` in `gen.py`): thick rough strokes with horns and drips, filled with flat pastel patches.
Everything is deterministic (fixed seeds), so re-running reproduces the committed files.

Requirements: Python 3 with `shapely`, `numpy`, `Pillow`; Node with `playwright` (Chromium rasterises the SVGs).

```
cd branding/lesha-logo
python3 gen.py /tmp/variants                 # all monogram variants as SVG (preview only)
python3 make_assets.py prep                  # glyph SVGs + render list in $LOGO_WORK (/tmp/lesha-logo-work)
node render_jobs.js /tmp/lesha-logo-work/render.json
python3 make_assets.py compose               # writes the raster icons in place
python3 make_vectors.py                      # controls/amnezia.svg, ic_amnezia_round.xml, Full_logo.svg, app.ico, app.icns
```

`make_assets.py compose` repaints the solid interior of each icon's existing shape (circle, square, squircle)
with a vertical gradient and draws the glyph on top, so run it on the committed icons, not on upstream ones.
For upstream icons, take upstream's file once, then re-run the steps above.
