#pragma once

#include <QObject>
#include <functional>

#include "FwdDecls.h"

namespace hpq::example {

// Asks for the virtual keyboard when a double tap opens the editor of an item of a view.
//
// Qt asks for the keyboard when a text widget is tapped. The editor of an item is not tapped: it
// gets the focus on the second tap of a double tap, and that tap goes to the view. The request
// is made when this tap is released, as Qt does for a text widget: a browser on a phone opens the
// keyboard at the end of a touch.
//
// Owned by the view.
class ItemEditorInputPanel : public QObject
{
public:
    using Request = std::function<void()>;

    // `request` asks for the keyboard; by default it asks the input method of the application.
    explicit ItemEditorInputPanel(QAbstractItemView &view, Request request = show_input_panel);

    static void show_input_panel();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Request m_request;
};

} // namespace hpq::example
