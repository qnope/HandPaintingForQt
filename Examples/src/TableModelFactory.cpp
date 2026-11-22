#include "TableModelFactory.h"

#include <QDate>
#include <QList>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <memory>

namespace hpq::example {

namespace {
constexpr int RowCount = 300;

const QStringList Headers = {"ID",     "First name", "Last name", "Department", "City",
                              "Age",    "Salary",     "Hire date", "Active",     "Email"};
const QStringList FirstNames = {"Alice", "Bob",   "Clara", "David", "Emma",
                                 "Felix", "Grace", "Hugo",  "Iris",  "Jules"};
const QStringList LastNames = {"Martin", "Bernard", "Dubois", "Thomas", "Robert", "Richard", "Petit"};
const QStringList Departments = {"Engineering", "Sales", "Marketing", "Finance", "Support", "HR"};
const QStringList Cities = {"Paris", "Lyon", "Lille", "Nantes", "Toulouse", "Bordeaux"};

QStandardItem *number_item(const QVariant &value)
{
    auto *item = new QStandardItem;
    item->setData(value, Qt::DisplayRole);
    return item;
}
} // namespace

std::unique_ptr<QStandardItemModel> create_table_model()
{
    auto model = std::make_unique<QStandardItemModel>(0, Headers.size());
    model->setHorizontalHeaderLabels(Headers);

    for (int i = 0; i < RowCount; ++i) {
        const QString first = FirstNames[i % FirstNames.size()];
        const QString last = LastNames[(i / 3) % LastNames.size()];

        const bool is_active = i % 4 != 0;
        auto *active = new QStandardItem(is_active ? "Yes" : "No");
        active->setCheckable(true);
        active->setCheckState(is_active ? Qt::Checked : Qt::Unchecked);

        QList<QStandardItem *> row;
        row << number_item(i + 1) << new QStandardItem(first) << new QStandardItem(last)
            << new QStandardItem(Departments[i % Departments.size()])
            << new QStandardItem(Cities[(i / 2) % Cities.size()]) << number_item(22 + (i * 7) % 43)
            << number_item(30000 + (i * 1237) % 70000)
            << number_item(QDate(2010, 1, 1).addDays((i * 37) % 5000)) << active
            << new QStandardItem(QString("%1.%2.%3@example.com").arg(first.toLower(), last.toLower()).arg(i + 1));
        model->appendRow(row);
    }
    return model;
}

} // namespace hpq::example
