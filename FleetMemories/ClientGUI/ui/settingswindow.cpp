#include "settingswindow.h"
#include "ui_settingswindow.h"
#include <QLocale>
#include <QSettings>
#include <QScrollBar>
#include "../../Protocol/kp.h"

extern std::unique_ptr<QSettings> settings;

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    for(auto &lang: *KP::supportedLangs) {
        QLocale locale = QLocale(lang);
        ui->languageBox->addItem(
            QStringLiteral("%1(%2)").arg(
                locale.nativeLanguageName(),
                locale.nativeTerritoryName()));
        if(lang == settings->value("client/language").toString()) {
            ui->languageBox->setCurrentIndex(ui->languageBox->count() - 1);
        }
    }
    ui->settingsTable->setColumnCount(2);
    ui->settingsTable->setHorizontalHeaderLabels(
        {
            //% "Key"
            qtTrId("settings-key"),
            //% "Value"
            qtTrId("settings-value")
        });
    ui->settingsTable->verticalHeader()->setVisible(false);
    connect(ui->settingsTable, &QTableWidget::cellChanged,
            this, &SettingsWindow::handleCellChange);
    connect(ui->languageBox, &QComboBox::currentIndexChanged,
            this, [this](){
                settings->setValue("client/language",
                                   (*KP::supportedLangs)[ui->languageBox->currentIndex()]);
            });
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    auto keys = settings->allKeys();
    keys.removeAll("client/equipdbcache");
    keys.removeAll("client/shipdbcache");
    keys.removeAll("client/mapdbcache");
    ui->settingsTable->setRowCount(keys.size() + 1);
    int i = 0;
    for(auto &key : keys) {
        QTableWidgetItem *newItem = new QTableWidgetItem(key);
        ui->settingsTable->setItem(i, 0, newItem);
        QTableWidgetItem *newItem2 = new QTableWidgetItem(settings->value(key).toString());
        ui->settingsTable->setItem(i, 1, newItem2);
        ++i;
    }
    ui->settingsTable->setItem(i, 0, new QTableWidgetItem());
    ui->settingsTable->setItem(i, 1, new QTableWidgetItem());

    QHeaderView *hH = ui->settingsTable->horizontalHeader();
    hH->setSectionResizeMode(QHeaderView::ResizeToContents);
    int outerTableWidth = hH->size().width();
    int innerTableWidth = 0;
    for(int i = 0; i < 2; ++i)
        innerTableWidth += hH->sectionSize(hH->logicalIndex(i));
    innerTableWidth += ui->settingsTable->verticalScrollBar()->minimumWidth()+14;
    hH->setSectionResizeMode(QHeaderView::Interactive);
    hH->resizeSection(hH->logicalIndex(1),
                      hH->sectionSize(hH->logicalIndex(1))
                          + outerTableWidth - innerTableWidth);
}

void SettingsWindow::handleCellChange(int row, int column) {
    if(row == ui->settingsTable->rowCount() - 1) {
        QString text = ui->settingsTable->item(row, column)->text();
        if(!text.isEmpty()) {
            ui->settingsTable->setRowCount(ui->settingsTable->rowCount() + 1);
            int i = ui->settingsTable->rowCount() - 1;
            ui->settingsTable->setItem(i, 0, new QTableWidgetItem());
            ui->settingsTable->setItem(i, 1, new QTableWidgetItem());
        }
    }
    if(column != 1) {
        return;
    }
    if(!ui->settingsTable->item(row, 0)->text().isEmpty()) {
        if(!ui->settingsTable->item(row, 1)->text().isEmpty()) {
            settings->setValue(ui->settingsTable->item(row, 0)->text(),
                               ui->settingsTable->item(row, 1)->text());
        }
        else {
            settings->remove(ui->settingsTable->item(row, 0)->text());
        }
    }
}
