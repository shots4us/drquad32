#include "ConsoleWindow.h"
#include "ui_ConsoleWindow.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QDebug>
#include <QTime>
#include <QString>
#include <QTextStream>
#include <QMessageBox>
#include <QScrollBar>

#include "MainWindow.h"
#include "../../Bootloader/msg_structs.h"


static const QBrush ansiPalette[] = {
    QBrush(QColor("#222")), QBrush(QColor("#C00")),
    QBrush(QColor("#2C0")), QBrush(QColor("#CC0")),
    QBrush(QColor("#00C")), QBrush(QColor("#C0C")),
    QBrush(QColor("#0CC")), QBrush(QColor("#CCC")),
    QBrush(QColor("#444")), QBrush(QColor("#F44")),
    QBrush(QColor("#4F4")), QBrush(QColor("#FF4")),
    QBrush(QColor("#44F")), QBrush(QColor("#F4F")),
    QBrush(QColor("#4FF")), QBrush(QColor("#FFF"))
};


ConsoleWindow::ConsoleWindow(MainWindow *parent) :
    QMainWindow(parent),
    ui(new Ui::ConsoleWindow),
    mainWindow(parent)
{
    ui->setupUi(this);

    connect(ui->action_clear, &QAction::triggered, this, &ConsoleWindow::actionClear_triggered);
    connect(ui->action_save, &QAction::triggered, this, &ConsoleWindow::actionSave_triggered);
    connect(ui->action_wrap, &QAction::triggered, this, &ConsoleWindow::actionWrap_triggered);

    connect(&ansiParser, &AnsiParser::attributesChanged, this, &ConsoleWindow::ansi_attributesChanged);
    connect(&ansiParser, &AnsiParser::printText, this, &ConsoleWindow::ansi_print_text);

    connect(&parent->connection, &Connection::messageReceived, this, &ConsoleWindow::connection_messageReceived);
    connect(&timer, &QTimer::timeout, this, &ConsoleWindow::timer_timeout);

    cursor = QTextCursor(ui->plainTextEdit->document());

    auto p = ui->plainTextEdit->palette();
    p.setColor(QPalette::All, QPalette::Base, ansiPalette[0].color());
    ui->plainTextEdit->setPalette(p);

    actionClear_triggered();
    timer.start(100);
}

ConsoleWindow::~ConsoleWindow()
{
    delete ui;
}

void ConsoleWindow::actionClear_triggered()
{
    ui->plainTextEdit->clear();
    ansi_attributesChanged(ansiParser.attributes);
}

void ConsoleWindow::actionSave_triggered()
{
    auto t = QDateTime::currentDateTime();

    auto fn = QString().sprintf(
        "log_%02d%02d%02d_%02d%02d%02d.txt",
        t.date().year() % 100, t.date().month(), t.date().day(),
        t.time().hour(), t.time().minute(), t.time().second()
    );

    auto modifiers = QApplication::keyboardModifiers();

    if (modifiers != Qt::ShiftModifier) {
        fn = QFileDialog::getSaveFileName(
            this, "Save console output", fn,
            "Text files (*.txt);;All Files (*.*)"
        );
    }

    if (fn == "")
        return;

    QFile f(fn);

    while (!f.open(QIODevice::WriteOnly)) {
        if (QMessageBox::critical(
            this, "Can't save log file", f.errorString(),
            QMessageBox::Abort | QMessageBox::Retry
        ) != QMessageBox::Retry) {
            return;
        }
    }

    f.write(ui->plainTextEdit->toPlainText().toLatin1());
    f.close();
}


void ConsoleWindow::actionWrap_triggered()
{
    if (ui->action_wrap->isChecked())
        ui->plainTextEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    else
        ui->plainTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
}


void ConsoleWindow::connection_messageReceived(const QByteArray &message)
{
    auto id = *(uint16_t*)message.constData();

    if (id == MSG_ID_SHELL_TO_PC)
        rx_buf.append( message.mid(2) );
}

void ConsoleWindow::timer_timeout()
{
    if (ui->action_pause->isChecked())
        return;

    if (rx_buf.isEmpty())
        return;

    auto pte = ui->plainTextEdit;
    auto vsb = pte->verticalScrollBar();
    bool at_bottom = vsb->value() == vsb->maximum();

    pte->setUpdatesEnabled(false);

    ansiParser.parse(rx_buf);
    if (at_bottom)
        vsb->setValue(vsb->maximum());

    pte->setUpdatesEnabled(true);

    rx_buf.clear();
}


void ConsoleWindow::ansi_attributesChanged(const AnsiParser::Attributes &attr)
{
    unsigned fg = attr.foreground + (attr.bold ? 8 : 0);
    unsigned bg = attr.background;

    Q_ASSERT(fg < 16);
    Q_ASSERT(bg < 16);

    ansiFormat.setForeground(ansiPalette[fg]);
    ansiFormat.setBackground(ansiPalette[bg]);

    cursor.setCharFormat(ansiFormat);
}


void ConsoleWindow::ansi_print_text(const QString &text)
{
    cursor.insertText(text);
}
