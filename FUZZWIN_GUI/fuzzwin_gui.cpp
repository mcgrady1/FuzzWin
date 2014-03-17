#include "fuzzwin_gui.h"

/****** CLASSE GUI *********/

FUZZWIN_GUI::FUZZWIN_GUI() : QMainWindow(nullptr),
    _env(QProcessEnvironment::systemEnvironment()), // environnement de l'application
    _fixedSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) // thauteur et largeur fixes
{  
    this->testConfig();

    /***********************************/
    /***** INITIALISATION DE LA GUI ****/
    /***********************************/

    // création du widget central
    _centralWidget = new QWidget(this);
    _centralWidget->setSizePolicy(_fixedSizePolicy);

    // création des groupes d'objets et du menu
    initGroupConfiguration();
    initGroupSession();
    initGroupOptions();
    initButtons();
    initGroupResultats();
    initLines();
    initMenuBar();

    // positionnement des objets sur une grille H=6, V=4
    _gCentralLayout = new QGridLayout(_centralWidget);
    _gCentralLayout->setContentsMargins(11, 11, 11, 11);
    _gCentralLayout->addWidget(_groupConfiguration, 0, 0, 1, 2);
    _gCentralLayout->addWidget(_groupSession,       1, 0, 1, 2);
    _gCentralLayout->addWidget(_groupOptions,       2, 0, 1, 2);

    _gCentralLayout->addWidget(_startButton,        3, 0, 1, 1);
    _gCentralLayout->addWidget(_saveSessionButton,  3, 1, 1, 1);
    _gCentralLayout->addWidget(_stopButton,         4, 0, 1, 1);
    _gCentralLayout->addWidget(_saveConfigButton,   4, 1, 1, 1);
    _gCentralLayout->addWidget(_quitButton,         5, 0, 1, 1);
    _gCentralLayout->addWidget(_aboutButton,        5, 1, 1, 1);

    _gCentralLayout->addWidget(_Vline,          0, 2, 6, 1);    
    _gCentralLayout->addWidget(_groupResultats, 0, 3, 6, 1);

    // affectation à la fenetre et redimension
    this->setMenuBar(_menuBar);
    this->setCentralWidget(_centralWidget);
    this->resize(750, 500);

    // connexion signaux/slots
    this->connectSignalsToSlots();
}

FUZZWIN_GUI::~FUZZWIN_GUI() {}

void FUZZWIN_GUI::testConfig()
{
    // test de la compatibilité de l'OS
    _osType = getNativeArchitecture();
    if (HOST_UNKNOWN == _osType)
    {
        QMessageBox::critical(nullptr, "Erreur", "OS non supporté", QMessageBox::Close);
        QApplication::quit();
    }

    // test de la présence des DLL du pintool
    QString exePath = QApplication::applicationDirPath();
    _pintool_X86    = QDir::toNativeSeparators(exePath + "/fuzzwinX86.dll");
    if (!QFile::exists(_pintool_X86))
    {
        QMessageBox::critical(nullptr, "Erreur", "pintool FuzzWin 32bits absent");
        QApplication::quit();
    }

    // pintool 64bits si besoin
    if (_osType >= BEGIN_HOST_64BITS) 
    {
        _pintool_X64 = QDir::toNativeSeparators(exePath + "/fuzzwinX64.dll");
        if (!QFile::exists(_pintool_X64))
        {
            QMessageBox::critical(nullptr, "Erreur", "pintool FuzzWin 64bits absent");
            QApplication::quit();
        }
    }
}

void FUZZWIN_GUI::initializeEnvironment()
{
    // présence "normale" de PIN = via variable d'environnement "PIN_ROOT"
    QString pinPath = _env.value("PIN_ROOT");
    
    // si la variable est renseignée : test de la présence des fichiers de PIN
    if (! pinPath.isEmpty())  checkPinPath(pinPath);

    // présence "normale" de Z3 = via variable d'environnement "Z3_ROOT"
    QString z3Path = _env.value("Z3_ROOT"); 
    
    // si la variable est renseignée : test de la présence des fichiers de Z3
    if (! z3Path.isEmpty()) checkZ3Path(z3Path);
}

void FUZZWIN_GUI::initMenuBar()
{
    _menuBar = new QMenuBar(this);
  
    _menuFichier    = new QMenu("Fichier", _menuBar);
    _action_Quitter = new QAction("Quitter", this);

    _menuAbout           = new QMenu("?", _menuBar);
    _action_AboutFuzzWin = new QAction("A propos de FuzzWin", this);
       
    _menuFichier->addAction(_action_Quitter);
    _menuAbout->addAction(_action_AboutFuzzWin);
    _menuBar->addAction(_menuFichier->menuAction());
    _menuBar->addAction(_menuAbout->menuAction());
}

void FUZZWIN_GUI::initGroupConfiguration()
{
    _groupConfiguration = new QGroupBox("Configuration", _centralWidget);
    _groupConfiguration->setSizePolicy(_fixedSizePolicy);

    _selectPin = new FuzzwinButton(_groupConfiguration, "Dossier de PIN");
    _selectZ3  = new FuzzwinButton(_groupConfiguration, "Dossier de Z3");

    _hLayout1 = new QHBoxLayout(_groupConfiguration);
    _hLayout1->addWidget(_selectPin);
    _hLayout1->addWidget(_selectZ3);
}

void FUZZWIN_GUI::initGroupSession() 
{
    _groupSession = new QGroupBox("Session", _centralWidget);
    _groupSession->setSizePolicy(_fixedSizePolicy);
    
    _selectInitialInput = new FuzzwinButton(_groupSession, "Entrée initiale");
    _selectTarget       = new FuzzwinButton(_groupSession, "Exécutable cible");
    _selectResultsDir   = new FuzzwinButton(_groupSession, "Dossier de résultats");

    _vLayout1 = new QVBoxLayout(_groupSession);
    _vLayout1->setContentsMargins(11, 11, 11, 11);
    _vLayout1->addWidget(_selectInitialInput);
    _vLayout1->addWidget(_selectTarget);
    _vLayout1->addWidget(_selectResultsDir);
}
   
void FUZZWIN_GUI::initGroupOptions()
{
    _groupOptions = new QGroupBox("Options", _centralWidget);
    _groupOptions->setSizePolicy(_fixedSizePolicy);

    _scoreEnabled          = new QCheckBox("Calcul du Score",               _groupOptions);
    _bytesToTaintEnabled   = new QCheckBox("Octets à marquer",              _groupOptions);
    _maxConstraintsEnabled = new QCheckBox("Nombre maximal de contraintes", _groupOptions);
    _maxTimeEnabled        = new QCheckBox("Temps maximal",                 _groupOptions);    
    _verboseEnabled        = new QCheckBox("Mode verbeux",                  _groupOptions);
    _keepfilesEnabled      = new QCheckBox("Garder fichiers temporaires",   _groupOptions);
    _traceOnlyEnabled      = new QCheckBox("Trace seulement",               _groupOptions);    
    
    _maxConstraints        = new QSpinBox(_groupOptions);
    _maxTime               = new QTimeEdit(_groupOptions);
    _listOfBytesToTaint    = new QLineEdit(_groupOptions);

    _maxConstraints->setEnabled(false);     // tant que la case n'est pas cochée
    _maxTime->setEnabled(false);            // tant que la case n'est pas cochée
    _listOfBytesToTaint->setEnabled(false); // tant que la case n'est pas cochée

    _traceOnlyEnabled->setEnabled(false);   // NON IMPLEMENTEE ENCORE

    _gLayout1 = new QGridLayout(_groupOptions);
    _gLayout1->setContentsMargins(11, 11, 11, 11);
    _gLayout1->addWidget(_scoreEnabled,          0, 0, 1, 1);   
    _gLayout1->addWidget(_maxConstraintsEnabled, 1, 0, 1, 1);
    _gLayout1->addWidget(_maxConstraints,        2, 0, 1, 1);
    _gLayout1->addWidget(_maxTimeEnabled,        3, 0, 1, 1);
    _gLayout1->addWidget(_maxTime,               4, 0, 1, 1);
    _gLayout1->addWidget(_bytesToTaintEnabled,   5, 0, 1, 1);
    _gLayout1->addWidget(_listOfBytesToTaint,    6, 0, 1, 1);
    _gLayout1->addWidget(_verboseEnabled,        7, 0, 1, 1);    
    _gLayout1->addWidget(_keepfilesEnabled,      8, 0, 1, 1);
    _gLayout1->addWidget(_traceOnlyEnabled,      9, 0, 1, 1);
}
    
void FUZZWIN_GUI::initGroupResultats()
{
    _groupResultats = new QGroupBox("Résultats", _centralWidget);

    _labelWorklistSize = new QLabel("Taille de la liste de travail", _groupResultats);
    _worklistSize      = new QSpinBox(_groupResultats);
    _worklistSize->setReadOnly(true);
    _worklistSize->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    _worklistSize->setButtonSymbols(QAbstractSpinBox::NoButtons);    
    _worklistSize->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    _labelElapsedTime = new QLabel("Temps écoulé", _groupResultats);
    _labelElapsedTime->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    _elapsedTime      = new QTimeEdit(_groupResultats);
    _elapsedTime->setLayoutDirection(Qt::LeftToRight);
    _elapsedTime->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    _elapsedTime->setReadOnly(true);
    _elapsedTime->setButtonSymbols(QAbstractSpinBox::NoButtons);
    _elapsedTime->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    _labelInitialInput = new QLabel("Entrée initiale",_groupResultats);
    _labelInitialInput->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _initialInput      = new InitialInputLineEdit(_groupResultats);
    _initialInput->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    _initialInput->setAcceptDrops(true);
    _initialInput->setDragEnabled(true);

    _labelTargetPath = new QLabel("Exécutable cible", _groupResultats);
    _labelTargetPath->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _targetPath      = new TargetPathLineEdit(_groupResultats);
    _targetPath->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    _targetPath->setAcceptDrops(true);
    _targetPath->setDragEnabled(true);

    _labelResultsDir  = new QLabel("Dossier de résultats", _groupResultats);
    _labelResultsDir->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _resultsDir       = new ResultsDirLineEdit(_groupResultats);
    _resultsDir->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    _resultsDir->setAcceptDrops(true);
    _resultsDir->setDragEnabled(true);
    
    _Tabs = new QTabWidget(_groupResultats);

    _logTab    = new QWidget();
    _logWindow = new QTextEdit(_logTab);
    _hLayout2  = new QHBoxLayout(_logTab);
    _hLayout2->setContentsMargins(11, 11, 11, 11);
    _hLayout2->addWidget(_logWindow);
    _logWindow->setAcceptDrops(false);

    _entriesTab   = new QWidget();    
    _listOfInputs = new QTreeWidget(_entriesTab);
    _listOfInputs->setColumnCount(4);
    _listOfInputs->setSortingEnabled(true);
    _listOfInputs->setFont(QFont("Times New Roman", 9));
    _listOfInputs->setAcceptDrops(false);    
    QStringList columnNames;
    columnNames << "Nom" << "Contraintes marquées" << "Dont inversées" << "Fautes";
    _listOfInputs->setHeaderLabels(columnNames);
    for (int i = 0 ; i < _listOfInputs->columnCount() ; ++i)
    {
        _listOfInputs->resizeColumnToContents(i);
    }
        
    QTreeWidgetItem *firstInput = new QTreeWidgetItem(columnNames);
    _listOfInputs->addTopLevelItem(firstInput);
        
    _vLayout2     = new QVBoxLayout(_entriesTab);
    _vLayout2->setContentsMargins(11, 11, 11, 11);
    _vLayout2->addWidget(_listOfInputs);

    _Tabs->addTab(_logTab,     "&Log");
    _Tabs->addTab(_entriesTab, "&Entrées");    
    _Tabs->setCurrentIndex(0);

    _gLayout2 = new QGridLayout(_groupResultats);
    _gLayout2->setContentsMargins(11, 11, 11, 11);    
    _gLayout2->addWidget(_labelWorklistSize, 0, 0, 1, 1);
    _gLayout2->addWidget(_worklistSize,      0, 1, 1, 1);

    _gLayout2->addWidget(_labelElapsedTime,  0, 2, 1, 1);
    _gLayout2->addWidget(_elapsedTime,       0, 3, 1, 1);

    _gLayout2->addWidget(_labelInitialInput, 1, 0, 1, 1);
    _gLayout2->addWidget(_initialInput,      1, 1, 1, 3);

    _gLayout2->addWidget(_labelTargetPath,   2, 0, 1, 1);
    _gLayout2->addWidget(_targetPath,        2, 1, 1, 3);    

    _gLayout2->addWidget(_labelResultsDir,   3, 0, 1, 1);
    _gLayout2->addWidget(_resultsDir,        3, 1, 1, 3);
    
    _gLayout2->addWidget(_Tabs,              4, 0, 1, 4);
}
    
void FUZZWIN_GUI::initButtons()
{
    _startButton       = new QPushButton("GO !!",          _centralWidget);
    _saveSessionButton = new QPushButton("Sauver session", _centralWidget);
    _saveConfigButton  = new QPushButton("Sauver config",  _centralWidget);
    _stopButton        = new QPushButton("STOP !!",        _centralWidget);
    _quitButton        = new QPushButton("Quitter",        _centralWidget); 
    _aboutButton       = new QPushButton("A propos...",    _centralWidget);

    _startButton->setMinimumWidth(100);
    _saveSessionButton->setMinimumWidth(100);
    _saveConfigButton->setMinimumWidth(100);
    _stopButton->setMinimumWidth(100);
    _quitButton->setMinimumWidth(100);
    _aboutButton->setMinimumWidth(100);  

    _startButton->setSizePolicy(_fixedSizePolicy);
    _saveSessionButton->setSizePolicy(_fixedSizePolicy);
    _saveConfigButton->setSizePolicy(_fixedSizePolicy);
    _stopButton->setSizePolicy(_fixedSizePolicy);
    _quitButton->setSizePolicy(_fixedSizePolicy);
    _aboutButton->setSizePolicy(_fixedSizePolicy);

    // boutons désactivés à la construction
    _startButton->setEnabled(false);   
    _stopButton->setEnabled(false);
}

void FUZZWIN_GUI::initLines()
{    
    _Vline = new QFrame(_centralWidget);
    _Vline->setFrameShape(QFrame::VLine);
    _Vline->setFrameShadow(QFrame::Sunken);
}

void FUZZWIN_GUI::connectSignalsToSlots()
{
    // cases à cocher qui activent des champs
    connect(_bytesToTaintEnabled,   SIGNAL(clicked(bool)), _listOfBytesToTaint, SLOT(setEnabled(bool)));
    connect(_maxConstraintsEnabled, SIGNAL(clicked(bool)), _maxConstraints,     SLOT(setEnabled(bool)));
    connect(_maxTimeEnabled,        SIGNAL(clicked(bool)), _maxTime,            SLOT(setEnabled(bool)));
    
    // bouton quitter => confirmation avant de le faire
    connect(_quitButton, SIGNAL(clicked(bool)), this, SLOT(quitFuzzwin()));
    
    // boutons de sélection de la GUI pour PIN, Z3 et les données de session
    connect(_selectPin,          SIGNAL(clicked()),  this, SLOT(selectPin_clicked()));
    connect(_selectZ3,           SIGNAL(clicked()),  this, SLOT(selectZ3_clicked()));
    connect(_selectInitialInput, SIGNAL(clicked()),  this, SLOT(selectInitialInput_clicked()));
    connect(_selectTarget,       SIGNAL(clicked()),  this, SLOT(selectTarget_clicked()));
    connect(_selectResultsDir,   SIGNAL(clicked()),  this, SLOT(selectResultsDir_clicked()));
   
    // signal de modification de status d'un bouton => vérifier l'ensemble
    connect(_selectPin,          SIGNAL(buttonStatusChanged()),  this, SLOT(testButtons()));
    connect(_selectZ3,           SIGNAL(buttonStatusChanged()),  this, SLOT(testButtons()));
    connect(_selectInitialInput, SIGNAL(buttonStatusChanged()),  this, SLOT(testButtons()));
    connect(_selectTarget,       SIGNAL(buttonStatusChanged()),  this, SLOT(testButtons()));
    connect(_selectResultsDir,   SIGNAL(buttonStatusChanged()),  this, SLOT(testButtons()));

    // signaux émis par le drop de fichiers sur les lignes de texte
    connect(_initialInput, SIGNAL(conformData()),   _selectInitialInput, SLOT(setButtonOk()));
    connect(_targetPath,   SIGNAL(newTargetDropped(QString)), this, SLOT(checkKindOfExe(QString)));
    connect(_resultsDir,   SIGNAL(newDirDropped(QString)),    this, SLOT(checkDir(QString)));

    // bouton GO FUZZING !!
    connect(_startButton, SIGNAL(clicked()), this, SLOT(go_clicked()));
    
    connect(_stopButton,  SIGNAL(clicked()), this, SLOT(stop_clicked()));
}

void FUZZWIN_GUI::closeEvent(QCloseEvent *e)
{
    // comme si on cliquait sur le bouton
    this->quitFuzzwin();
    // si la fonction revient ici, c'est qu'il y a eu abandon => ignorer l'event
    e->ignore();
}

void FUZZWIN_GUI::sendToLogWindow(const QString &msg)
{
    QTime thisTime = QDateTime::currentDateTime().time();
    QString fullMsg(thisTime.toString("[HH:mm:ss.zzz] : ") + msg);
    _logWindow->insertPlainText(fullMsg);
}

void FUZZWIN_GUI::selectPin_clicked()
{
    // récupération du dossier où se trouve PIN
    QString pinPath = QFileDialog::getExistingDirectory(this, 
        "Sélection du répertoire de PIN",
        QDir::currentPath(),
        QFileDialog::ShowDirsOnly);

    if (pinPath.isNull()) return;   // bouton "cancel" sélectionné

    // test de la présence des fichiers indispensables pour PIN
    checkPinPath(pinPath);
}

void FUZZWIN_GUI::selectZ3_clicked()
{
    QString z3Path = QFileDialog::getExistingDirectory(this, 
        "Sélection du répertoire de Z3",
        QDir::currentPath(),
        QFileDialog::ShowDirsOnly);

    if (z3Path.isNull()) return;    // bouton "cancel" sélectionné
    
    checkZ3Path(z3Path);
}

void FUZZWIN_GUI::selectInitialInput_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, 
        "Sélection de l'entrée initiale",
        QDir::currentPath());

    // si non Null => bouton Ok choisi, donc traiter
    if (!filename.isNull()) 
    {
        _initialInput->setText(filename);
        _initialInput->setCursorPosition(0);
        _selectInitialInput->setButtonOk();
    }
}

void FUZZWIN_GUI::selectTarget_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, 
        "Sélection de l'exécutable cible",
        QDir::currentPath(),
        "Exécutables (*.exe)");

    // si non Null => bouton Ok choisi, donc traiter
    if (!filename.isNull()) checkKindOfExe(filename);
}

void FUZZWIN_GUI::selectResultsDir_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(this, 
        "Sélection du répertoire de résultats",
        QDir::currentPath(),
        QFileDialog::ShowDirsOnly);

    // chaine nulle => annulation de la boite de dialogue, sinon traitement
    if (!dirPath.isNull()) checkDir(dirPath); 
}

void FUZZWIN_GUI::quitFuzzwin()
{
    QMessageBox::StandardButton result = QMessageBox::warning(this,
        "Confirmation", "Voulez vous vraiment quitter ?",
        QMessageBox::Yes | QMessageBox::Abort, QMessageBox::Abort);

    if (QMessageBox::Yes == result) QApplication::quit();
}

void FUZZWIN_GUI::testButtons()
{
    // si tout est au vert => activer bouton, sinon désactiver
    bool globalStatus = _selectPin->getStatus() 
                      & _selectZ3->getStatus() 
                      & _selectInitialInput->getStatus()
                      & _selectTarget->getStatus()
                      & _selectResultsDir->getStatus();
    
    _startButton->setEnabled(globalStatus);
}

void FUZZWIN_GUI::go_clicked()
{
    // tout passer en séparateurs uniformes
    QString firstInput       = QDir::toNativeSeparators(_initialInput->text());
    QString resultDirectory  = QDir::toNativeSeparators(_resultsDir->text());
    QString targetPath       = QDir::toNativeSeparators(_targetPath->text());

    // création du thread, avec entrée initiale, dossier de résultats et exécutable cible
    _fuzzwinThread = new FuzzwinAlgorithm(firstInput, resultDirectory, targetPath);

    // récupération des options     
    _fuzzwinThread->_computeScore   = _scoreEnabled->isChecked();
    _fuzzwinThread->_verbose        = _verboseEnabled->isChecked();
    _fuzzwinThread->_keepFiles      = _keepfilesEnabled->isChecked();
    _fuzzwinThread->_maxConstraints = _maxConstraintsEnabled->isChecked() ? _maxConstraints->value() : 0;
    
    if (_maxTimeEnabled->isChecked())
    {
        QTime maxTime = _maxTime->time();
        _fuzzwinThread->_maxExecutionTime = (60* maxTime.minute()) + maxTime.second();
    }
    else  _fuzzwinThread->_maxExecutionTime = 0;
   
    if (_bytesToTaintEnabled->isChecked())
    {
         _fuzzwinThread->_bytesToTaint = std::string(qPrintable(_listOfBytesToTaint->text()));
    }
    else  _fuzzwinThread->_bytesToTaint = "all";

    // Ligne de commande pour le pintool 
    _fuzzwinThread->buildPinCmdLine(_pinPath_X86, _pinPath_X64, _pintool_X86, _pintool_X64);

    // chemin vers Z3
    _fuzzwinThread->_z3Path = qPrintable(_z3Path);

    // désactiver le bouton "Go", et activer le "Stop"
    _startButton->setDisabled(true);
    _stopButton->setEnabled(true);

    // connection des signaux du thread aux slots de la GUI
    connect(this, SIGNAL(launchAlgorithm()), _fuzzwinThread, SLOT(algorithmSearch()), Qt::QueuedConnection);
    connect(_fuzzwinThread, SIGNAL(sendToGui(QString)), this, SLOT(sendToLogWindow(QString)), Qt::QueuedConnection);
    if (_verboseEnabled)
    {
        connect(_fuzzwinThread, SIGNAL(sendToGuiVerbose(QString)), this, SLOT(sendToLogWindow(QString)), Qt::QueuedConnection);
    }

    // C'EST PARTI MON KIKI
    _fuzzwinThread->run();
    emit launchAlgorithm();
    sendToLogWindow("bah ca marche passs !!!\n");

    //emit sendToGui("\n******************************\n");
    //emit sendToGui("* ---> FIN DE L'ANALYSE <--- *\n");
    //emit sendToGui("******************************\n");
}

void FUZZWIN_GUI::stop_clicked()
{
    // activer le bouton "Go", et désactiver le "Stop"
    _startButton->setEnabled(true);
    _stopButton->setDisabled(true);

    // fermer le thread
    _fuzzwinThread->terminate();
}

void FUZZWIN_GUI::checkKindOfExe(const QString &path)
{
    // conversion QString -> encodage local 
    std::string filenameLocal(qPrintable(path));

    switch (getKindOfExecutable(filenameLocal))
    {
    case SCS_64BIT_BINARY:
        if (_osType < BEGIN_HOST_64BITS)
        {
            sendToLogWindow("Exécutable 64 bits non supporté sur OS 32bits");
            _selectTarget->setButtonError();
        }
        else 
        {
            sendToLogWindow("Exécutable 64 bits sélectionné");
            _selectTarget->setButtonOk();
            _targetPath->setText(path);
        }
        break;

    case SCS_32BIT_BINARY:
        sendToLogWindow("Exécutable 32bits sélectionné");
        _selectTarget->setButtonOk();
        _targetPath->setText(path);
        break;

    default:
        sendToLogWindow("Exécutable cible incompatible");
        _selectTarget->setButtonError();
        break;
    }
    _targetPath->setCursorPosition(0);
}

void FUZZWIN_GUI::checkPinPath(QString path)
{
    bool goodNess = true; // on considere que tout est ok à priori. 1 seul false et c'est rouge

    // ajout du séparateur, si non présent
    if (!(path.endsWith('/') || path.endsWith('\\')))
    {
        path.append(QDir::separator());
    }

    // modules 32 bits 
    _pinPath_X86          = QDir::toNativeSeparators(path + "ia32/bin/pin.exe");
    QString pin_X86_VmDll = QDir::toNativeSeparators(path + "ia32/bin/pinvm.dll");

    // test de la présence des fichiers adéquats
    if (!QFile::exists(_pinPath_X86))
    {
        this->sendToLogWindow("exécutable PIN 32bits absent");

        goodNess &= false;
    }
    if (!QFile::exists(pin_X86_VmDll))
    {
        this->sendToLogWindow("librairie PIN_VM 32bits absente");
        goodNess &= false;
    }

    // OS 64 bits : présence obligatoire des modules 64bits
    if (_osType >= BEGIN_HOST_64BITS) 
    {
        // modules 64 bits
        _pinPath_X64          = QDir::toNativeSeparators(path + "intel64/bin/pin.exe");
        QString pin_X64_VmDll = QDir::toNativeSeparators(path + "intel64/bin/pinvm.dll");
    
        if (!QFile::exists(_pinPath_X64))
        {
            sendToLogWindow("exécutable PIN 64bits absent");
            goodNess &= false;
        }
        if (!QFile::exists(pin_X64_VmDll))
        {
            sendToLogWindow("librairie PIN_VM 64bits absente");
            goodNess &= false;
        }
    }
    // si tout est bon (aucun 'false' rencontré) => bouton au vert
    // sinon bouton au rouge
    if (goodNess) _selectPin->setButtonOk();
    else          _selectPin->setButtonError();
}

void FUZZWIN_GUI::checkZ3Path(QString path)
{
    // ajout du séparateur, si non présent
    if (!(path.endsWith('/') || path.endsWith('\\'))) path.append(QDir::separator());
    
    // chemin vers l'exécutable Z3
    path += "bin/z3.exe";
    
    // test de son existence
    if (QFile::exists(path))
    {
        sendToLogWindow("Répertoire de Z3 : OK");
        _z3Path = path;
        _selectZ3->setButtonOk();
    }
    else
    {
        sendToLogWindow("solveur Z3 absent");
        _selectZ3->setButtonError();
    } 
}

void FUZZWIN_GUI::checkDir(const QString &path)
{
    QDir resultsDir(path);

    // demander confirmation d'effacement du contenu si non vide
    if (resultsDir.entryInfoList(QDir::NoDotAndDotDot|QDir::AllEntries).count() != 0)
    {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, "Confirmer l'effacement du dossier ?",
            "Le répertoire suivant existe déjà : " + path + "\n"
            "Voulez-vous effacer son contenu avant exécution ?",
            QMessageBox::Ok | QMessageBox::No | QMessageBox::Abort,
            QMessageBox::No);
            
        // Abandon => aucun changement
        if      (QMessageBox::Abort == result) return ;
        // OK => effacement du dossier
        else if (QMessageBox::Ok    == result)
        {
            bool isSuccess = resultsDir.removeRecursively(); // nouveauté Qt 5.0
            isSuccess &= resultsDir.mkpath("."); 

            if (isSuccess) // tout s'est bien passé
            {
                sendToLogWindow("Effacement du dossier de résultats -> Ok");
                _selectResultsDir->setButtonOk();
                _resultsDir->setText(path);     
            }
            else
            {
                sendToLogWindow("Effacement du dossier de résultats -> Erreur");
                _selectResultsDir->setButtonError();
                _resultsDir->clear();
            }
        }
        // No => dossier inchangé
        else
        {
            sendToLogWindow("Dossier de résultats sélectionné (pas d'effacement)");
            _selectResultsDir->setButtonOk();
            _resultsDir->setText(path);

        }
    }
    // dossier existant et vide
    else
    {
         sendToLogWindow("Dossier de résultats sélectionné (vide)");
        _selectResultsDir->setButtonOk();
        _resultsDir->setText(path);
    }
    _resultsDir->setCursorPosition(0);
}