#include <gtest/gtest.h>

#include <QStandardItemModel>
#include <memory>
#include <type_traits>

#include "TreeModelFactory.h"

namespace {
constexpr int DepartmentColumn = 3;

auto make_model() { return hpq::example::create_tree_model(); }
} // namespace

static_assert(std::is_same_v<decltype(hpq::example::create_tree_model()), std::unique_ptr<QStandardItemModel>>,
              "create_tree_model must return a unique_ptr");

TEST(TreeModelFactory, HasExpectedShapeAndHeaders)
{
    auto model = make_model();
    ASSERT_EQ(model->rowCount(), 20);
    ASSERT_EQ(model->columnCount(), 10);
    for (int c = 0; c < 10; ++c)
        EXPECT_FALSE(model->headerData(c, Qt::Horizontal).toString().isEmpty());
}

TEST(TreeModelFactory, EachParentHasFifteenChildrenWithTenColumns)
{
    auto model = make_model();
    for (int r = 0; r < model->rowCount(); ++r) {
        const auto *parent = model->item(r, 0);
        ASSERT_EQ(parent->rowCount(), 15);
        EXPECT_EQ(parent->columnCount(), 10);
    }
}

TEST(TreeModelFactory, ChildrenHaveNoChildren)
{
    auto model = make_model();
    for (int r = 0; r < model->rowCount(); ++r) {
        const auto *parent = model->item(r, 0);
        for (int i = 0; i < parent->rowCount(); ++i)
            for (int c = 0; c < 10; ++c)
                EXPECT_FALSE(parent->child(i, c)->hasChildren());
    }
}

TEST(TreeModelFactory, NoCellIsEmpty)
{
    auto model = make_model();
    for (int r = 0; r < model->rowCount(); ++r) {
        for (int c = 0; c < 10; ++c) {
            ASSERT_NE(model->item(r, c), nullptr);
            EXPECT_TRUE(model->item(r, c)->data(Qt::DisplayRole).isValid());
            EXPECT_FALSE(model->item(r, c)->data(Qt::DisplayRole).toString().isEmpty());
        }
        const auto *parent = model->item(r, 0);
        for (int i = 0; i < parent->rowCount(); ++i)
            for (int c = 0; c < 10; ++c) {
                ASSERT_NE(parent->child(i, c), nullptr);
                EXPECT_FALSE(parent->child(i, c)->data(Qt::DisplayRole).toString().isEmpty());
            }
    }
}

TEST(TreeModelFactory, ChildDepartmentEqualsParentName)
{
    auto model = make_model();
    for (int r = 0; r < model->rowCount(); ++r) {
        const auto *parent = model->item(r, 0);
        for (int i = 0; i < parent->rowCount(); ++i)
            EXPECT_EQ(parent->child(i, DepartmentColumn)->text(), parent->text());
    }
}
