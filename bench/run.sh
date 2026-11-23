#!/bin/sh
# Runs the repaint benchmark for every theme, scale and scenario, several times each, and
# prints one CSV line per run.
#   bench/run.sh <path to HpqRepaintBench> [runs] [repetitions]
bench=$1
runs=${2:-5}
repetitions=${3:-200}
export QT_QPA_PLATFORM=offscreen
# One processor, always the same, when the system can pin a process: steadier times.
pin=""
if command -v taskset >/dev/null 2>&1; then
  pin="taskset -c 2"
fi
echo "theme,scale,scenario,repetitions,median_ms,p10_ms,p90_ms,min_ms,geometry_ms,geometry_calls"
for run in $(seq "$runs"); do
  for scale in 1 2; do
    for theme in none sketch sketch-hachure china-ink slate slate-hachure notebook velleda \
      watercolor watercolor-hachure blueprint invisible-ink highlighter; do
      for scenario in full resize interact scroll table line-edit button; do
        QT_SCALE_FACTOR=$scale $pin "$bench" --theme "$theme" --scenario "$scenario" \
          --repetitions "$repetitions" 2>/dev/null
      done
    done
  done
done
