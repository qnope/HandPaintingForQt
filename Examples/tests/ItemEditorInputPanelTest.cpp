#include "ItemEditorInputPanel.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <gtest/gtest.h>

namespace {
constexpr int ActivationTimeoutMs = 5000;

// A table with one cell, in the active window: an editor gets the focus only there.
// The mouse events are the ones Qt sends to the viewport for clicks on the cell.
class ItemEditorInputPanelTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_model.setItem(0, 0, new QStandardItem("Text"));
        m_view.setModel(&m_model);
        new hpq::example::ItemEditorInputPanel(m_view, [this] { ++m_request_count; });
        m_view.resize(300, 200);
        m_view.show();
        m_view.activateWindow();
        QElapsedTimer timer;
        timer.start();
        while (!m_view.isActiveWindow() && timer.elapsed() < ActivationTimeoutMs)
            QApplication::processEvents(QEventLoop::AllEvents, 50);
        ASSERT_TRUE(m_view.isActiveWindow());
    }

    void send(QEvent::Type type, Qt::MouseButton button = Qt::LeftButton)
    {
        const QPoint position = m_view.visualRect(m_model.index(0, 0)).center();
        const Qt::MouseButtons buttons = type == QEvent::MouseButtonRelease ? Qt::NoButton : button;
        QMouseEvent event(type, position, m_view.viewport()->mapToGlobal(position), button, buttons,
                          Qt::NoModifier);
        QApplication::sendEvent(m_view.viewport(), &event);
    }

    void click()
    {
        send(QEvent::MouseButtonPress);
        send(QEvent::MouseButtonRelease);
    }

    // The second press of a double click comes as a double click event.
    void double_click()
    {
        click();
        send(QEvent::MouseButtonDblClick);
        send(QEvent::MouseButtonRelease);
    }

    bool editor_has_focus() const
    {
        const QWidget *focus_widget = QApplication::focusWidget();
        return focus_widget && focus_widget != m_view.viewport() && m_view.viewport()->isAncestorOf(focus_widget);
    }

    QStandardItemModel m_model;
    QTableView m_view;
    int m_request_count = 0;
};
} // namespace

TEST_F(ItemEditorInputPanelTest, AsksForTheKeyboardWhenADoubleClickOpensAnEditor)
{
    double_click();
    ASSERT_TRUE(editor_has_focus());
    EXPECT_EQ(m_request_count, 1);
}

TEST_F(ItemEditorInputPanelTest, DoesNotAskForTheKeyboardOnAClick)
{
    click();
    EXPECT_FALSE(editor_has_focus());
    EXPECT_EQ(m_request_count, 0);
}

TEST_F(ItemEditorInputPanelTest, DoesNotAskForTheKeyboardWhenNoEditorOpens)
{
    m_view.setEditTriggers(QAbstractItemView::NoEditTriggers);
    double_click();
    EXPECT_FALSE(editor_has_focus());
    EXPECT_EQ(m_request_count, 0);
}

TEST_F(ItemEditorInputPanelTest, AsksOnlyWhenTheLeftButtonIsReleased)
{
    double_click();
    ASSERT_EQ(m_request_count, 1);
    send(QEvent::MouseButtonRelease, Qt::RightButton);
    EXPECT_EQ(m_request_count, 1);
}
