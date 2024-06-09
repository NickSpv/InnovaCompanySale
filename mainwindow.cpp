#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    database = new Database();
    buttonDelegate = new ButtonDelegate();

    connect(buttonDelegate, &ButtonDelegate::buttonClicked, this, &MainWindow::on_delegate_button_clicked);

    // Создаем список QString
    QStringList stringList;
    stringList << "Прибыль от заказов на производство" << "Прибыль от заказов на разработку" << "Заказы с фабриками и датами";

    // Создаем и настраиваем QComboBox
    QComboBox* comboBox = ui->comboBox;
    for(const QString& str : stringList) {
        comboBox->addItem(str);
    }
    comboBox->setCurrentIndex(-1);

    stringList.clear();
    stringList << "bill_view" << "production_order_view" << "development_order_view" << "customer_view" << "application_view";

    comboBox = ui->comboBox_2;
    for(const QString& str : stringList) {
        comboBox->addItem(str);
    }
    comboBox->setCurrentIndex(-1);

    this->ui->tableView->setSortingEnabled(true);
    //this->ui->tableView_2->setSortingEnabled(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_comboBox_currentTextChanged(const QString &arg1)
{
    while (QLayoutItem *item = this->ui->formLayout_2->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            delete widget;
        }
        delete item;
    }
    if (this->ui->tableView->model())
        delete this->ui->tableView->model();
    QSqlTableModel *model = database->getModelView(arg1);
    this->ui->tableView->setModel(model);
    QMap<QString, QString>* filters = new QMap<QString, QString>();
    for (int i = 0; i < model->columnCount(); ++i) {
        QLabel *label = new QLabel(model->headerData(i, Qt::Horizontal).toString());
        CustomComboBox *comboBox = new CustomComboBox();
        comboBox->setModel(model);
        comboBox->setMyIndex(i);

        QStringList values;
        for (int j = 0; j < model->rowCount(); ++j) {
            QString value = model->index(j, i).data().toString();
            values << value;
        }
        values.removeDuplicates();
        comboBox->blockSignals(true);
        comboBox->addItems(values);
        comboBox->clearEditText();
        comboBox->blockSignals(false);

        filters->insert(label->text(), "");

        //connect(comboBox, &QComboBox::currentTextChanged, comboBox, &CustomComboBox::updateComboBoxItems);
        connect(comboBox, &QComboBox::currentTextChanged, model, [model, label, filters](const QString &text){
            filters->insert(label->text(), text);
            QString filter = "";
            QMap<QString, QString>::const_iterator it;
            for (it = filters->constBegin(); it != filters->constEnd(); ++it) {
                filter += "LOWER(\"" + it.key() + "\"::text) LIKE LOWER('%" + it.value() + "%')";
                if (it + 1 != filters->constEnd())
                    filter += " AND ";
            }
            std::cout << filter.toStdString() << std::endl;
            model->setFilter(filter);
            model->select();
        });

        ui->formLayout_2->addRow(label, comboBox);
    }
    this->ui->tableView->resizeColumnsToContents();

    for (int i = 0; i < this->ui->tableView->model()->columnCount(); ++i) {
        this->ui->tableView->setColumnWidth(i, this->ui->tableView->columnWidth(i) + 20);
    }
}


void MainWindow::on_pushButton_clicked()
{
    QTextDocument *doc = new QTextDocument;
    doc->setDocumentMargin(10);
    QTextCursor cursor(doc);

    QString htmlHeader = "<!DOCTYPE html>"
                         "<html>"
                         "<head>"
                         "<title>innova.nickspv.ru</title>"
                         "</head>"
                         "<body>"
                         "<h1 style=\"text-align:center\">Отчет</h1>"
                         "<p style=\"text-align:center\">" +
                         static_cast<QSqlTableModel*>(this->ui->tableView->model())->tableName() +
                         ".</p>";
/*
    for (int i = 0; i < this->ui->formLayout_2->rowCount(); ++i) {
        QLayoutItem *labelItem = this->ui->formLayout_2->itemAt(i, QFormLayout::LabelRole);
        QLabel *label = qobject_cast<QLabel *>(labelItem->widget());

        QLayoutItem *fieldItem = this->ui->formLayout_2->itemAt(i, QFormLayout::FieldRole);
        QComboBox *comboBox = qobject_cast<QComboBox *>(fieldItem->widget());

        if (label and comboBox and not comboBox->currentText().isEmpty()) {
            QString labelText = label->text();
            QString comboBoxText = comboBox->currentText();
            htmlHeader += "<p style=\"text-align:center\"> " + labelText +
                          " содержит " + comboBoxText + ".</p>";
        }
    }*/
    htmlHeader += "<p>&nbsp;</p>"
                  "<p>Отчёт составил: ___________________</p>"
                  "<p>Отчёт принял:&nbsp; &nbsp; ___________________</p>"
                  "<p style=\"text-align:center\">Дата составления отчёта: " + QDate::currentDate().toString() + "</p>"
                                                      "<p>&nbsp;</p>"
                                                      "</body>"
                                                      "</html>";
    std::cout << htmlHeader.toStdString() << std::endl;
    cursor.insertHtml(htmlHeader);
    cursor.movePosition(QTextCursor::NextBlock);

    QTextTable *tableT = cursor.insertTable(this->ui->tableView->model()->rowCount() + 1, this->ui->tableView->model()->columnCount());
    QTextTableCell headerCell;
    for (int i = 0; i < this->ui->tableView->model()->columnCount(); i++) {
        headerCell = tableT->cellAt(0, i);
        QTextCursor headerCellCursor = headerCell.firstCursorPosition();
        headerCellCursor.insertText(this->ui->tableView->model()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
    }

    for (int i = 0; i < this->ui->tableView->model()->rowCount(); i++){
        for (int j = 0; j < this->ui->tableView->model()->columnCount(); j++) {
            QTextTableCell cell = tableT->cellAt(i + 1, j);
            QTextCursor cellCursor = cell.firstCursorPosition();
            cellCursor.insertText(this->ui->tableView->model()->data(this->ui->tableView->model()->index(i, j), 0).toString());
        }
    }

    cursor.movePosition(QTextCursor::End);
    cursor.insertBlock();

    QString filePath = QFileDialog::getSaveFileName(nullptr, "Save PDF", "", "PDF Files (*.pdf)");

    //Print to PDF
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    doc->print(&printer);
}



void MainWindow::on_comboBox_2_currentTextChanged(const QString &arg1)
{
    if (this->ui->tableView_2->model())
        delete this->ui->tableView_2->model();
    QSqlTableModel *model = database->getModelView(arg1);
    std::cout << model->columnCount() << std::endl;

    //this->ui->tableView_2->setSortingEnabled(false);
    if (model->columnCount() > 0) {
        model->removeColumn(0);
        model->insertColumn(model->columnCount());
        model->setHeaderData(model->columnCount() - 1, Qt::Horizontal, "Удаление", Qt::DisplayRole);
        std::cout << model->columnCount() << std::endl;

        model->insertColumn(model->columnCount());
        model->setHeaderData(model->columnCount() - 1, Qt::Horizontal, "Изменение", Qt::DisplayRole);
        std::cout << model->columnCount() << std::endl;

        this->ui->tableView_2->setItemDelegateForColumn(model->columnCount()-1, buttonDelegate); // Устанавливаем делегат для столбца 2
        this->ui->tableView_2->setItemDelegateForColumn(model->columnCount()-2, buttonDelegate); // Устанавливаем делегат для столбца 3
    }

    this->ui->tableView_2->setModel(model);
    this->ui->tableView_2->resizeColumnsToContents();

    for (int i = 0; i < this->ui->tableView_2->model()->columnCount(); ++i) {
        this->ui->tableView_2->setColumnWidth(i, this->ui->tableView_2->columnWidth(i) + 20);
    }
    //this->ui->tableView_2->setSortingEnabled(true);
}


void MainWindow::on_delegate_button_clicked(const QModelIndex &index)
{
    std::cout << "Button clicked in row:" << index.row() << " column:" << index.column();
    int count_columns = this->ui->tableView_2->model()->columnCount();
    QString view_name =static_cast<QSqlTableModel*>(
                            this->ui->tableView_2->model()
                            )->tableName();
    std::cout << view_name.toStdString() << std::endl;
    QSqlTableModel *model = database->getModelView(view_name);
    //this->ui->tableView_2->setModel(model);
    QString table_name = view_name.left(view_name.lastIndexOf("_"));
    QSqlTableModel *main_model = database->getModelView(table_name);

    if (index.column() == count_columns - 1) {
        QString sql_query = "delete from " + table_name + " where " +
                            model->headerData(0, Qt::Horizontal).toString() +
                            " = '" + model->index(index.row(), 0).data().toString() + "'";
        std::cout << sql_query.toStdString() << std::endl;
        this->database->database.exec(sql_query);
    } else if (index.column() == count_columns - 2) {
        QDialog dialog(this);
        QFormLayout form(&dialog);
        form.addRow(new QLabel("Изменение записи"));

        QMap<int, QSqlTableModel*> models;
        for (int i = 1; i < main_model->columnCount(); i++) {
            QMap<int, int> nums;
            QString column_name = main_model->headerData(i, Qt::Horizontal).toString();
            if (column_name.contains("_id")) {
                QSqlTableModel *tmp_model = database->getModelView(column_name + "_view");
                std::cout << column_name.toStdString() << std::endl;
                models[i] = tmp_model;
                for (int j = 0; j < tmp_model->rowCount(); j++) {
                    QModelIndex ind = main_model->index(j, i);
                    //main_model->setData(ind, tmp_model->data(tmp_model->index(j, 1)));
                }

                QString label = model->headerData(i, Qt::Horizontal).toString();
                CustomComboBox *comboBox = new CustomComboBox(&dialog);
                QStringList values;
                for (int j = 0; j < tmp_model->rowCount(); ++j) {
                    nums[tmp_model->index(j, 0).data().toInt()] = j;
                    QString value = tmp_model->index(j, 1).data().toString();
                    values << value;
                }
                //comboBox->blockSignals(true);
                comboBox->addItems(values);
                QString quer = "select " + tmp_model->headerData(0, Qt::Horizontal).toString() + " from " + table_name
                               + " where " + model->headerData(0, Qt::Horizontal).toString() + " = " +
                               model->index(index.row(), 0).data().toString();
                std::cout << quer.toStdString() << std::endl;
                this->database->sqlquery.exec(quer);
                this->database->sqlquery.next();
                int vind = this->database->sqlquery.value(0).toInt();
                //int vind = 1;
                std::cout << "fsdfsdf\n";
                std::cout << vind << std::endl;
                std::cout << nums[vind] << std::endl;
                comboBox->setCurrentIndex(vind != 0 ? nums[vind] : -1);
                //std::cout << "fjsdf " << main_model->index(index.row(), 1).data().toInt() << std::endl;
                //comboBox->blockSignals(false);
                form.addRow(label, comboBox);
            } else {
                models[i] = nullptr;
                QString label = model->headerData(i, Qt::Horizontal).toString();
                QLineEdit *lineEdit = new QLineEdit(&dialog);
                lineEdit->setText(model->index(index.row(), i).data().toString());
                form.addRow(label, lineEdit);
            }
        }

        QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                   Qt::Horizontal, &dialog);
        form.addRow(&buttonBox);
        QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
        QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

        if (dialog.exec() == QDialog::Accepted) {
            QString sql_query = "update " + table_name + " set ";

            for (int i = 0; i < form.rowCount(); ++i) {
                QLayoutItem *fieldItem = form.itemAt(i, QFormLayout::FieldRole);
                QComboBox *comboBox;
                QLineEdit *lineEdit;
                try {
                    comboBox = dynamic_cast<QComboBox *>(fieldItem->widget());
                } catch(...) {
                    comboBox = nullptr;
                }
                try {
                    lineEdit = dynamic_cast<QLineEdit *>(fieldItem->widget());
                } catch(...) {
                    lineEdit = nullptr;
                }
                if (comboBox) {
                    int index_i = comboBox->currentIndex();
                    sql_query += main_model->headerData(i, Qt::Horizontal).toString() +
                                 " = '" + models[i]->index(index_i, 0).data().toString() + "', ";
                } else if (lineEdit) {
                    sql_query += main_model->headerData(i, Qt::Horizontal).toString() +
                                 " = '" + lineEdit->text() + "', ";
                }
            }
            sql_query.chop(2);
            sql_query += " where " + model->headerData(0, Qt::Horizontal).toString() +
                         " = '" + model->index(index.row(), 0).data().toString() + "'";
            sql_query = sql_query.replace("''", "NULL");
            std::cout << std::endl << sql_query.toStdString() << std::endl;
            this->database->database.exec(sql_query);
        } else return;
    }
    QString arg1 = ui->comboBox_2->currentText();
    on_comboBox_2_currentTextChanged(arg1);
}


void MainWindow::on_pushButton_2_clicked()
{
    QString view_name =static_cast<QSqlTableModel*>(
                            this->ui->tableView_2->model()
                            )->tableName();
    QSqlTableModel *model = database->getModelView(view_name);
    std::cout << view_name.toStdString() << std::endl;
    QString table_name = view_name.left(view_name.lastIndexOf("_"));
    QSqlTableModel *main_model = database->getModelView(table_name);

    QDialog dialog(this);
    QFormLayout form(&dialog);
    form.addRow(new QLabel("Добавление записи"));

    QMap<int, QSqlTableModel*> models;
    for (int i = 1; i < main_model->columnCount(); i++) {
        QString column_name = main_model->headerData(i, Qt::Horizontal).toString();
        if (column_name.contains("_id")) {
            QSqlTableModel *tmp_model = database->getModelView(column_name + "_view");
            models[i] = tmp_model;
            for (int j = 0; j < tmp_model->rowCount(); j++) {
                QModelIndex ind = main_model->index(j, i);
                //main_model->setData(ind, tmp_model->data(tmp_model->index(j, 1)));
            }

            QString label = model->headerData(i, Qt::Horizontal).toString();
            CustomComboBox *comboBox = new CustomComboBox(&dialog);
            QStringList values;
            for (int j = 0; j < tmp_model->rowCount(); ++j) {
                QString value = tmp_model->index(j, 1).data().toString();
                values << value;
            }
            //comboBox->blockSignals(true);
            comboBox->addItems(values);
            comboBox->setCurrentIndex(-1);
            //std::cout << "fjsdf " << main_model->index(index.row(), 1).data().toInt() << std::endl;
            //comboBox->blockSignals(false);
            form.addRow(label, comboBox);
        } else {
            models[i] = nullptr;
            QString label = model->headerData(i, Qt::Horizontal).toString();
            QLineEdit *lineEdit = new QLineEdit(&dialog);
            lineEdit->setText("");
            form.addRow(label, lineEdit);
        }
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    if (dialog.exec() == QDialog::Accepted) {
        QString sql_query_1 = "insert into " + table_name + " (";
        QString sql_query_2 = " values ('";

        for (int i = 0; i < form.rowCount(); ++i) {
            QLayoutItem *fieldItem = form.itemAt(i, QFormLayout::FieldRole);
            QComboBox *comboBox;
            QLineEdit *lineEdit;
            try {
                comboBox = dynamic_cast<QComboBox *>(fieldItem->widget());
            } catch(...) {
                comboBox = nullptr;
            }
            try {
                lineEdit = dynamic_cast<QLineEdit *>(fieldItem->widget());
            } catch(...) {
                lineEdit = nullptr;
            }
            if (comboBox) {
                int index_i = comboBox->currentIndex();
                sql_query_1 += main_model->headerData(i, Qt::Horizontal).toString() + ", ";
                sql_query_2 += models[i]->index(index_i, 0).data().toString() + "', '";
            } else if (lineEdit) {
                sql_query_1 += main_model->headerData(i, Qt::Horizontal).toString() + ", ";
                sql_query_2 += lineEdit->text() + "', '";
            }
        }
        sql_query_1.chop(2);
        sql_query_2.chop(3);
        QString sql_query = sql_query_1 + ")" + sql_query_2 + ")";
        sql_query = sql_query.replace("''", "NULL");
        std::cout << std::endl << sql_query.toStdString() << std::endl;
        this->database->database.exec(sql_query);
    } else return;
    QString arg1 = ui->comboBox_2->currentText();
    on_comboBox_2_currentTextChanged(arg1);
}
