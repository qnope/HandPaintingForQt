# Repaint benchmark

`HpqRepaintBench` shows the example window on the offscreen platform and measures how long
`QWidget::repaint()` takes under a theme: the whole pipeline of a real repaint (the style, the
hand paint engine, the geometry engine, the raster engine of Qt and the backing store).

```
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DHPQ_BUILD_BENCH=ON
cmake --build build/release
bench/run.sh build/release/bench/HpqRepaintBench > results.csv
```

- Themes: `none` (plain Fusion, as a floor), `sketch`, `sketch-hachure`, `slate`, `slate-hachure`.
- Scale: `QT_SCALE_FACTOR=1` or `2` (the device pixel ratio of the window).
- Scenarios: `full` repaints the whole window as it is; `resize` gives it a new size before
  each repaint; `table`, `line-edit` and `button` repaint one widget, as a scroll, a key press
  or a hover do; `rectangles` repaints small parts of the window, to check with
  `--backing-store` that they leave it as a repaint of the whole draws it.
- The geometry engine has a fixed base seed: every run draws the same window, and
  `--screenshot` or `--backing-store` save it to compare two versions pixel by pixel.

Each line gives the median, the 10th and 90th percentiles and the minimum of the repaint
time, then the time spent in the geometry engine during one repaint, in milliseconds.
