# Growth Program Source Notes (2026-02-27)

This file records internet sources used to refresh the default Growth Program catalog.

## Fertilizer lines

- Terra Aquatica TriPart feeding chart (official PDF):
  - https://aptus-holland.com/wp-content/uploads/2020/12/TriPart-Feedchart.pdf
  - Used for Aquatica TriPart stage values and Grow/Micro/Bloom ratio shift by phase.
- General Hydroponics FloraSeries usage guide (official):
  - https://generalhydroponics.com/wp-content/uploads/assets/GH_FloraSeries_UsageGuide.pdf
  - Used for FloraSeries stage ratios (e.g., 2-1-1 for mild veg and 1-2-3 for aggressive bloom).
- Advanced Nutrients pH Perfect Grow/Micro/Bloom (official product guidance):
  - https://www.advancednutrients.com/products/ph-perfect-grow-micro-bloom/
  - Used for equal-part progression by week: 1 ml/L -> 2 ml/L -> 3 ml/L -> 4 ml/L.
- FoxFarm Hydro Trio references (official):
  - https://foxfarm.com/product/big-bloom-liquid-fertilizer/
  - https://foxfarm.com/product/grow-big-hydro-liquid-concentrate-fertilizer/
  - https://foxfarm.com/product/tiger-bloom-liquid-fertilizer/
  - Used for relative phase weighting of A/B/C channels in the FoxFarm preset.

## Plant selection

- University of Arkansas (Division of Agriculture) hydroponics guide:
  - https://www.uaex.uada.edu/farm-ranch/crops-commercial-horticulture/horticulture/ar-home-hydroponics/default.aspx
  - Used for common crop groups and examples (lettuce, basil/mint herbs, tomatoes, peppers, cucumbers, berries).
- Oregon State Extension: hydroponic crop categories:
  - https://catalog.extension.oregonstate.edu/em9457/html
  - Used to keep the list centered on common hydroponic categories (leafy greens, herbs, fruiting crops).

## Mapping notes

- The UI still uses 3 nutrient channels (`A/B/C`), so all fertilizer presets are normalized into 3-part phase profiles.
- Plant presets define frequency (`feedingsPerWeek`) and plant multiplier (`nutrientMul`), then combine with fertilizer phase tables.
