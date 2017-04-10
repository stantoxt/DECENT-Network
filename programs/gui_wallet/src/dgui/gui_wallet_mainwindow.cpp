/*
 *	File: gui_wallet_mainwindow.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */



#include <QMenuBar>
#include <QMoveEvent>
#include <QMessageBox>

#include "qt_commonheader.hpp"
#include "gui_wallet_mainwindow.hpp"
#include "gui_wallet_global.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <graphene/utilities/dirhelper.hpp>
#include <graphene/wallet/wallet.hpp>

#ifndef DEFAULT_WALLET_FILE_NAME
#define DEFAULT_WALLET_FILE_NAME       "wallet.json"
#endif

using namespace nlohmann;
using namespace gui_wallet;
using namespace std;
using namespace graphene;
using namespace utilities;

static gui_wallet::Mainwindow_gui_wallet*  s_pMainWindowInstance = NULL;

WalletOperator::WalletOperator()
: QObject(nullptr)
, m_wallet_api()
{

}

WalletOperator::~WalletOperator()
{

}

void WalletOperator::slot_connect()
{
   string str_error;
   try
   {
      m_wallet_api.Connent();
   }
   catch(std::exception const& ex)
   {
      str_error = ex.what();
   }
   emit signal_connected(str_error);
}




Mainwindow_gui_wallet::Mainwindow_gui_wallet()
: m_ActionExit(tr("&Exit"),this)
, m_ActionConnect(tr("Connect"),this)
, m_ActionAbout(tr("About"),this)
, m_ActionInfo(tr("Info"),this)
, m_ActionHelp(tr("Help"),this)
, m_ActionLock(tr("Lock"),this)
, m_ActionUnlock(tr("Unlock"),this)
, m_ActionImportKey(tr("Import key"),this)
, m_info_dialog()
, m_locked(true)
, m_import_key_dlg(2, "Key Import")
, m_nConnected(0)
, m_SetPasswordDialog(this, true)
, m_UnlockDialog(this, false)
, m_p_wallet_operator(new WalletOperator())
, m_wallet_operator_thread(this)

{
   s_pMainWindowInstance = this;

   m_barLeft = new QMenuBar;
   m_barRight = new QMenuBar;

   m_pCentralAllLayout = new QVBoxLayout;
   m_pMenuLayout = new QHBoxLayout;

   fc::path wallet_path = decent_path_finder::instance().get_decent_home() / DEFAULT_WALLET_FILE_NAME;

   m_pMenuLayout->addWidget(m_barLeft);
   m_pMenuLayout->addWidget(m_barRight);


   m_pCentralWidget = new CentralWigdet(m_pCentralAllLayout,this);
   m_pCentralWidget->setLayout(m_pCentralAllLayout);
   //setCentralWidget(m_pCentralWidget);
   CreateActions();
   CreateMenues();
   resize(900,550);

   setCentralWidget(m_pCentralWidget);

   m_info_dialog.resize(0,0);

   setUnifiedTitleAndToolBarOnMac(false);

   QComboBox* pUsersCombo = m_pCentralWidget->usersCombo();


   connect(pUsersCombo, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(CurrentUserChangedSlot(const QString&)) );


   setWindowTitle(tr("DECENT - Blockchain Content Distribution"));

   centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
   setStyleSheet("QMainWindow{color:black;""background-color:white;}");


   m_p_wallet_operator->moveToThread(&m_wallet_operator_thread);
   m_wallet_operator_thread.start();

   DCT_VERIFY(connect(this, SIGNAL(signal_connect()),
                      m_p_wallet_operator, SLOT(slot_connect())));
   DCT_VERIFY(connect(m_p_wallet_operator, SIGNAL(signal_connected(std::string)),
                      this, SLOT(slot_connected(std::string))));

   //WalletInterface::initialize();
   //ConnectSlot();
   emit signal_connect();

   //connect(&GlobalEvents::instance(), SIGNAL(walletConnected()), this, SLOT(DisplayWalletContentGUI()));
   //connect(&GlobalEvents::instance(), SIGNAL(walletConnectionError(std::string)), this, SLOT(DisplayConnectionError(std::string)));

    connect(pUsersCombo, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(CurrentUserChangedSlot(const QString&)) );
    
    
    setWindowTitle(tr("DECENT - Blockchain Content Distribution"));
    
    centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
    setStyleSheet("QMainWindow{color:black;""background-color:white;}");
    
   
   
    
    _balanceUpdater.setSingleShot(false);
    _balanceUpdater.setInterval(10000);
    connect(&_balanceUpdater, SIGNAL(timeout()), this, SLOT( currentUserBalanceUpdate() ));
    _balanceUpdater.start();
   
   connect(&GlobalEvents::instance(), SIGNAL(walletConnected(bool)), this, SLOT(DisplayWalletContentGUI(bool)));
   connect(&GlobalEvents::instance(), SIGNAL(walletConnectionError(std::string)), this, SLOT(DisplayConnectionError(std::string)));

   
   _downloadChecker.setSingleShot(false);
   _downloadChecker.setInterval(5000);
   connect(&_downloadChecker, SIGNAL(timeout()), this, SLOT(CheckDownloads()));
   _downloadChecker.start();
}

Mainwindow_gui_wallet::~Mainwindow_gui_wallet()
{
   m_wallet_operator_thread.quit();
   m_wallet_operator_thread.wait();
   m_p_wallet_operator->m_wallet_api.SaveWalletFile();
   //WalletInterface::destroy();
}

void Mainwindow_gui_wallet::slot_connected(std::string str_error)
{
   if (str_error.empty())
   {
      _downloadChecker.setSingleShot(false);
      _downloadChecker.setInterval(5000);
      connect(&_downloadChecker, SIGNAL(timeout()), this, SLOT(CheckDownloads()));
      _downloadChecker.start();

      DisplayWalletContentGUI(m_p_wallet_operator->m_wallet_api.IsNew());
   }
   //else
      //QMessageBox::critical(this, "Error", str_error.c_str());
}

void Mainwindow_gui_wallet::currentUserBalanceUpdate()
{
    std::string userBalanceUpdate = GlobalEvents::instance().getCurrentUser();
    if( userBalanceUpdate == "" ) {
        return;
    }
    UpdateAccountBalances(userBalanceUpdate);
}



void Mainwindow_gui_wallet::RunTaskImpl(std::string const& str_command, std::string& str_result)
{
   if (s_pMainWindowInstance == NULL) {
      throw std::runtime_error("Main window is not initialized yet");
   }
   
   str_result = s_pMainWindowInstance->m_p_wallet_operator->m_wallet_api.RunTaskImpl(str_command);
}

bool Mainwindow_gui_wallet::RunTaskParseImpl(std::string const& str_command, nlohmann::json& json_result) {
   try {
      std::string str_result;
      Mainwindow_gui_wallet::RunTaskImpl(str_command, str_result);
      json_result = json::parse(str_result);
      return true;
   } catch (const std::exception& ex) {
      json_result = json(ex.what());
   } catch (...) {
      json_result = json("Unhandled exception");
   }
   return false;
}


void Mainwindow_gui_wallet::CreateActions()
{
    m_ActionExit.setStatusTip( tr("Exit Program") );
    connect( &m_ActionExit, SIGNAL(triggered()), this, SLOT(close()) );

    m_ActionAbout.setStatusTip( tr("About") );
    connect( &m_ActionAbout, SIGNAL(triggered()), this, SLOT(AboutSlot()) );

    m_ActionHelp.setStatusTip( tr("Help") );
    connect( &m_ActionHelp, SIGNAL(triggered()), this, SLOT(HelpSlot()) );

    m_ActionInfo.setStatusTip( tr("Info") );
    connect( &m_ActionInfo, SIGNAL(triggered()), this, SLOT(InfoSlot()) );

    m_ActionConnect.setStatusTip( tr("Connect to witness node") );
    connect( &m_ActionConnect, SIGNAL(triggered()), this, SLOT(ConnectSlot()) );

   
    m_ActionLock.setDisabled(m_locked);
    m_ActionLock.setStatusTip( tr("Lock account") );
    connect( &m_ActionLock, SIGNAL(triggered()), this, SLOT(LockSlot()) );

    m_ActionUnlock.setEnabled(m_locked);
    m_ActionUnlock.setStatusTip( tr("Unlock account") );
    connect( &m_ActionUnlock, SIGNAL(triggered()), this, SLOT(UnlockSlot()) );

    m_ActionImportKey.setDisabled(true);
    m_ActionImportKey.setStatusTip( tr("Import key") );
    connect( &m_ActionImportKey, SIGNAL(triggered()), this, SLOT(ImportKeySlot()) );


}


void Mainwindow_gui_wallet::CreateMenues()
{

    QMenuBar* pMenuBar = m_barLeft;
    m_pMenuFile = pMenuBar->addMenu( tr("&File") );
    m_pMenuFile->addAction( &m_ActionExit );
    m_pMenuFile->addAction( &m_ActionConnect );
    m_pMenuFile->addAction( &m_ActionLock );
    m_pMenuFile->addAction( &m_ActionUnlock );
    m_pMenuFile->addAction( &m_ActionImportKey );





    m_pMenuView = pMenuBar->addMenu( tr("&View") );

    QAction* browseAction = new QAction(tr("Browse Content"), this);
    m_pMenuView->addAction(browseAction);
    browseAction->setProperty("index", 0);

    QAction* transactionsAction = new QAction(tr("Transactions"), this);
    m_pMenuView->addAction(transactionsAction);
    transactionsAction->setProperty("index", 1);

    QAction* uploadAction = new QAction(tr("Publish"), this);
    m_pMenuView->addAction(uploadAction);
    uploadAction->setProperty("index", 2);

    QAction* overviewAction = new QAction(tr("Overview"), this);
    m_pMenuView->addAction(overviewAction);
    overviewAction->setProperty("index", 3);

    QAction* purchasedAction = new QAction(tr("Purchased"), this);
    m_pMenuView->addAction(purchasedAction);
    purchasedAction->setProperty("index", 4);

    connect( browseAction, SIGNAL(triggered()), this, SLOT(ViewAction()) );
    connect( transactionsAction, SIGNAL(triggered()), this, SLOT(ViewAction()) );
    connect( uploadAction, SIGNAL(triggered()), this, SLOT(ViewAction()) );
    connect( overviewAction, SIGNAL(triggered()), this, SLOT(ViewAction()) );
    connect( purchasedAction, SIGNAL(triggered()), this, SLOT(ViewAction()) );




    m_pMenuHelpL = pMenuBar->addMenu( tr("&Help") );

    /******************************************************/
    m_pMenuHelpL->addAction(&m_ActionAbout);
    m_pMenuHelpL->addAction(&m_ActionInfo);
    m_pMenuHelpL->addAction(&m_ActionHelp);
    
    
}

void Mainwindow_gui_wallet::ViewAction() {
    QAction* act = dynamic_cast<QAction*>(sender());
    int index = act->property("index").toInt();

    m_pCentralWidget->SetMyCurrentTabIndex(index);
}

void Mainwindow_gui_wallet::CurrentUserChangedSlot(const QString& a_new_user)
{
    GlobalEvents::instance().setCurrentUser(a_new_user.toStdString());
    UpdateAccountBalances(a_new_user.toStdString());
}


void Mainwindow_gui_wallet::UpdateAccountBalances(const std::string& username) {

   
   json allAssets;
   std::string getAssetsCommand = "list_assets \"\" 100";
   if (!RunTaskParse(getAssetsCommand, allAssets)) {
      ALERT_DETAILS("Could not get account balances", allAssets.get<string>().c_str());
      return;
   }
   
   
   std::string csLineToRun = "list_account_balances " + username;
   json allBalances;
   
   if (!RunTaskParse(csLineToRun, allBalances)) {
      ALERT_DETAILS("Could not get account balances", allBalances.get<string>().c_str());
      return;
   }
   
   
   
   std::vector<std::string> balances;
   for (int i = 0; i < allBalances.size(); ++i) {
      
      std::string assetName = "Unknown";
      int precision = 1;
      
      for (int assInd = 0; assInd < allAssets.size(); ++assInd) {
         if (allAssets[assInd]["id"].get<std::string>() == allBalances[i]["asset_id"]) {
            assetName = allAssets[assInd]["symbol"].get<std::string>();
            precision = allAssets[assInd]["precision"].get<int>();
            break;
         }
      }
      
      double amount = 0;
      if (allBalances[i]["amount"].is_number()) {
         amount = allBalances[i]["amount"].get<double>();
      } else {
         amount = std::stod(allBalances[i]["amount"].get<std::string>());
      }
      amount = amount / pow(10, precision);
      
      QString str = QString::number(amount) + tr(" ") + QString::fromStdString(assetName);
      
      balances.push_back(str.toStdString());
   }
   m_pCentralWidget->SetAccountBalancesFromStrGUI(balances);
   
}

void Mainwindow_gui_wallet::LockSlot()
{
    m_ActionLock.setDisabled(true);
    m_ActionUnlock.setDisabled(true);
   
    const std::string csLine = "lock";
    std::string dummy;
   
    try {
        RunTask(csLine, dummy);
    } catch (std::exception& ex) {
        ALERT_DETAILS("Unable to lock the wallet", ex.what());
    }
    
    UpdateLockedStatus();
    
}


void Mainwindow_gui_wallet::UnlockSlot()
{
    
    QPoint thisPos = pos();
    thisPos.rx() += size().width() / 2;
    thisPos.ry() += size().height() / 2;

    
    
    std::string cvsPassword;
    
   if(!m_UnlockDialog.execRD(thisPos, cvsPassword)) {
      UpdateLockedStatus();
      return;
   }
   
    m_ActionLock.setDisabled(true);
    m_ActionUnlock.setDisabled(true);
    
    const std::string csPassLine = "unlock " + cvsPassword;
    json result;
   
    if (!RunTaskParse(csPassLine, result)) {
       ALERT_DETAILS("Unable to unlock the wallet", result.get<std::string>().c_str());
       return;
    }
    
    UpdateLockedStatus();
    
}


void Mainwindow_gui_wallet::UpdateLockedStatus()
{
   const std::string csLine = "is_locked";
   std::string a_result;

   try
   {
      RunTask(csLine, a_result);
      m_locked = (a_result == "true");

      if (false == m_locked)
         GlobalEvents::instance().setWalletUnlocked();

   }
   catch (const std::exception& ex)
   {
      ALERT_DETAILS("Unable to get wallet lock status", ex.what());
      m_locked = true;
   }

   m_ActionLock.setDisabled(m_locked);
   m_ActionUnlock.setEnabled(m_locked);
   if (m_locked)
   {
      UnlockSlot();
   }
}



void Mainwindow_gui_wallet::CheckDownloads()
{
    auto& global_instance = gui_wallet::GlobalEvents::instance();
    std::string str_current_username = global_instance.getCurrentUser();

    if (str_current_username == "") {
        _activeDownloads.clear();
        return;
    }
   
   json contents;
   if (!RunTaskParse("search_my_purchases \"" + str_current_username +"\" \"\" \"\" ", contents)) {
      std::cout << contents.get<string>() << std::endl;
      return;
   }
   
   
   for (int i = 0; i < contents.size(); ++i) {
      
      auto content = contents[i];
      std::string URI = contents[i]["URI"].get<std::string>();
      
      if (URI == "") {
         continue;
      }
      
      if (_activeDownloads.find(URI) == _activeDownloads.end()) {
         json ignore_result;
         if (RunTaskParse("download_package \"" + URI +"\" ", ignore_result)) {
            _activeDownloads.insert(URI);
         } else {
            std::cout << "Can not resume download: " << URI << std::endl;
            std::cout << "Error: " << ignore_result.get<string>() << std::endl;
            
         }
         
      }
      
   }
   
   
   

}


void Mainwindow_gui_wallet::DisplayConnectionError(std::string errorMessage) {
   ALERT_DETAILS("Could not connect to wallet", errorMessage.c_str());
}


void Mainwindow_gui_wallet::DisplayWalletContentGUI(bool isNewWallet)
{
   if (isNewWallet)
   {
      SetPassword();
   }

   m_ActionLock.setDisabled(true);
   m_ActionUnlock.setDisabled(true);
   UpdateLockedStatus();

   m_ActionImportKey.setEnabled(true);
   QComboBox& userCombo = *m_pCentralWidget->usersCombo();

   try
   {
      std::string a_result;
      RunTask("list_my_accounts", a_result);

      auto accs = json::parse(a_result);

      for (int i = 0; i < accs.size(); ++i)
      {
         std::string id = accs[i]["id"].get<std::string>();
         std::string name = accs[i]["name"].get<std::string>();

         userCombo.addItem(tr(name.c_str()));
      }

      if (accs.size() > 0)
      {
         userCombo.setCurrentIndex(0);
         UpdateAccountBalances(userCombo.itemText(0).toStdString());
      }
   }
   catch (const std::exception& ex)
   {
      //ALERT_DETAILS("Failed to get account information", ex.what());
      QMessageBox::critical(this, "Error", QString("Failed to get account information - %1").arg(ex.what()));
   }
}


void Mainwindow_gui_wallet::ImportKeySlot()
{
    std::vector<std::string> cvsUsKey(2);
    QComboBox& cUsersCombo = *m_pCentralWidget->usersCombo();
    cUsersCombo.setWindowTitle("key import");
    cvsUsKey[0] = "";
    cvsUsKey[1] = "";

    if(cUsersCombo.count()&&(cUsersCombo.currentIndex()>0))
    {
        QString cqsUserName = cUsersCombo.currentText();
        QByteArray cbaResult = cqsUserName.toLatin1();
        cvsUsKey[0] = cbaResult.data();
    }

    QPoint thisPos = pos();
    thisPos.rx() += size().width() / 2 - 175;
    thisPos.ry() += size().height() / 2 - 75;
    RET_TYPE aRet = m_import_key_dlg.execRD(&thisPos,cvsUsKey);
   
    if(aRet == RDB_CANCEL){
        return ;
    }

    std::string csTaskStr = "import_key " + cvsUsKey[0] + " " + cvsUsKey[1];
    std::string result;
    bool hasError = false;
    
    try {
        RunTask(csTaskStr, result);
        hasError = result.find("exception") != std::string::npos;
    } catch (...) {
        hasError = true;
    }
    
    if (hasError) {
        ALERT_DETAILS("Can not import key.", result.c_str());
    } else {
        DisplayWalletContentGUI(false);

    }

    
}


void Mainwindow_gui_wallet::InfoSlot()
{
    try {
        std::string a_result;
        
        RunTask("info", a_result);
        QString aStrToDisplay(tr(a_result.c_str()));
        
        m_info_dialog.setFixedSize(600,500);
        m_info_dialog->setText(aStrToDisplay);
        m_info_dialog.exec();
    } catch (...) {
        
    }

}


void Mainwindow_gui_wallet::AboutSlot()
{
    try {
        std::string a_result;
        
        RunTask("about", a_result);
        
        m_info_dialog.setFixedSize(500,300);
        m_info_dialog->setText(tr(a_result.c_str()));
        m_info_dialog.exec();
    } catch (...) {
        
    }
    
}


void Mainwindow_gui_wallet::HelpSlot()
{
    
    try {
        std::string a_result;
        
        RunTask("help", a_result);
        
        m_info_dialog.setFixedSize(500,500);
        m_info_dialog->setText(tr(a_result.c_str()));
        m_info_dialog.exec();
    } catch (...) {
        
    }
}





void Mainwindow_gui_wallet::ConnectSlot()
{
}




void Mainwindow_gui_wallet::SetPassword()
{
   std::string pcsPassword;
   
   QPoint thisPos = pos();
   thisPos.rx() += this->size().width() / 2;
   thisPos.ry() += this->size().height() / 2;
   
   if (m_SetPasswordDialog.execRD(thisPos, pcsPassword))
   {
      const std::string setPassword = "set_password " + pcsPassword;
      const std::string unlockTask = "unlock " + pcsPassword;
      std::string result;
      
      try
      {
         RunTask(setPassword, result);
         RunTask(unlockTask, result);

         m_p_wallet_operator->m_wallet_api.SaveWalletFile();
         
         m_ActionImportKey.setEnabled(true);
         m_ActionUnlock.setEnabled(false);
         m_ActionLock.setEnabled(true);
      }
      catch (const std::exception& ex)
      {
         ALERT_DETAILS("Unable to unlock the wallet", ex.what());
      }
   }
}


void Mainwindow_gui_wallet::GoToThisTab(int index , std::string info)
{
    m_pCentralWidget->SetTransactionInfo(info);
    m_pCentralWidget->SetMyCurrentTabIndex(index);
}

