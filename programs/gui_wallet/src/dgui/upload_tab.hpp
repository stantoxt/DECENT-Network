/*
 *	File      : upload_tab.hpp
 *
 *	Created on: 21 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef UPLOAD_TAB_HPP
#define UPLOAD_TAB_HPP

#include <map>
#include <string>

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QTimer>
#include <QDateEdit>
#include <QComboBox>

#include "decent_button.hpp"
#include "gui_wallet_tabcontentmanager.hpp"

#include <QDialog>
#include <vector>
#include "qt_commonheader.hpp"
#include "gui_wallet_global.hpp"
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"



namespace gui_wallet
{
    enum FieldsRows {
        LIFETIME = 0,
        KEYPARTS,
        PRICE,
        ASSETID,
        SEEDERS,
        CONTENTPATH,
        SELECTPATH,
        SAMPLESPATH,
        SELECTSAMPLES,
        NUM_FIELDS
    };

   class Mainwindow_gui_wallet;
   
    
    
    class Upload_popup : public QDialog
    {
        Q_OBJECT
        
        typedef std::map<std::string, std::string> AssetMap;
        friend class Upload_tab;
    public:
        Upload_popup(Mainwindow_gui_wallet* pMainWindow);

    public slots:
        void browseContent();
        void browseSamples();
        void uploadContent();
        void onGrabPublishers();
        void uploadCanceled();
        void updateUploadButtonStatus();

    public:
        void onPublishersDone(void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result);
       
    private:
        Mainwindow_gui_wallet* m_pMainWindow;
       
        QVBoxLayout*     u_main_layout;
        QLineEdit*       _titleText;
        QTextEdit*       _descriptionText;
        QDateEdit*       _lifeTime;
        QComboBox*       _seeders;
        QComboBox*       _keyparts;
        QLineEdit*       _price;
        QLineEdit*       _contentPath;
        QLineEdit*       _samplesPath;
        DecentButton*    _upload_button;
        DecentButton*    _cancel_button;
       
        QTimer          m_getPublishersTimer;
        QTimer*          _buttonStatusCheck;
       
       std::map<std::string, double> _publisherIdToPriceMap;
    public:
    signals:
        void uploadFinished();
    };
    
}








namespace gui_wallet
{
    class Mainwindow_gui_wallet;
    
    
    class Upload_tab : public TabContentManager
    {
        Q_OBJECT;
    public:
        Upload_tab(Mainwindow_gui_wallet* parent);
        void ShowDigitalContentsGUI();
       
        
    public:
       virtual void timeToUpdate(const std::string& result);
       virtual std::string getUpdateCommand();
       
    public slots:
        void show_content_popup();
        void content_was_bought();
        void uploadPopup();
        
    protected:
        QVBoxLayout     m_main_layout;
        QHBoxLayout     m_search_layout;
        DecentTable     m_pTableWidget;
        QLineEdit       m_filterLineEdit;
        QComboBox       m_searchTypeCombo;
        DecentButton*   upload_button;
        Upload_popup    popup;
        
        std::vector<SDigitalContent>  _digital_contents;
        ContentDetailsGeneral*        _content_popup;
        Mainwindow_gui_wallet*        _parent;
       bool                          _isUploading;
    
        bool                          m_doUpdate;
        QTimer                        m_contentUpdateTimer;
    };
    

    
}

#endif //UploadTab_H
