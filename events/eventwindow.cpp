#include "eventwindow.h"

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfile.h>
#include <qstring.h>

#include <QFont>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QTableWidget>
#include <fstream>
#include <iostream>

#include "ui_eventwindow.h"

using namespace std;
namespace Utils {
/**
 * FUNCTION: padLeft
 * USE: Returns a new string that right aligns the characters in a
 *   string by padding them on the left with a specified
 *   character, of a specified total length
 * @param source: The source string
 * @param totalWidth: The number of characters to pad the source string
 * @param paddingChar: The padding character
 * @return: The modified source string left padded with as many paddingChar
 *   characters needed to create a length of totalWidth
 */
std::string padLeft(std::string source, std::size_t totalWidth,
                    char paddingChar = ' ') {
  if (totalWidth > source.size()) {
    source.insert(0, totalWidth - source.size(), paddingChar);
  }
  return source;
}

/**
 * FUNCTION: padRight
 * USE: Returns a new string that left aligns the characters in a
 *   string by padding them on the right with a specified
 *   character, of a specified total length
 * @param source: The source string
 * @param totalWidth: The number of characters to pad the source string
 * @param paddingChar: The padding character
 * @return: The modified source string right padded with as many paddingChar
 *   characters needed to create a length of totalWidth
 */
std::string padRight(std::string source, std::size_t totalWidth,
                     char paddingChar = ' ') {
  if (totalWidth > source.size()) {
    source.insert(source.size(), totalWidth - source.size(), paddingChar);
  }
  return source;
}

/**
 * FUNCTION: padCenter
 * USE: Returns a new string that center aligns the characters in a
 *   string by padding them on the left and right with a specified
 *   character, of a specified total length
 * @param source: The source string
 * @param totalWidth: The number of characters to pad the source string
 * @param paddingChar: The padding character
 * @return: The modified source string padded with as many paddingChar
 *   characters needed to create a length of totalWidth
 */
std::string padCenter(std::string source, std::size_t totalWidth,
                      char paddingChar = ' ') {
  if (totalWidth > source.size()) {
    std::size_t totalPadWidth = totalWidth - source.size();
    std::size_t leftPadWidth = (totalPadWidth / 2) + source.size();
    source = padRight(padLeft(source, leftPadWidth, paddingChar), totalWidth,
                      paddingChar);
  }
  return source;
}
}  // namespace Utils

struct tableRow {
  std::string prefix;
  std::string event;
  std::string type;
  std::string datatype;
  std::string updateEvery;
  std::string comment;
};
enum modes { INPUT = 0, INPUTVALUE = 1, OUTPUT = 3, AXIS = 9 };

QList<tableRow> tableRows = QList<tableRow>();
QList<int> tableRowsToDelete = QList<int>();
QList<int> rowsChanged = QList<int>();

EventWindow::EventWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::EventWindow) {
  ui->setupUi(this);
  this->setWindowTitle("Event editor");
  QStringList headers = {"Prefix",       "Event",   "Type",  "Datatype",
                         "Update every", "Comment", "Remove"};
  auto eventGrid = new QGridLayout();
  tableRows.clear();
  tableRowsToDelete.clear();
  rowsChanged.clear();
  readFile();
  int loadedFontID =
      QFontDatabase::addApplicationFont(":/fonts/Roboto-Black.ttf");

  QFont Triforce("Roboto Black", 11, 900);
  eventTable->horizontalHeader()->setFont(Triforce);
  eventTable->setRowCount(tableRows.size());
  eventTable->setColumnCount(headers.size());
  eventTable->setHorizontalHeaderLabels(headers);

  eventTable->verticalHeader()->setVisible(false);
  eventTable->setColumnWidth(0, 50);
  eventTable->setColumnWidth(1, 450);
  eventTable->setColumnWidth(5, 250);
  eventTable->setColumnWidth(6, 80);
  eventTable->setMinimumWidth(1150);
  eventTable->setMinimumHeight(490);
  eventTable->setSortingEnabled(true);

  int rowCounter = 0;
  this->setStyleSheet("background:white;");

  for (auto &row : tableRows) {
    fillRow(rowCounter);
    rowCounter++;
  }
  connect(eventTable, &QTableWidget::itemChanged, this,
          &EventWindow::cellTextChanged);

  eventGrid->addWidget(eventTable, 0, 0, 1, 2);

  auto saveBtn = new QPushButton("Save");
  auto addBtn = new QPushButton("Add event");
  connect(addBtn, &QPushButton::pressed, this,
          &EventWindow::addEventBtnPressed);
  connect(saveBtn, &QPushButton::pressed, this, &EventWindow::saveBtnPressed);

  addBtn->setFont(Triforce);
  QFontDatabase db;
  QHBoxLayout *btnRow = new QHBoxLayout();
  eventGrid->addLayout(btnRow, 1, 1);
  btnRow->addWidget(addBtn);
  addBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  saveBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  btnRow->addWidget(saveBtn);
  addBtn->setStyleSheet(
      "background-color:#0f4c5c;"
      "border-radius:3px;"
      "color:white;"
      "font-size: 2.44em;"
      "line-height: 1.4;"
      "font-weight:bold;"
      "min-height:24px;"
      "padding:5px;");
  saveBtn->setFont(Triforce);
  saveBtn->setStyleSheet(
      "background-color:#509402;"
      "border-radius:3px;"
      "color:white;"
      "font-size: 2.44em;"
      "line-height: 1.4;"
      "font-weight:bold;"
      "min-height:24px;"
      "padding:5px;");
  this->setLayout(eventGrid);
  eventTable->setFrameStyle(QFrame::NoFrame);

  this->adjustSize();
}

void EventWindow::fillRow(int index) {
  eventTable->blockSignals(true);

  QFont Triforce("Roboto", 10, 400);
  auto prefixCell = new QTableWidgetItem(
      QString::fromStdString(tableRows[index].prefix).toStdString().c_str());

  auto eventCell =
      new QTableWidgetItem(QString::fromStdString(tableRows[index].event));
  auto typeCell =
      new QTableWidgetItem(QString::fromStdString(tableRows[index].type));
  auto updateEveryCell = new QTableWidgetItem();
  if (tableRows[index].type == "3") {
    updateEveryCell->setText(
        QString::fromStdString(tableRows[index].updateEvery));
  } else {
    updateEveryCell->setFlags(Qt::ItemIsSelectable);
    updateEveryCell->setText("n/a");
  }
  auto commentCell =
      new QTableWidgetItem(QString::fromStdString(tableRows[index].comment));
  auto typeComboBox = new QComboBox();
  typeComboBox->setObjectName("cb" + QString::number(index));

  typeComboBox->addItems({"Input", "Input with value", "Axis", "Output"});
  switch (stoi(tableRows[index].type)) {
    case INPUT:
      typeComboBox->setCurrentIndex(0);
      break;
    case INPUTVALUE:
      typeComboBox->setCurrentIndex(1);
      break;
    case AXIS:
      typeComboBox->setCurrentIndex(2);
      break;
    case OUTPUT:
      typeComboBox->setCurrentIndex(3);
      break;
  }
  auto dataTypeCombobox = new QComboBox();
  dataTypeCombobox->setObjectName("cb" + QString::number(index));

  dataTypeCombobox->addItems({"Int", "Float", "Bool"});

  if (tableRows[index].datatype == "i") {
    dataTypeCombobox->setCurrentIndex(0);
  } else if (tableRows[index].datatype == "f") {
    dataTypeCombobox->setCurrentIndex(1);
  } else if (tableRows[index].datatype == "b") {
    dataTypeCombobox->setCurrentIndex(2);
  }
  dataTypeCombobox->setObjectName("dt" + QString::number(index));

  QTableWidgetItem *naDatatype = new QTableWidgetItem("n/a");
  naDatatype->setFlags(Qt::ItemIsSelectable);
  prefixCell->setFont(Triforce);
  eventCell->setFont(Triforce);
  commentCell->setFont(Triforce);
  updateEveryCell->setFont(Triforce);

  typeComboBox->setFocusPolicy(Qt::StrongFocus);
  typeComboBox->installEventFilter(this);
  auto *removeToggle = new QCheckBox();
  removeToggle->setStyleSheet("margin-left:50%; margin-right:50%;");
  removeToggle->setObjectName("del" + QString::number(index));
  connect(removeToggle, &QCheckBox::stateChanged, this,
          &EventWindow::delChanged);
  connect(typeComboBox, &QComboBox::currentIndexChanged, this,
          &EventWindow::comboBoxChanged);
  eventTable->setItem(index, 0, prefixCell);
  eventTable->setItem(index, 1, eventCell);
  eventTable->setCellWidget(index, 2, typeComboBox);
  if (tableRows[index].type == "3") {
    eventTable->setCellWidget(index, 3, dataTypeCombobox);
  } else {
    eventTable->setItem(index, 3, naDatatype);
  }

  eventTable->setItem(index, 4, updateEveryCell);
  eventTable->setItem(index, 5, commentCell);
  eventTable->setCellWidget(index, 6, removeToggle);
  eventTable->blockSignals(false);
}
void EventWindow::delChanged() {
  auto sendCb = qobject_cast<QCheckBox *>(sender());

  int index = sendCb->objectName().mid(3).toInt();
  if (sendCb->checkState() == Qt::Checked) {
    tableRowsToDelete.append(index);
  } else {
    tableRowsToDelete.remove(tableRowsToDelete.indexOf(index));
  }
}
void EventWindow::cellTextChanged(QTableWidgetItem *changedItem) {
  std::string textToCompare;
  changedItem->tableWidget()->blockSignals(true);
  switch (changedItem->column()) {
    case 0:
      textToCompare = tableRows[changedItem->row()].prefix;
      break;
    case 1:
      textToCompare = tableRows[changedItem->row()].event;
      break;

    case 3:
      textToCompare = tableRows[changedItem->row()].updateEvery;
      break;
    case 4:
      textToCompare = tableRows[changedItem->row()].comment;
      break;
  }

  QFont font;
  if (textToCompare != changedItem->text().toStdString()) {
    changedItem->setForeground(QColor(1, 150, 11));

    font.setBold(true);

  } else {
    font.setBold(false);
    changedItem->setForeground(QColor(0, 0, 0));
  }

  changedItem->setFont(font);
  changedItem->tableWidget()->blockSignals(false);
  if (checkIfRowChanged(changedItem->row())) {
    if (rowsChanged.indexOf(changedItem->row()) == -1) {
      rowsChanged.append(changedItem->row());
      cout << "Changed" << changedItem->row() << endl;
    }
  }
  cout << changedItem->text().toStdString() << endl;
}

void EventWindow::addEventBtnPressed() {
  tableRow *newTableRow = new tableRow();
  eventTable->setRowCount(eventTable->rowCount() + 1);
  newTableRow->prefix = "0000";
  newTableRow->event = "Your event goes here";
  newTableRow->comment = "Comments go here";
  newTableRow->updateEvery = "0";
  newTableRow->type = "0";
  tableRows.append(*newTableRow);
  fillRow(tableRows.size() - 1);
}

void EventWindow::saveBtnPressed() {
  // A row gets flagged for change when the data != the first found state
  // If the value changes from default -> changed value -> the default value
  // We want to remove the changed flag
  for (auto &changed : rowsChanged) {
    if (!checkIfRowChanged(changed)) {
      rowsChanged.removeAt(rowsChanged.indexOf(changed));
      cout << "ROW REMOVED" << endl;
    }
  }
  auto saveMessageBox = new QMessageBox();
  if (rowsChanged.size() > 0 || tableRowsToDelete.size() > 0) {
    saveMessageBox->setText(
        QString::number(rowsChanged.size() + tableRowsToDelete.size()) +
        " Changes have been found");
    saveMessageBox->setInformativeText("Do you want to save these changes?");
    QString detailedText = "";

    for (auto &toRemove : tableRowsToDelete) {
      detailedText += "DELETE: ";
      detailedText += QString::fromStdString(tableRows[toRemove].prefix) + " ";
      detailedText += QString::fromStdString(tableRows[toRemove].event) + "\n";
    }

    for (auto &changed : rowsChanged) {
      detailedText +=
          "OLD VALUE: " + QString::fromStdString(tableRows[changed].prefix);
      detailedText += (" " + QString::fromStdString(tableRows[changed].event));
      detailedText +=
          (" " + QString::fromStdString(tableRows[changed].comment));
      detailedText += " -> NEW VALUE: " + eventTable->item(changed, 0)->text();
      detailedText += (" " + eventTable->item(changed, 1)->text());
      detailedText += (" " + eventTable->item(changed, 5)->text() + "\n");
    }

    saveMessageBox->setDetailedText(detailedText);
    saveMessageBox->setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);

    QSpacerItem *horizontalSpacer =
        new QSpacerItem(600, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout *layout = (QGridLayout *)saveMessageBox->layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1,
                    layout->columnCount());
  } else {
    saveMessageBox->setText("No changes have been made");
  }

  int ret = saveMessageBox->exec();
  for (auto &changed : rowsChanged) {
    cout << changed << endl;
  }

  switch (ret) {
    case QMessageBox::Save:

      for (auto &changedRow : rowsChanged) {
        cout << "CHANGED " << endl;
        tableRows[changedRow].prefix =
            eventTable->item(changedRow, 0)->text().toStdString();
        tableRows[changedRow].event =
            eventTable->item(changedRow, 1)->text().toStdString();
        tableRows[changedRow].comment =
            eventTable->item(changedRow, 5)->text().toStdString();
        int newType =
            this->findChild<QComboBox *>("cb" + QString::number(changedRow))
                ->currentIndex();

        switch (newType) {
          case 0:
            newType = 0;
            break;
          case 1:
            newType = 1;
            break;
          case 2:
            newType = 9;
            break;
          case 3:
            newType = 3;
            break;
        }
        tableRows[changedRow].updateEvery =
            eventTable->item(changedRow, 4)->text().toStdString();
        tableRows[changedRow].type = to_string(newType);
        if (tableRows[changedRow].type == "3") {
          int newDatatype =
              this->findChild<QComboBox *>("dt" + QString::number(changedRow))
                  ->currentIndex();
          std::string dtNewStr = "";
          switch (newDatatype) {
            case 0:
              dtNewStr = "i";
              break;
            case 1:
              dtNewStr = "f";
              break;
            case 2:
              dtNewStr = "b";
              break;
            default:
              dtNewStr = "i";
              break;
          }

          tableRows[changedRow].datatype = dtNewStr;
        }
      }
      for (auto &del : tableRowsToDelete) {
        cout << "FOUND DELETE" << tableRows[del].event << endl;
        eventTable->removeRow(del);
        tableRows.removeAt(del);
        cout << "FOUND DELETE AFTER" << tableRows[del].event << endl;
        cout << "DELTED " << del << endl;
      }
      writeFile();
      break;
    case QMessageBox::Cancel:
      break;
  }
}

void EventWindow::writeFile() {
  QFile newEventsFile(applicationEventsPath);
  cout << "WRITEFILE " << newEventsFile.fileName().toStdString().c_str()
       << endl;
  newEventsFile.open(QIODevice::ReadWrite);
  newEventsFile.resize(0);
  QTextStream out(&newEventsFile);
  for (auto &row : tableRows) {
    if (row.updateEvery == "n/a") {
      row.updateEvery = "0";
    }
    QString typeString = "";
    if (row.type == "3") {
      typeString = QString::fromStdString(row.type) +
                   QString::fromStdString(row.datatype);
    } else {
      typeString = QString::fromStdString(row.type);
    }
    QString formattedAxis = "";
    if (row.type == "9") {
      formattedAxis = "-0+1023";
    }
    QString formattedEvent = checkSpaces(row.event);
    out << formattedEvent << "^" << typeString
        << "#" + QString::fromStdString(Utils::padLeft(row.prefix, 4))
        << formattedAxis << "$" << QString::fromStdString(row.updateEvery)
        << "//" << QString::fromStdString(row.comment)

        << "\n";
  }
  newEventsFile.close();
}
QString EventWindow::checkSpaces(std::string stringToCheck) {
  std::vector<size_t> *vec = new std::vector<size_t>();

  std::string charsToFind[] = {"{", "}", "+", "-", "/", "*", "(", ")"};
  for (auto &c : charsToFind) {
    size_t pos = stringToCheck.find(c);
    while (pos != std::string::npos) {
      if (pos < stringToCheck.size() - 1 && stringToCheck[pos + 1] != ' ' &&
          c != "(") {
        stringToCheck.insert(pos + 1, " ");
      }
      if (pos > 0 && stringToCheck[pos - 1] != ' ' && c != ")") {
        stringToCheck.insert(pos, " ");
      }

      pos = stringToCheck.find(c, pos + c.size());
    }
  }
  std::string formattedString = stringToCheck;

  return QString::fromStdString(stringToCheck);
}

void EventWindow::comboBoxChanged() {
  auto senderComboBox = qobject_cast<QComboBox *>(sender());
  int index = stoi(senderComboBox->objectName().toStdString().substr(2));
  if (checkIfRowChanged(index)) {
    if (rowsChanged.indexOf(index) == -1) {
      rowsChanged.append(index);
    }
  }
  if (senderComboBox->currentIndex() == 3) {
    auto dataTypeCombobox = new QComboBox();
    dataTypeCombobox->setObjectName("cb" + QString::number(index));

    dataTypeCombobox->addItems({"Int", "Float", "Bool"});

    if (tableRows[index].datatype == "i") {
      dataTypeCombobox->setCurrentIndex(0);
    } else if (tableRows[index].datatype == "f") {
      dataTypeCombobox->setCurrentIndex(1);
    } else if (tableRows[index].datatype == "b") {
      dataTypeCombobox->setCurrentIndex(2);
    }
    dataTypeCombobox->setObjectName("dt" + QString::number(index));
    eventTable->setCellWidget(index, 3, dataTypeCombobox);
    eventTable->removeCellWidget(index, 4);
    QTableWidgetItem *emptyUpdateEvery = new QTableWidgetItem("0");
    eventTable->setItem(index, 4, emptyUpdateEvery);

  } else {
    eventTable->removeCellWidget(index, 3);
    QTableWidgetItem *naTextWidget = new QTableWidgetItem("n/a");
    naTextWidget->setFlags(Qt::ItemIsSelectable);
    eventTable->setItem(index, 3, naTextWidget);
    eventTable->setItem(index, 4, naTextWidget);
  }
}

bool EventWindow::eventFilter(QObject *obj, QEvent *e) {
  if (e->type() == QEvent::Wheel) {
    QComboBox *combo = qobject_cast<QComboBox *>(obj);
    if (combo && !combo->hasFocus()) return true;
  }

  return false;
}

bool EventWindow::checkIfRowChanged(int index) {
  bool checks[] = {tableRows[index].prefix.compare(
                       eventTable->item(index, 0)->text().toStdString()) == 0,
                   tableRows[index].event.compare(
                       eventTable->item(index, 1)->text().toStdString()) == 0,
                   //      tableRows[index].updateEvery ==
                   //        //                eventTable->item(index,
                   //        3)->text().toStdString()== 0,
                   tableRows[index].comment.compare(
                       eventTable->item(index, 4)->text().toStdString()) == 0};

  int typeIndex = this->findChild<QComboBox *>("cb" + QString::number(index))
                      ->currentIndex();
  switch (typeIndex) {
    case 0:
      typeIndex = INPUT;
      break;
    case 1:
      typeIndex = INPUTVALUE;
      break;
    case 2:
      typeIndex = AXIS;
      break;
    case 3:
      typeIndex = OUTPUT;
      break;
  }

  if (stoi(tableRows[index].type) != typeIndex) {
    cout << "HELP" << endl;
    return true;
  }
  for (auto &check : checks) {
    if (!check) return true;
  }
  return false;
}

EventWindow::~EventWindow() {
  emit EventWindow::closedEventWindow();
  delete ui;
}

void EventWindow::closeEvent(QCloseEvent *event) {
  emit EventWindow::closedEventWindow();
  delete ui;
}
void EventWindow::readFile() {
  std::ifstream file(applicationEventsPath.toStdString());
  std::string row;

  while (std::getline(file, row)) {
    int modeDelimiter = row.find("^");
    int prefixDelimiter = row.find("#");
    int updateEveryDelimiter = row.find("$");
    int commentDelimiter = row.find("//");
    if (row.front() == ' ') {
      row.erase(0, 1);
    }
    if (row.size() > 25 && row.at(0) != '/') {
      auto newRow = new tableRow();
      newRow->prefix = row.substr(prefixDelimiter + 1, 4);
      newRow->event = row.substr(0, modeDelimiter);
      newRow->type = row.substr(modeDelimiter + 1, 1);
      if (newRow->type == "3") {
        newRow->datatype = row.substr(modeDelimiter + 2, 1);
      }

      newRow->updateEvery =
          row.substr(updateEveryDelimiter + 1,
                     commentDelimiter - updateEveryDelimiter - 1);
      newRow->comment = row.substr(commentDelimiter + 2);

      tableRows.append(*newRow);
      qDebug() << newRow->comment.c_str();
    }
  }
  file.close();
}
