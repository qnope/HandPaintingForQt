#include <gtest/gtest.h>

#include <QDate>
#include <QMetaType>
#include <QStandardItemModel>
#include <memory>
#include <type_traits>

#include "TableModelFactory.h"

namespace {
const QStringList Headers = {"ID",     "First name", "Last name", "Department", "City",
                              "Age",    "Salary",     "Hire date", "Active",     "Email"};

int column_of(const QString &header) { return Headers.indexOf(header); }

auto make_model() { return hpq::example::create_table_model(); }
} // namespace

static_assert(std::is_same_v<decltype(hpq::example::create_table_model()), std::unique_ptr<QStandardItemModel>>,
              "create_table_model must return a unique_ptr");

TEST(TableModelFactory, HasExpectedShapeAndHeaders)
{
    auto model = make_model();
    ASSERT_EQ(model->rowCount(), 300);
    ASSERT_EQ(model->columnCount(), 10);
    for (int c = 0; c < Headers.size(); ++c)
        EXPECT_EQ(model->headerData(c, Qt::Horizontal).toString(), Headers[c]);
}

TEST(TableModelFactory, IdColumnIsRowOrder)
{
    auto model = make_model();
    for (int r = 0; r < model->rowCount(); ++r)
        EXPECT_EQ(model->item(r, column_of("ID"))->data(Qt::DisplayRole).toInt(), r + 1);
}

TEST(TableModelFactory, AgeAndSalaryAreNumbers)
{
    auto model = make_model();
    for (int r = 0; r < model->rowCount(); ++r) {
        const auto age = model->item(r, column_of("Age"))->data(Qt::DisplayRole).typeId();
        const auto salary = model->item(r, column_of("Salary"))->data(Qt::DisplayRole).typeId();
        EXPECT_TRUE(age == QMetaType::Int || age == QMetaType::Double);
        EXPECT_TRUE(salary == QMetaType::Int || salary == QMetaType::Double);
    }
}

TEST(TableModelFactory, HireDateIsValidDate)
{
    auto model = make_model();
    for (int r = 0; r < model->rowCount(); ++r) {
        const QVariant v = model->item(r, column_of("Hire date"))->data(Qt::DisplayRole);
        EXPECT_EQ(v.typeId(), QMetaType::QDate);
        EXPECT_TRUE(v.toDate().isValid());
    }
}

TEST(TableModelFactory, ActiveItemsAreCheckable)
{
    auto model = make_model();
    for (int r = 0; r < model->rowCount(); ++r)
        EXPECT_TRUE(model->item(r, column_of("Active"))->isCheckable());
}

TEST(TableModelFactory, NoCellIsEmpty)
{
    auto model = make_model();
    for (int r = 0; r < model->rowCount(); ++r)
        for (int c = 0; c < model->columnCount(); ++c) {
            EXPECT_FALSE(model->item(r, c)->data(Qt::DisplayRole).toString().isEmpty())
                << r << "," << c;
        }
}

TEST(TableModelFactory, IsDeterministic)
{
    auto a = make_model();
    auto b = make_model();
    for (int r = 0; r < a->rowCount(); ++r)
        for (int c = 0; c < a->columnCount(); ++c) {
            EXPECT_EQ(a->item(r, c)->data(Qt::DisplayRole), b->item(r, c)->data(Qt::DisplayRole));
            EXPECT_EQ(a->item(r, c)->data(Qt::CheckStateRole),
                      b->item(r, c)->data(Qt::CheckStateRole));
        }
}
