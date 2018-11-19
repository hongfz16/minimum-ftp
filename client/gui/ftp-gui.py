import sys
import os
from pathlib import Path
from PyQt5.QtCore import (Qt, QDir, QTimer, QEvent,
                          pyqtSignal, QThreadPool)
from PyQt5.QtWidgets import (QMainWindow, QApplication,
                             QDesktopWidget, QAction, qApp,
                             QMenu, QGridLayout, QPushButton,
                             QWidget, QLabel, QLineEdit,
                             QTextEdit, QLCDNumber, QSlider,
                             QVBoxLayout, QFileDialog, QListView,
                             QHBoxLayout, QSplitter, QFrame,
                             QFileSystemModel, QStyle, QPlainTextEdit,
                             QMessageBox, QInputDialog, QListWidget,
                             QListWidgetItem, QProgressBar)
from PyQt5.QtGui import (QStandardItemModel, QIcon, QStandardItem,
                         QTextCursor, QColor, QGuiApplication)
from mFTP import mFTP
from ftpWorker import putWorker, getWorker
from ftpFileModel import LocalFileSystemModel, RemoteFileSystemModel

class FTPGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.debug = False
        self.ftp = mFTP()
        if self.debug:
            self.ftp.open_handler('127.0.0.1', 8000)
            self.ftp.user_handler('anonymous', 'password')
        self.initUI()
        self.threadpool = QThreadPool()
        self.getFlags = []
        self.putFlags = []

    def initUI(self):
        self.centerwidget = QWidget(self)
        self.setCentralWidget(self.centerwidget)
        self.setupLayout(self.centerwidget)
        self.setupMenu()
        self.setupWidget()

        self.resize(1200, 960)
        self.center()
        self.setWindowTitle('Minimum-FTP')
        self.show()

    def center(self):
        qr = self.frameGeometry()
        cp = QDesktopWidget().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())

    def setupLayout(self, wid):
        # Auth related widget
        authlay = QHBoxLayout()

        hostLabel = QLabel('Host:')
        portLabel = QLabel('Port:')
        usernameLabel = QLabel('Username:')
        passwordLabel = QLabel('Password:')

        self.hostInput = QLineEdit()
        self.portInput = QLineEdit()
        self.usernameInput = QLineEdit()
        self.passwordInput = QLineEdit()
        self.passwordInput.setEchoMode(QLineEdit.Password)

        self.connectBtn = QPushButton('Connect', self)
        if self.debug:
            self.connectBtn.setEnabled(False)
        # self.connectBtn.setIcon(QApplication.style().standardIcon(QStyle.SP_DirIcon))

        authlay.addWidget(hostLabel)
        authlay.addWidget(self.hostInput)
        authlay.addWidget(portLabel)
        authlay.addWidget(self.portInput)
        authlay.addWidget(usernameLabel)
        authlay.addWidget(self.usernameInput)
        authlay.addWidget(passwordLabel)
        authlay.addWidget(self.passwordInput)
        authlay.addWidget(self.connectBtn)

        # Show commands in this widget
        commandlay = QVBoxLayout()

        commandLabel = QLabel('Command Window')

        self.commandText = QTextEdit()
        
        commandlay.addWidget(commandLabel)
        commandlay.addWidget(self.commandText)

        # Show local file and remote file in this widget
        locallay = QVBoxLayout()
        localpathlay = QHBoxLayout()
        remotelay = QVBoxLayout()
        remotepathlay = QHBoxLayout()

        localfileLabel = QLabel('Local File')
        remotefileLabel = QLabel('Remote File')

        self.localfileLine = QLineEdit()
        self.remotefileLine = QLineEdit()
        self.localfileList = QListView()
        self.remotefileList = QListView()
        # self.localfileReload = QPushButton()
        # self.remotefileReload = QPushButton()
        # self.localfileReload.setIcon(QApplication.style().standardIcon(QStyle.SP_BrowserReload))
        # self.remotefileReload.setIcon(QApplication.style().standardIcon(QStyle.SP_BrowserReload))

        localpathlay.addWidget(localfileLabel)
        localpathlay.addWidget(self.localfileLine)
        # localpathlay.addWidget(self.localfileReload)
        remotepathlay.addWidget(remotefileLabel)
        remotepathlay.addWidget(self.remotefileLine)
        # remotepathlay.addWidget(self.remotefileReload)
        
        locallay.addLayout(localpathlay)
        locallay.addWidget(self.localfileList)
        remotelay.addLayout(remotepathlay)
        remotelay.addWidget(self.remotefileList)

        localframe = QFrame(self)
        localframe.setFrameShape(QFrame.StyledPanel)
        localframe.setLayout(locallay)
        remoteframe = QFrame(self)
        remoteframe.setFrameShape(QFrame.StyledPanel)
        remoteframe.setLayout(remotelay)

        filesplitter = QSplitter(Qt.Horizontal)
        filesplitter.addWidget(localframe)
        filesplitter.addWidget(remoteframe)

        self.tasklist = QListWidget(self)

        # Set overall layout
        vlay = QVBoxLayout()

        authframe = QFrame(self)
        authframe.setFrameShape(QFrame.StyledPanel)
        authframe.setLayout(authlay)
        commandframe = QFrame(self)
        commandframe.setFrameShape(QFrame.StyledPanel)
        commandframe.setLayout(commandlay)

        topsplitter = QSplitter(Qt.Vertical)
        topsplitter.addWidget(authframe)
        topsplitter.addWidget(commandframe)
        topsplitter.addWidget(filesplitter)
        topsplitter.addWidget(self.tasklist)

        vlay.addWidget(topsplitter)

        wid.setLayout(vlay)

    def setupMenu(self):
        self.statusbar = self.statusBar()
        self.statusbar.showMessage('Ready')

        menubar = self.menuBar()
        menubar.setNativeMenuBar(False)
        fileMenu = menubar.addMenu('File')
        aboutMenu = menubar.addMenu('About')

        modeMenu = QMenu('Mode', self)
        self.binaryAct = QAction('Binary', self, checkable=True)
        self.binaryAct.setChecked(False)
        self.binaryAct.triggered.connect(self.setBinary)
        modeMenu.addAction(self.binaryAct)
        fileMenu.addMenu(modeMenu)

        systemAct = QAction('System', self)
        systemAct.triggered.connect(self.checkSystem)
        aboutMenu.addAction(systemAct)

    def setupWidget(self):
        self.connectBtn.clicked.connect(self.connectClicked)

        self.commandText.setReadOnly(True)
        self.localfileLine.setReadOnly(True)
        self.remotefileLine.setReadOnly(True)

        self.localfileModel = LocalFileSystemModel()
        self.localfileList.setModel(self.localfileModel)
        self.localfileList.doubleClicked.connect(self.doubleClickLocalFile)
        self.localfileLine.setText(str(self.localfileModel.path))

        self.remotefileModel = RemoteFileSystemModel(self.ftp, self)
        self.remotefileList.setModel(self.remotefileModel)
        self.remotefileList.doubleClicked.connect(self.doubleClickRemoteFile)
        self.remotefileLine.setText(str(self.remotefileModel.path))

        # self.localfileReload.clicked.connect(self.localfileModel.setupItem)
        # self.remotefileReload.clicked.connect(self.remotefileModel.setupItem)

        self.remotefileList.installEventFilter(self)
        self.localfileList.installEventFilter(self)

    def eventFilter(self, source, event):
        if (event.type() == QEvent.ContextMenu and
            source is self.remotefileList):
            menu = QMenu()

            getAct = QAction('Download', self)
            menu.addAction(getAct)
            mkdirAct = QAction('New Directory', self)
            mkdirAct.triggered.connect(self.remoteMkdir)
            menu.addAction(mkdirAct)
            rmdirAct = QAction('Remove Directory', self)
            menu.addAction(rmdirAct)
            renameAct = QAction('Rename', self)
            menu.addAction(renameAct)
            refreshAct = QAction('Refresh', self)
            menu.addAction(refreshAct)

            action = menu.exec_(event.globalPos())
            if action:
                item = source.indexAt(event.pos())
                item_name = item.data()
                if action == rmdirAct:
                    self.remoteRmdir(item_name)
                elif action == renameAct:
                    self.remoteRename(item_name)
                elif action == refreshAct:
                    self.remotefileModel.setupItem()
                elif action == getAct:
                    self.getSlot(item_name)
            return True
        elif (event.type() == QEvent.ContextMenu and
              source is self.localfileList):
            menu = QMenu()

            putAct = QAction('Upload', self)
            menu.addAction(putAct)

            action = menu.exec_(event.globalPos())
            if action:
                item = source.indexAt(event.pos())
                item_name = item.data()
                if action == putAct:
                    self.putSlot(item_name)

        return super(FTPGUI, self).eventFilter(source, event)

    def putSlot(self, filename):
        downloadtask = QListWidgetItem(self.tasklist)
        self.tasklist.addItem(downloadtask)
        
        dlvlay = QHBoxLayout()
        listwidget = QWidget(self)
        typelabel = QLabel('Upload')
        dlfilename = QLabel(filename[:30])
        downloadprogress = QProgressBar(self)
        dlbutton = QPushButton('Pause')
        dlvlay.addWidget(typelabel, 1)
        dlvlay.addWidget(dlfilename, 2)
        dlvlay.addWidget(downloadprogress, 6)
        dlvlay.addWidget(dlbutton, 1)
        listwidget.setLayout(dlvlay)

        downloadtask.setSizeHint(listwidget.sizeHint())
        downloadtask.setFlags(downloadtask.flags() & ~Qt.ItemIsSelectable)
        self.tasklist.setItemWidget(downloadtask, listwidget)

        path = self.localfileModel.path/filename
        putPauseflag = {'stop':False, 'button':dlbutton,
                             'dlpb':downloadprogress, 'localname':path,
                             'remotefullpath':self.remotefileModel.path,
                             'remotename':filename}
        self.putFlags.append(putPauseflag)
        id = len(self.putFlags) - 1
        dlbutton.clicked.connect(self.putPauseSlot(id))
        worker = putWorker(self.ftp, path, filename, 0,
                           downloadprogress, self.putUpdatePBSlot, self.putFinishSlot,
                           putPauseflag, id)
        self.threadpool.start(worker)

    def putUpdatePBSlot(self, tup):
        tup[1].setValue(tup[0])

    def putFinishSlot(self, tup):
        new_code = 0
        if tup[1] == 1:
            new_code = 1
        putPauseflag = self.putFlags[tup[3]]
        self.printLog([tup[0], new_code], 'put ' + tup[2])
        if tup[1] != 1:
            self.remotefileModel.setupItem()
        if tup[1] == 0:
            putPauseflag['stop'] = True
            putPauseflag['button'].setEnabled(False)

    def putPauseSlot(self, id):
        def realPauseSlot():
            putPauseflag = self.putFlags[id]
            if not putPauseflag['stop']:
                putPauseflag['stop'] = True
                putPauseflag['button'].setText('Continue')
            else:
                putPauseflag['stop'] = False
                putPauseflag['button'].setText('Pause')
                localname = putPauseflag['localname']
                remotename = putPauseflag['remotename']
                remotefullpath = putPauseflag['remotefullpath']
                dlpb = putPauseflag['dlpb']
                info = self.remotefileModel.getinfo(remotefullpath, remotename)
                # if info['size']:
                try:
                    size = info['size']
                except:
                    size = 0
                worker = putWorker(self.ftp, localname, remotefullpath + '/' + remotename, size,
                                   dlpb, self.putUpdatePBSlot, self.putFinishSlot,
                                   putPauseflag, id)
                self.threadpool.start(worker)
        return realPauseSlot

    def doubleClickLocalFile(self, index):
        self.localfileModel.changeDir(index.data())
        self.localfileLine.setText(str(self.localfileModel.path))

    def getSlot(self, filename):
        remotefilename = filename
        remotefullpath = self.remotefileModel.path
        localfilename = str(self.localfileModel.path/filename)
        size = self.remotefileModel.file_info[remotefilename]['size']
        gettaskitem = QListWidgetItem(self.tasklist)
        self.tasklist.addItem(gettaskitem)
        
        dlvlay = QHBoxLayout()
        listwidget = QWidget(self)
        typelabel = QLabel('Download')
        dlfilename = QLabel(filename[:30])
        getpb = QProgressBar(self)
        dlbutton = QPushButton('Pause')
        dlvlay.addWidget(typelabel, 1)
        dlvlay.addWidget(dlfilename, 2)
        dlvlay.addWidget(getpb, 6)
        dlvlay.addWidget(dlbutton, 1)
        listwidget.setLayout(dlvlay)

        gettaskitem.setSizeHint(listwidget.sizeHint())
        gettaskitem.setFlags(gettaskitem.flags() & ~Qt.ItemIsSelectable)
        self.tasklist.setItemWidget(gettaskitem, listwidget)

        getPauseflag = {'stop':False, 'button':dlbutton,
                             'getpb':getpb, 'localname':localfilename,
                             'remotefullpath':remotefullpath,
                             'remotename':filename, 'size':size}

        self.getFlags.append(getPauseflag)
        id = len(self.getFlags) - 1
        dlbutton.clicked.connect(self.getPauseSlot(id))

        worker = getWorker(self.ftp, localfilename, remotefullpath + '/' + remotefilename,
                           False, getpb, self.getUpdatePBSlot, self.getFinishSlot, self.getFlags[id], id)
        self.threadpool.start(worker)

    def getUpdatePBSlot(self, tup):
        tup[1].setValue(tup[0])

    def getFinishSlot(self, tup):
        new_code = 0
        getPauseflag = self.getFlags[tup[3]]
        if tup[1] == 1:
            new_code = 1
        self.printLog([tup[0], new_code], 'get ' + tup[2])
        if tup[1] != 1:
            self.localfileModel.setupItem()
        if tup[1] == 0:
            getPauseflag['stop'] = True
            getPauseflag['button'].setEnabled(False)

    def getPauseSlot(self, id):
        def realPauseSlot():
            getPauseflag = self.getFlags[id]
            if not getPauseflag['stop']:
                getPauseflag['stop'] = True
                getPauseflag['button'].setText('Continue')
            else:
                getPauseflag['stop'] = False
                getPauseflag['button'].setText('Pause')
                localfilename = getPauseflag['localname']
                remotefullpath = getPauseflag['remotefullpath']
                remotename = getPauseflag['remotename']
                getpb = getPauseflag['getpb']
                worker = getWorker(self.ftp, localfilename, remotefullpath + '/' + remotename,
                                   True, getpb, self.getUpdatePBSlot, self.getFinishSlot, getPauseflag, id)
                self.threadpool.start(worker)
        return realPauseSlot

    def remoteMkdir(self):
        text, ok = QInputDialog.getText(self, 'New Directory', 'Enter new directory name:')
        if not ok:
            return
        mkdirresult = self.ftp.mkdir_handler(text)
        self.printLog(mkdirresult, 'mkdir '+text)
        if mkdirresult[1] == 0:
            self.remotefileModel.setupItem()

    def remoteRmdir(self, dirname):
        rmdirresult = self.ftp.rmdir_handler(dirname)
        self.printLog(rmdirresult, 'rmdir '+dirname)
        if rmdirresult[1] == 0:
            self.remotefileModel.setupItem()

    def remoteRename(self, oldname):
        newname, ok = QInputDialog.getText(self, 'Rename', 'Enter new file name:')
        if not ok:
            return
        renameresult = self.ftp.rename_handler(oldname, newname)
        self.printLog(renameresult, 'rename '+oldname+' '+newname)
        if renameresult[1] == 0:
            self.remotefileModel.setupItem()

    def doubleClickRemoteFile(self, index):
        self.remotefileModel.changeDir(index.data())
        self.remotefileLine.setText(str(self.remotefileModel.path))

    def connectClicked(self):
        if self.connectBtn.text() == 'Connect':
            host = self.hostInput.text()
            # TODO: more robust
            port = int(self.portInput.text())
            username = self.usernameInput.text()
            password = self.passwordInput.text()

            openresult = self.ftp.open_handler(host, port)
            self.printLog(openresult, 'open '+host+' '+str(port))
            if openresult[1] != 0:
                return
            loginresult = self.ftp.user_handler(username, password)
            self.printLog(loginresult, 'user '+username+' '+'*'*len(password))
            if loginresult[1] == 0:
                self.connectBtn.setText('Disconnect')
                self.remotefileModel.setupItem()
        else:
            byeresult = self.ftp.bye_handler()
            self.printLog(byeresult, 'bye')
            if byeresult[1] == 0:
                self.connectBtn.setText('Connect')
                self.remotefileModel.removeRows(0, self.remotefileModel.rowCount())

    def setBinary(self):
        if not self.ftp.open_status:
            return
        binresult = self.ftp.binary_handler()
        self.printLog(binresult, 'binary')
        if binresult[1] == 0:
            self.binaryAct.setChecked(True)

    def checkSystem(self):
        if self.ftp.open_status:
            sysresult = self.ftp.system_handler()
            self.printLog(sysresult, 'system')
            if sysresult[1] == 0:
                message = sysresult[0].strip('\r\n')
            else:
                message = 'Something wrong while receiving data.'
        else:
            message = 'No connection established.'
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Information)
        msg.setText("FTP Server System Information")
        msg.setInformativeText(message)
        msg.setWindowTitle("About-System")
        msg.setStandardButtons(QMessageBox.Ok)
        msg.exec_()

    def printLog(self, result, command):
        greenColor = QColor(46,139,87)
        blueColor = QColor(0, 0, 255)
        redColor = QColor(255, 0, 0)
        blackColor = QColor(0, 0, 0)
        self.setUpdatesEnabled(False)
        self.commandText.append("<span style='color:#4169E1'>&gt;ftp&nbsp;&nbsp;&nbsp;</span><span style='color:#2F4F4F'>"+command+"</span>")
        if result[1] == 0:
            self.commandText.setTextColor(greenColor)
        elif result[1] == 1:
            self.commandText.setTextColor(redColor)
        else:
            self.commandText.setTextColor(redColor)
        self.commandText.append(result[0].strip('\r\n'))
        self.setUpdatesEnabled(True)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = FTPGUI()
    sys.exit(app.exec_())