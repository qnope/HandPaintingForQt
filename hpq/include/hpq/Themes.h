#pragma once

#include <hpq/Export.h>
#include <hpq/Theme.h>

namespace hpq {

// The nine themes of hpq. Each is a value: an application changes the fields that it wants
// before it gives the theme to apply_theme.

// Graphite pencil on paper.
// Registers the embedded "Architects Daughter" font in QFontDatabase: make the theme once the
// QApplication exists.
// With FillStyle::Hachure, the text of a selection is graphite and not paper: it is read
// on the paper that shows between the hachures.
HPQ_EXPORT Theme sketch_theme(FillStyle fill_style = FillStyle::Solid);

// China ink laid with a brush on rice paper: black lines that swell and thin with the
// pressure of the hand, washes of diluted ink, and the red of a seal for the selection.
// Registers the embedded "Kaushan Script" font in QFontDatabase: make the theme once the
// QApplication exists.
// With FillStyle::Hachure, the text of a selection is ink and not paper: it is read on the
// paper that shows between the hachures.
HPQ_EXPORT Theme china_ink_theme(FillStyle fill_style = FillStyle::Solid);

// White and colored chalk on a black slate.
// Registers the embedded "Architects Daughter" font in QFontDatabase: make the theme once the
// QApplication exists.
// With FillStyle::Hachure, fills are hatched with chalk on the slate.
HPQ_EXPORT Theme slate_theme(FillStyle fill_style = FillStyle::Solid);

// A fountain pen on the Seyes ruled paper of a French school notebook.
// Registers the embedded "Playwrite FR Moderne" font in QFontDatabase: make the theme once the
// QApplication exists.
// The window brush is the ruled sheet; views and buttons are see-through, so that the ruling
// runs under them. A selection is a stroke of highlighter: with FillStyle::Hachure, it is
// hatched with a colored pencil instead.
HPQ_EXPORT Theme notebook_theme(FillStyle fill_style = FillStyle::Solid);

// Dry-erase markers on a white board: black for the text, blue for the frames, red for the
// selection and green for the links.
// Registers the embedded "Architects Daughter" font in QFontDatabase: make the theme once the
// QApplication exists.
// With FillStyle::Hachure, the text of a selection is black and not white: it is read on the
// board that shows between the hachures.
HPQ_EXPORT Theme velleda_theme(FillStyle fill_style = FillStyle::Solid);

// Watercolor on a grainy white paper: lines drawn with the tip of a paint brush in Payne's
// grey, and washes of diluted color with a soft edge where the pigment gathers. A wash lets
// what is under it show through: the text of a selection is dark, on a wash of blue.
// Registers the embedded "Architects Daughter" font in QFontDatabase: make the theme once the
// QApplication exists.
HPQ_EXPORT Theme watercolor_theme(FillStyle fill_style = FillStyle::Solid);

// White ink on the blue paper of an architect's blueprint, printed with a faint grid.
// Registers the embedded "Architects Daughter" font in QFontDatabase, and letters in capitals:
// make the theme once the QApplication exists.
// The grid is a texture of the Window, Base and Button brushes: their color is the blue of the paper.
// With FillStyle::Hachure, the text of a selection is ink and not paper: it is read on the
// paper that shows between the hachures.
HPQ_EXPORT Theme blueprint_theme(FillStyle fill_style = FillStyle::Solid);

// A letter written in invisible ink: a pencil draws the frames and the marks, and the text
// shows only where it is selected, under the violet light of a lamp.
// Registers the embedded "Architects Daughter" font in QFontDatabase: make the theme once the
// QApplication exists.
// The text is written with TextInk::Invisible: HighlightedText is the only color in which a
// text is drawn, and no other text role of the palette has it.
// With FillStyle::Hachure, the text of a selection is violet and not luminous: it is read on
// the paper that shows between the hachures.
HPQ_EXPORT Theme invisible_ink_theme(FillStyle fill_style = FillStyle::Solid);

// Notes written with a black pen on white paper, and marked with fluorescent highlighters:
// yellow for the selection and the text of the labels, green for the buttons and the headers,
// pink for the frames and a light pass of blue for the alternate rows.
// Registers the embedded "Architects Daughter" font in QFontDatabase: make the theme once the
// QApplication exists.
// A highlighter lets what is under it show: its colors are translucent, and the text of a
// selection stays the one of the pen.
// With FillStyle::Hachure the colors are opaque, so that the fills are hatched, and the
// selection is orange: the thin hachures of the yellow highlighter would not show on the paper.
// The marker of the labels is a line, not a fill: it stays yellow.
HPQ_EXPORT Theme highlighter_theme(FillStyle fill_style = FillStyle::Solid);

}  // namespace hpq
