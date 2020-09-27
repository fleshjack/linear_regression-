#include "coincontroldialog.h"
#include "ui_coincontroldialog.h"

#include "init.h"
#include "base58.h"
#include "bitcoinunits.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "optionsmodel.h"
#include "coincontrol.h"
#include "rpcserver.h"
#include "guiutil.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QCursor>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFlags>
#include <QIcon>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>

using namespace std;
QList<qint64> CoinControlDialog::payAmounts;
CCoinControl* CoinControlDialog::coinControl = new CCoinControl();

CoinControlDialog::CoinControlDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CoinControlDialog),
    model(0)
{
    ui->setupUi(this);

    // context menu actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
             copyTransactionHashAction = new QAction(tr("Copy transaction ID"), this);  // we need to enable/disable this
             lockAction = new QAction(tr("Lock unspent"), this);                        // we need to enable/disable this
             unlockAction = new QAction(tr("Unlock unspent"), this);                    // we need to enable/disable this

    // context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTransactionHashAction);
    contextMenu->addSeparator();
    contextMenu->addAction(lockAction);
    contextMenu->addAction(unlockAction);

    // context menu signals
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTransactionHashAction, SIGNAL(triggered()), this, SLOT(copyTransactionHash()));
    connect(lockAction, SIGNAL(triggered()), this, SLOT(lockCoin()));
    connect(unlockAction, SIGNAL(triggered()), this, SLOT(unlockCoin()));

    // clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy low output"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);

    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(clipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(clipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(clipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(clipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(clipboardBytes()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(clipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(clipboardChange()));

    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    // toggle tree/list mode
    connect(ui->radioTreeMode, SIGNAL(toggled(bool)), this, SLOT(radioTreeMode(bool)));
    connect(ui->radioListMode, SIGNAL(toggled(bool)), this, SLOT(radioListMode(bool)));

    // click on checkbox
    connect(ui->treeWidget, SIGNAL(itemChanged( QTreeWidgetItem*, int)), this, SLOT(viewItemChanged( QTreeWidgetItem*, int)));

    // click on header
	#if QT_VERSION < 0x050000
        ui->treeWidget->header()->setClickable(true);
	#else
        ui->treeWidget->header()->setSectionsClickable(true);
    #endif
    connect(ui->treeWidget->header(), SIGNAL(sectionClicked(int)), this, SLOT(headerSectionClicked(int)));

    // ok button
    connect(ui->buttonBox, SIGNAL(clicked( QAbstractButton*)), this, SLOT(buttonBoxClicked(QAbstractButton*)));

    // (un)select all
    connect(ui->pushButtonSelectAll, SIGNAL(clicked()), this, SLOT(buttonSelectAllClicked()));

    // custom Coin Control Selection Button (select less than)
    connect(ui->pushButtonCustomCC, SIGNAL(clicked()), this, SLOT(customSelectCoins()));

    ui->treeWidget->setColumnWidth(COLUMN_CHECKBOX, 80);
    ui->treeWidget->setColumnWidth(COLUMN_AMOUNT, 100);
	ui->treeWidget->setColumnWidth(COLUMN_CONFIRMATIONS, 85);
	ui->treeWidget->setColumnWidth(COLUMN_AGE, 55);
	ui->treeWidget->setColumnWidth(COLUMN_POTENTIALSTAKE, 120);
	ui->treeWidget->setColumnWidth(COLUMN_TIMEESTIMATE, 150);
	ui->treeWidget->setColumnWidth(COLUMN_WEIGHT, 70);
    ui->treeWidget->setColumnWidth(COLUMN_LABEL, 85);
    ui->treeWidget->setColumnWidth(COLUMN_ADDRESS, 125);
    ui->treeWidget->setColumnWidth(COLUMN_DATE, 110);
	ui->treeWidget->setColumnHidden(COLUMN_AGE_INT64, true);
	ui->treeWidget->setColumnHidden(COLUMN_POTENTIALSTAKE_INT64, true);
    ui->treeWidget->setColumnHidden(COLUMN_TXHASH, true);         // store transacton hash in this column, but dont show it
    ui->treeWidget->setColumnHidden(COLUMN_VOUT_INDEX, true);     // store vout index in this column, but dont show it
    ui->treeWidget->setColumnHidden(COLUMN_AMOUNT_INT64, true);   // store amount int64 in this column, but dont show it

    // default view is sorted by confirmations desc
    sortView(COLUMN_CONFIRMATIONS, Qt::DescendingOrder);

	// combo box to select coin filter
	ui->QComboBoxFilterCoins->addItem("< Amount");
	ui->QComboBoxFilterCoins->addItem("> Amount");
	ui->QComboBoxFilterCoins->addItem("< Weight");
	ui->QComboBoxFilterCoins->addItem("> Weight");
	ui->QComboBoxFilterCoins->addItem("> Age");
    ui->QComboBoxFilterCoins->addItem("< Age");
}

CoinControlDialog::~CoinControlDialog()
{
    delete ui;
}

void CoinControlDialog::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel() && model->getAddressTableModel())
    {
        updateView();
        updateLabelLocked();
        CoinControlDialog::updateLabels(model, this);
    }
}

// helper function str_pad
QString CoinControlDialog::strPad(QString s, int nPadLength, QString sPadding)
{
    while (s.length() < nPadLength)
        s = sPadding + s;

    return s;
}

// ok button
void CoinControlDialog::buttonBoxClicked(QAbstractButton* button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
        done(QDialog::Accepted); // closes the dialog
}

// (un)select all
void CoinControlDialog::buttonSelectAllClicked()
{
    Qt::CheckState state = Qt::Checked;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
    {
        if (ui->treeWidget->topLevelItem(i)->checkState(COLUMN_CHECKBOX) != Qt::Unchecked)
        {
            state = Qt::Unchecked;
            break;
        }
    }
    ui->treeWidget->setEnabled(false);
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
            if (ui->treeWidget->topLevelItem(i)->checkState(COLUMN_CHECKBOX) != state)
                ui->treeWidget->topLevelItem(i)->setCheckState(COLUMN_CHECKBOX, state);
    ui->treeWidget->setEnabled(true);
    CoinControlDialog::updateLabels(model, this);
}

void CoinControlDialog::customSelectCoins()
{
	QString strUserAmount = ui->lineEditCustomCC->text();
	QString strComboText = ui->QComboBoxFilterCoins->currentText();
	
	double dUserAmount = QString(strUserAmount).toDouble();
	bool treeMode = ui->radioTreeMode->isChecked();
	
	QFlags<Qt::ItemFlag> flgCheckbox=Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        
    map<QString, vector<COutput> > mapCoins;
    model->listCoins(mapCoins);

    BOOST_FOREACH(PAIRTYPE(QString, vector<COutput>) coins, mapCoins)
    {
        QTreeWidgetItem *itemWalletAddress = new QTreeWidgetItem();
    
        QTreeWidgetItem *itemOutput;
        if (treeMode)    itemOutput = new QTreeWidgetItem(itemWalletAddress);
        else             itemOutput = new QTreeWidgetItem(ui->treeWidget);
        itemOutput->setFlags(flgCheckbox);
        itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Unchecked);
        BOOST_FOREACH(const COutput& out, coins.second)
        {
            // transaction hash
            uint256 txhash = out.tx->GetHash();
        
            //Getting the coin amount
            double dCoinAmount = out.tx->vout[out.i].nValue;
                
            //Coin Weight
            uint64_t nTxWeight = 0;
            model->getStakeWeightFromValue(out.tx->GetTxTime(), out.tx->vout[out.i].nValue, nTxWeight);
                
            //Age
            double dAge = (GetTime() - out.tx->GetTxTime()) / (double)(1440 * 60);
        
            COutPoint outpt(txhash, out.i);
            
            //selecting the coins
            if (strComboText == "< Amount")
            {
                if (dCoinAmount < dUserAmount * COIN)
                {			
                    coinControl->Select(outpt);
                    itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
                }	
            }
            else if (strComboText == "> Amount")
            {
                if (dCoinAmount > dUserAmount * COIN)
                {			
                    coinControl->Select(outpt);
                    itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
                }
            }
            else if (strComboText == "< Weight")
            {
                if (nTxWeight < dUserAmount)
                {			
                    coinControl->Select(outpt);
                    itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
                }
            }
            else if (strComboText == "> Weight")
            {
                if (nTxWeight > dUserAmount)
                {			
                    coinControl->Select(outpt);
                    itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
                }
            }
            else if (strComboText == "< Age")
            {
                if (dAge < dUserAmount)
                {			
                    coinControl->Select(outpt);
                    itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
                }
            }
            else if (strComboText == "> Age")
            {
                if (dAge > dUserAmount)
                {			
                    coinControl->Select(outpt);
                    itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
                }
            }
            else
            {
                coinControl->UnSelect(outpt);
                itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Unchecked);
            }
        }
    }	

    CoinControlDialog::updateLabels(model, this);
	updateView();
}

// context menu
void CoinControlDialog::showMenu(const QPoint &point)
{
    QTreeWidgetItem *item = ui->treeWidget->itemAt(point);
    if(item)
    {
        contextMenuItem = item;

        // disable some items (like Copy Transaction ID, lock, unlock) for tree roots in context menu
        if (item->text(COLUMN_TXHASH).length() == 64) // transaction hash is 64 characters (this means its a child node, so its not a parent node in tree mode)
        {
            copyTransactionHashAction->setEnabled(true);
            if (model->isLockedCoin(uint256(item->text(COLUMN_TXHASH).toStdString()), item->text(COLUMN_VOUT_INDEX).toUInt()))
            {
               lockAction->setEnabled(false);
               unlockAction->setEnabled(true);
            }
            else
            {
               lockAction->setEnabled(true);
               unlockAction->setEnabled(false);
            }
        }
        else // this means click on parent node in tree mode -> disable all
        {
            copyTransactionHashAction->setEnabled(false);
            lockAction->setEnabled(false);
            unlockAction->setEnabled(false);
        }

        // show context menu
        contextMenu->exec(QCursor::pos());
    }
}

// context menu action: copy amount
void CoinControlDialog::copyAmount()
{
    QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_AMOUNT));
}

// context menu action: copy label
void CoinControlDialog::copyLabel()
{
    if (ui->radioTreeMode->isChecked() && contextMenuItem->text(COLUMN_LABEL).length() == 0 && contextMenuItem->parent())
        QApplication::clipboard()->setText(contextMenuItem->parent()->text(COLUMN_LABEL));
    else
        QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_LABEL));
}

// context menu action: copy address
void CoinControlDialog::copyAddress()
{
    if (ui->radioTreeMode->isChecked() && contextMenuItem->text(COLUMN_ADDRESS).length() == 0 && contextMenuItem->parent())
        QApplication::clipboard()->setText(contextMenuItem->parent()->text(COLUMN_ADDRESS));
    else
        QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_ADDRESS));
}

// context menu action: copy transaction id
void CoinControlDialog::copyTransactionHash()
{
    QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_TXHASH));
}

// context menu action: lock coin
void CoinControlDialog::lockCoin()
{
    if (contextMenuItem->checkState(COLUMN_CHECKBOX) == Qt::Checked)
        contextMenuItem->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);

    COutPoint outpt(uint256(contextMenuItem->text(COLUMN_TXHASH).toStdString()), contextMenuItem->text(COLUMN_VOUT_INDEX).toUInt());
    model->lockCoin(outpt);
    contextMenuItem->setDisabled(true);
    contextMenuItem->setIcon(COLUMN_CHECKBOX, QIcon(":/icons/lock_closed"));
    updateLabelLocked();
}

// context menu action: unlock coin
void CoinControlDialog::unlockCoin()
{
    COutPoint outpt(uint256(contextMenuItem->text(COLUMN_TXHASH).toStdString()), contextMenuItem->text(COLUMN_VOUT_INDEX).toUInt());
    model->unlockCoin(outpt);
    contextMenuItem->setDisabled(false);
    contextMenuItem->setIcon(COLUMN_CHECKBOX, QIcon());
    updateLabelLocked();
}

// copy label "Quantity" to clipboard
void CoinControlDialog::clipboardQuantity()
{
    QApplication::clipboard()->setText(ui->labelCoinControlQuantity->text());
}

// copy label "Amount" to clipboard
void CoinControlDialog::clipboardAmount()
{
    QApplication::clipboard()->setText(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// copy label "Fee" to clipboard
void CoinControlDialog::clipboardFee()
{
    QApplication::clipboard()->setText(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")));
}

// copy label "After fee" to clipboard
void CoinControlDialog::clipboardAfterFee()
{
    QApplication::clipboard()->setText(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")));
}

// copy lab