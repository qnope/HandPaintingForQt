#pragma once

#include <QBrush>
#include <QPalette>

namespace hpq {

// The brush of each color role of one group of a palette. A theme starts from its ground and
// from what it writes with, then gives their own brush to the roles that differ.
struct PaletteColors {
    // Every ground is `ground`, every other role is `text`.
    PaletteColors(const QBrush &ground, const QBrush &text);

    // The grounds.
    QBrush window;
    QBrush base;
    QBrush button;
    QBrush light;
    QBrush bright_text;
    QBrush tool_tip_base;
    QBrush alternate_base;
    QBrush midlight;
    // The lines.
    QBrush mid;
    QBrush dark;
    QBrush shadow;
    // Highlight, and Accent since Qt 6.6.
    QBrush highlight;
    QBrush highlighted_text;
    // Link and LinkVisited.
    QBrush link;
    QBrush placeholder_text;
    // WindowText, Text, ButtonText, ToolTipText and any later role.
    QBrush text;
};

// The palette whose Active and Inactive groups have the brushes of `enabled`, and whose
// Disabled group has those of `disabled`.
QPalette make_palette(const PaletteColors &enabled, const PaletteColors &disabled);

}  // namespace hpq
