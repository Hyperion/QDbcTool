#include "DTForm.h"
#include <QtCore/QDataStream>
#include <QtGui/QTableView>
#include <QtGui/QFileDialog>
#include <QtGui/QSortFilterProxyModel>
#include <QtCore/QAbstractItemModel>

#include "Alphanum.h"

DTForm::DTForm(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);

    format = new DBCFormat("dbcFormats.xml");
    dbc = new DTObject(this, format);

    fieldBox = new QToolButton(this);
    fieldBox->setText("Fields");
    fieldBox->setPopupMode(QToolButton::InstantPopup);
    fieldBox->setFixedSize(50, 20);

    progressBar = new QProgressBar(this);
    progressBar->setValue(0);
    progressBar->setTextVisible(false);
    progressBar->setTextDirection(QProgressBar::TopToBottom);
    progressBar->setFixedSize(100, 20);

    dbcInfo = new QLabel(this);
    statusText = new QLabel("Ready!", this);

    mainToolBar->addWidget(progressBar);
    mainToolBar->addSeparator();
    mainToolBar->addWidget(fieldBox);
    mainToolBar->addSeparator();
    mainToolBar->addWidget(dbcInfo);
    mainToolBar->addSeparator();
    mainToolBar->addWidget(statusText);

    proxyModel = new DBCSortedModel(this);
    proxyModel->setDynamicSortFilter(true);
    tableView->setModel(proxyModel);

    //config = new QSettings("config.ini", QSettings::IniFormat, this);

    connect(actionOpen, SIGNAL(triggered()), this, SLOT(SlotOpenFile()));

    // Export actions
    connect(actionExport_as_SQL, SIGNAL(triggered()), this, SLOT(SlotExportAsSQL()));
    connect(actionExport_as_CSV, SIGNAL(triggered()), this, SLOT(SlotExportAsCSV()));

    connect(actionWrite_DBC, SIGNAL(triggered()), this, SLOT(SlotWriteDBC()));

    connect(tableView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(SlotCustomContextMenu(const QPoint&)));

    connect(actionAbout, SIGNAL(triggered()), this, SLOT(SlotAbout()));
}

void DTForm::SlotRemoveRecord()
{
    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());

    QItemSelectionModel *selectionModel = tableView->selectionModel();
    
    QModelIndexList indexes = selectionModel->selectedRows();
    QModelIndex index;

    foreach (index, indexes)
    {
        int row = smodel->mapToSource(index).row();
        model->removeRows(row, 1, QModelIndex());
    }
}

bool DBCTableModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; ++row)
        m_dbcList.removeAt(position);

    m_dbc->SetRecordCount(m_dbcList.size());

    endRemoveRows();
    
    return true;
}

void DTForm::SlotCustomContextMenu(const QPoint& pos)
{
    QModelIndex index = tableView->indexAt(pos);
    QMenu* menu = new QMenu(this);

    QAction* add = new QAction("Add record (not implemented)", this);

    QAction* remove = new QAction("Remove record", this);
    connect(remove, SIGNAL(triggered()), this, SLOT(SlotRemoveRecord()));

    menu->addAction(add);
    menu->addAction(remove);
    menu->popup(tableView->viewport()->mapToGlobal(pos));
}

void DTForm::ApplyFilter()
{
    for (QList<QAction*>::iterator itr = fieldBox->actions().begin(); itr != fieldBox->actions().end(); ++itr)
        (*itr)->deleteLater();

    for (quint32 i = 0; i < dbc->GetFieldCount(); i++)
    {
        if (!format->IsVisible(i))
            tableView->hideColumn(i);

        QAction* action = new QAction(this);
        action->setText(format->GetFieldName(i));
        action->setCheckable(true);
        action->setChecked(!format->IsVisible(i));
        action->setData(i);

        fieldBox->addAction(action);
    }
    connect(fieldBox, SIGNAL(triggered(QAction*)), this, SLOT(SlotSetVisible(QAction*)));
}

void DTForm::SlotSetVisible(QAction* action)
{
    if (action->isChecked())
    {
        format->SetFieldAttribute(action->data().toUInt(), "visible", "false");
        tableView->hideColumn(action->data().toUInt());
    }
    else
    {
        format->SetFieldAttribute(action->data().toUInt(), "visible", "true");
        tableView->showColumn(action->data().toUInt());
    }
}

DBCSortedModel::DBCSortedModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool DBCSortedModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QVariant leftData = sourceModel()->data(left);
    QVariant rightData = sourceModel()->data(right);

    if (compare(leftData.toString(), rightData.toString()) < 0)
        return true;

    return false;
}

DTForm::~DTForm()
{
}


DTBuild::DTBuild(QWidget *parent, DTForm* form)
    : QDialog(parent), m_form(form)
{
    setupUi(this);
    show();
}

DTBuild::~DTBuild()
{
}

AboutForm::AboutForm(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    show();
}

AboutForm::~AboutForm()
{
}

void DTForm::SlotAbout()
{
    new AboutForm;
}

void DTForm::SlotExportAsSQL()
{
    if (!dbc->isEmpty())
    {
        QString fileName = QFileDialog::getSaveFileName(this, "Save as SQL file", ".", "SQL File (*.sql)");

        if (!fileName.isEmpty())
        {
            dbc->SetSaveFileName(fileName);
            statusText->setText("Exporting to SQL file...");
            dbc->ThreadBegin(THREAD_EXPORT_SQL);
        }
    }
    else
        statusText->setText("DBC not loaded! Please load DBC file!");

}

void DTForm::SlotExportAsCSV()
{
    if (!dbc->isEmpty())
    {
        QString fileName = QFileDialog::getSaveFileName(this, "Save as CSV file", ".", "CSV File (*.csv)");

        if (!fileName.isEmpty())
        {
            dbc->SetSaveFileName(fileName);
            statusText->setText("Exporting to CSV file...");
            dbc->ThreadBegin(THREAD_EXPORT_CSV);
        }
    }
    else
        statusText->setText("DBC not loaded! Please load DBC file!");

}

void DTForm::SlotWriteDBC()
{
    if (!dbc->isEmpty())
    {
        QString fileName = QFileDialog::getSaveFileName(this, "Save as DBC file", ".", "DBC File (*.dbc)");

        if (!fileName.isEmpty())
        {
            dbc->SetSaveFileName(fileName);
            statusText->setText("Writing DBC file...");
            dbc->ThreadBegin(THREAD_WRITE_DBC);
        }
    }
    else
        statusText->setText("DBC not loaded! Please load DBC file!");

}

void DTForm::SlotOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open DBC file", ".", "DBC Files (*.dbc)");

    if (fileName.isEmpty())
        return;

    QFileInfo finfo(fileName);

    QStringList buildList = format->GetBuildList(finfo.baseName());
    if (!buildList.isEmpty())
    {
        DTBuild* build = new DTBuild;
        build->comboBox->clear();
        build->comboBox->addItems(buildList);

        if (build->exec() == QDialog::Accepted)
        {
            dbcInfo->setText(QString(" <b>Name:</b> <font color=green>%0</font> <b>Build:</b> <font color=green>%1</font> ")
                .arg(finfo.baseName())
                .arg(build->comboBox->currentText()));

            statusText->setText("Loading DBC file...");

            DBCSortedModel* smodel = static_cast<DBCSortedModel*>(tableView->model());
            DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
            
            if (model)
                delete model;

            dbc->Set(fileName, build->comboBox->currentText());
            dbc->ThreadBegin(THREAD_OPENFILE);
        }
    }
    else
    {
        dbcInfo->setText(QString(" <b>Name:</b> <font color=green>%0</font> <b>Build:</b> <font color=green>Unknown</font> ")
                .arg(finfo.baseName()));

        statusText->setText("Loading unknown DBC file with default format...");

        DBCSortedModel* smodel = static_cast<DBCSortedModel*>(tableView->model());
        DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
        if (model)
            delete model;

        dbc->Set(fileName);
        dbc->ThreadBegin(THREAD_OPENFILE);
    }
}

bool DTForm::event(QEvent *ev)
{
    switch (ev->type())
    {
        case ProgressBar::TypeId:
        {
            ProgressBar* bar = (ProgressBar*)ev;
            
            switch (bar->GetId())
            {
                case BAR_STEP:
                {
                    progressBar->setValue(bar->GetStep());
                    return true;
                }
                case BAR_SIZE:
                {
                    progressBar->setMaximum(bar->GetSize());
                    return true;
                }
            }

            break;
        }
        case SendModel::TypeId:
        {
            SendModel* m_ev = (SendModel*)ev;
            proxyModel->setSourceModel(m_ev->GetObject());
            ApplyFilter();
            return true;
        }
        break;
        case SendText::TypeId:
        {
            SendText* m_ev = (SendText*)ev;
            switch (m_ev->GetId())
            {
                case 1:
                    statusText->setText(m_ev->GetText());
                    break;
                default:
                    break;
            }
            return true;
        }
        break;
        default:
            break;
    }
    

    return QWidget::event(ev);
}

DBCTableModel::DBCTableModel(QObject *parent, DTObject *dbc)
    : QAbstractTableModel(parent), m_dbc(dbc)
{
    m_dbcList.clear();
}

DBCTableModel::DBCTableModel(DBCList dbcList, QObject *parent, DTObject *dbc)
    : QAbstractTableModel(parent), m_dbc(dbc)
{
    m_dbcList = dbcList;
}

void DBCTableModel::clear()
{
    beginResetModel();
    m_dbcList.clear();
    m_fieldNames.clear();
    endResetModel();
}

int DBCTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_dbc->GetRecordCount();
}

int DBCTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_dbc->GetFieldCount();
}

QVariant DBCTableModel::data(const QModelIndex &index, int role) const
{
    if (m_dbcList.isEmpty())
        return QVariant();

    if (!index.isValid())
        return QVariant();

    if (index.row() >= m_dbcList.size() || index.row() < 0)
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return m_dbcList.at(index.row()).at(index.column());

    return QVariant();
}

bool DBCTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (m_dbcList.isEmpty())
        return false;

    if (!index.isValid())
        return false;

    if (index.row() >= m_dbcList.size() || index.row() < 0)
        return false;

    if (role == Qt::EditRole)
    {
        QStringList p = m_dbcList.at(index.row());
        p.replace(index.column(), value.toString());
        m_dbcList.replace(index.row(), p);
        emit(dataChanged(index, index));

        return true;
    }

    return false;
}

QVariant DBCTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (m_fieldNames.isEmpty())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
        return m_fieldNames.at(section);

    return QVariant();
}

Qt::ItemFlags DBCTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}