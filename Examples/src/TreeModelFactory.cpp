#include "TreeModelFactory.h"

#include <QList>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <memory>

namespace hpq::example {

namespace {
constexpr int DepartmentCount = 20;
constexpr int EmployeesPerDepartment = 15;

const QStringList Headers = {"Name",   "Kind",   "Role", "Department", "City",
                              "Budget", "Status", "Level", "Email",     "Phone"};
const QStringList BaseDepartments = {"Engineering", "Sales",   "Marketing", "Finance", "Support",
                                      "HR",          "Legal",   "Design",    "Research", "Logistics"};
const QStringList FirstNames = {"Alice", "Bob",   "Clara", "David", "Emma",
                                 "Felix", "Grace", "Hugo",  "Iris",  "Jules"};
const QStringList LastNames = {"Martin", "Bernard", "Dubois", "Thomas", "Robert", "Richard", "Petit"};
const QStringList Roles = {"Manager", "Developer", "Analyst", "Specialist", "Assistant"};
const QStringList Cities = {"Paris", "Lyon", "Lille", "Nantes", "Toulouse", "Bordeaux"};
const QStringList Statuses = {"Active", "On leave", "Remote"};

QString department_name(int index)
{
    const QString base = BaseDepartments[index % BaseDepartments.size()];
    return index < BaseDepartments.size() ? base : base + " " + QString::number(index / BaseDepartments.size() + 1);
}

QList<QStandardItem *> make_row(const QStringList &cells)
{
    QList<QStandardItem *> row;
    for (const QString &text : cells)
        row.append(new QStandardItem(text));
    return row;
}
} // namespace

std::unique_ptr<QStandardItemModel> create_tree_model()
{
    auto model = std::make_unique<QStandardItemModel>(0, Headers.size());
    model->setHorizontalHeaderLabels(Headers);

    for (int d = 0; d < DepartmentCount; ++d) {
        const QString department = department_name(d);
        const QString city = Cities[d % Cities.size()];
        auto parent_row = make_row({department, "Department", "Division", department, city,
                                  QString::number((d + 1) * 100000), "Active", "0",
                                  department.toLower().remove(' ') + "@example.com",
                                  QString("+33 1 00 00 %1").arg(d, 2, 10, QLatin1Char('0'))});
        for (int e = 0; e < EmployeesPerDepartment; ++e) {
            const int n = d * EmployeesPerDepartment + e;
            const QString first = FirstNames[n % FirstNames.size()];
            const QString last = LastNames[n % LastNames.size()];
            parent_row.first()->appendRow(
                make_row({first + " " + last, "Employee", Roles[n % Roles.size()], department,
                         Cities[n % Cities.size()], QString::number(30000 + (n * 997) % 50000),
                         Statuses[n % Statuses.size()], QString::number(1 + n % 5),
                         first.toLower() + "." + last.toLower() + "@example.com",
                         QString("+33 6 00 %1 %2").arg(d, 2, 10, QLatin1Char('0')).arg(e, 2, 10, QLatin1Char('0'))}));
        }
        model->appendRow(parent_row);
    }
    return model;
}

} // namespace hpq::example
