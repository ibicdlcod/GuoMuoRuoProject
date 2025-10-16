#include "choosehomeport.h"
#include "ui_choosehomeport.h"
#include <QJsonArray>

ChooseHomePort::ChooseHomePort(QWidget *parent, const QJsonObject input)
    : QDialog(parent)
    , ui(new Ui::ChooseHomePort)
{
    ui->setupUi(this);
    connect(this, &QDialog::finished,
            this, &ChooseHomePort::finishChoice);
    if(input.contains("choices")) {
        ui->buttonJPN->setCheckable(false);
        ui->buttonGER->setCheckable(false);
        ui->buttonITA->setCheckable(false);
        ui->buttonUSA->setCheckable(false);
        ui->buttonGBR->setCheckable(false);
        ui->buttonFRA->setCheckable(false);
        ui->buttonSOV->setCheckable(false);
        ui->buttonOCE->setCheckable(false);
        for(const auto &availablePort: input["choices"].toArray()) {
            switch(availablePort.toInt()) {
            case KP::Japanese: ui->buttonJPN->setCheckable(true); break;
            case KP::German:   ui->buttonGER->setCheckable(true); break;
            case KP::Italian:  ui->buttonITA->setCheckable(true); break;
            case KP::American: ui->buttonUSA->setCheckable(true); break;
            case KP::British:  ui->buttonGBR->setCheckable(true); break;
            case KP::French:   ui->buttonFRA->setCheckable(true); break;
            case KP::Soviet:   ui->buttonSOV->setCheckable(true); break;
            case KP::Oceanian: ui->buttonOCE->setCheckable(true); break;
            default: break;
            }
        }
    }
}

ChooseHomePort::~ChooseHomePort()
{
    delete ui;
}

void ChooseHomePort::finishChoice(int status) {
    if(status == QDialog::Rejected) {
        return;
    }
    else if(ui->buttonJPN->isChecked()) {
        emit portChosen(KP::Japanese);
    }
    else if(ui->buttonGER->isChecked()) {
        emit portChosen(KP::German);
    }
    else if(ui->buttonITA->isChecked()) {
        emit portChosen(KP::Italian);
    }
    else if(ui->buttonUSA->isChecked()) {
        emit portChosen(KP::American);
    }
    else if(ui->buttonGBR->isChecked()) {
        emit portChosen(KP::British);
    }
    else if(ui->buttonFRA->isChecked()) {
        emit portChosen(KP::French);
    }
    else if(ui->buttonSOV->isChecked()) {
        emit portChosen(KP::Soviet);
    }
    else if(ui->buttonOCE->isChecked()) {
        emit portChosen(KP::Oceanian);
    }
}
