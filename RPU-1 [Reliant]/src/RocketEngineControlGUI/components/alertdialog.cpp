#include "alertdialog.h"
#include "ui_alertdialog.h"


AlertDialog::AlertDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AlertDialog)
{
    ui->setupUi(this);
    connect(this->ui->OkButtonBox, &QDialogButtonBox::clicked, this, &AlertDialog::close);
}

AlertDialog::~AlertDialog()
{
    delete ui;
}

void AlertDialog::setAlertTitle(QString title)
{
    this->ui->TitleLabel->setText(title);
}

void AlertDialog::setAlertDescription(QString description)
{
    this->ui->DescriptionLabel->setText(description);
}

