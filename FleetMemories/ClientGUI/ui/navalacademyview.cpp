/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "navalacademyview.h"
#include "ui_navalacademyview.h"

#include "../clientv2.h"
#include "../equipicon.h"
#include "../networkerror.h"

using namespace std::chrono_literals;

extern std::unique_ptr<QSettings> settings;

NavalAcademyView::NavalAcademyView(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::NavalAcademyView)
{
    ui->setupUi(this);
    
    // Similar initialization to TechView but with dual panels
    // Connect signals/slots for left and right panels separately
    // Initialize slider/spinbox for amount input
    // Hide ship toggle button
}

NavalAcademyView::~NavalAcademyView() {
    delete ui;
}

// Implement other methods following TechView patterns