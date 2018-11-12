import sys
import os
from pathlib import Path
from PyQt5.QtCore import (Qt, QDir, QTimer, QEvent)
from PyQt5.QtWidgets import (QMainWindow, QApplication,
                             QDesktopWidget, QAction, qApp,
                             QMenu, QGridLayout, QPushButton,
                             QWidget, QLabel, QLineEdit,
                             QTextEdit, QLCDNumber, QSlider,
                             QVBoxLayout, QFileDialog, QListView,
                             QHBoxLayout, QSplitter, QFrame,
                             QFileSystemModel, QStyle, QPlainTextEdit,
                             QMessageBox, QInputDialog)
from PyQt5.QtGui import (QStandardItemModel, QIcon, QStandardItem,
                         QTextCursor, QColor, QGuiApplication)
from mFTP import mFTP

class FTPGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.debug = True
        self.ftp = mFTP()
        if self.debug:
            self.ftp.open_handler('127.0.0.1', 8000)
            self.ftp.user_handler('anonymous', 'password')
        self.initUI()

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
        path = self.localfileModel.path/filename
        putresult = self.ftp.put_handler(path, filename)
        self.printLog(putresult, 'put ' + filename)
        if putresult[1] == 0:
            self.remotefileModel.setupItem()

    def doubleClickLocalFile(self, index):
        self.localfileModel.changeDir(index.data())
        self.localfileLine.setText(str(self.localfileModel.path))

    def getSlot(self, filename):
        localfilename = str(self.localfileModel.path/filename)
        getresult = self.ftp.get_handler(filename, localfilename)
        self.printLog(getresult, 'get ' + filename)
        if getresult[1] == 0:
            self.localfileModel.setupItem()

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
        # self.commandText.setTextColor(greenColor)
        # self.commandText.append('>ftp '+command)
        self.setUpdatesEnabled(False)
        self.commandText.append("<span style='color:#4169E1'>&gt;ftp&nbsp;&nbsp;&nbsp;</span><span style='color:#2F4F4F'>"+command+"</span>")
        if result[1] == 0:
            # message = "<p style='color:#008000'>" + message + "</p>"
            self.commandText.setTextColor(greenColor)
        elif result[1] == 1:
            # message = "<p style='color:#0000FF'>" + message + "</p>"
            self.commandText.setTextColor(redColor)
        else:
            # message = "<p style='color:#FF0000'>" + message + "</p>"
            self.commandText.setTextColor(redColor)
        # self.commandText.moveCursor(QTextCursor.End)
        # self.commandText.insertPlainText(result[0])
        self.commandText.append(result[0].strip('\r\n'))
        self.setUpdatesEnabled(True)
        # QGuiApplication.processEvents()
        # self.commandText.update()
        # self.commandText.appendHtml(message)

class LocalFileSystemModel(QStandardItemModel):
    def __init__(self):
        super().__init__()
        self.path=Path('/')
        self.setupItem()

    def setupItem(self):
        self.removeRows(0, self.rowCount())
        two_more = ['.', '..']
        for x in two_more:
            item = QStandardItem(QApplication.style().standardIcon(QStyle.SP_DirIcon), x)
            item.setEditable(False)
            self.appendRow(item)
        for x in self.path.iterdir():
            if x.is_dir() and x.name[0]!='.':
                item = QStandardItem(QApplication.style().standardIcon(QStyle.SP_DirIcon), x.name)
                item.setEditable(False)
                self.appendRow(item)
        for x in self.path.iterdir():
            if x.is_file() and x.name[0]!='.':
                item = QStandardItem(QApplication.style().standardIcon(QStyle.SP_FileIcon), x.name)
                item.setEditable(False)
                self.appendRow(item)

    def changeDir(self, name):
        tmp_path = self.path/name
        if tmp_path.exists() and tmp_path.is_dir():
            self.path = tmp_path.resolve()
            # self.removeRows(0, self.rowCount())
            self.setupItem()

class RemoteFileSystemModel(QStandardItemModel):
    def __init__(self, ftp, gui):
        super().__init__()
        self.ftp = ftp
        self.path = '/'
        self.gui = gui
        self.file_info = {}
        self.setupItem()

    def setupItem(self):
        if not self.ftp.login_status:
            return
        self.file_info = {}
        self.removeRows(0, self.rowCount())
        self.updatePath()
        result = self.ftp.ls_handler()
        self.gui.printLog(result, 'ls')
        if result[1] == 0:
            for x in result[2][1:]:
                info = self.parse_linux_ls(x)
                if info['type'] == 'd':
                    item = QStandardItem(QApplication.style().standardIcon(QStyle.SP_DirIcon), info['name'])
                else:
                    item = QStandardItem(QApplication.style().standardIcon(QStyle.SP_FileIcon), info['name'])
                item.setEditable(False)
                self.appendRow(item)

    def changeDir(self, name):
        if not self.ftp.login_status:
            return
        result = self.ftp.cd_handler(name)
        self.gui.printLog(result, 'cd '+name)
        if result[1] == 0:
            # self.removeRows(0, self.rowCount())
            self.setupItem()

    def updatePath(self):
        if not self.ftp.login_status:
            return
        result = self.ftp.pwd_handler()
        self.gui.printLog(result, 'pwd')
        if result[1] == 0:
            self.path = result[2]

    def parse_linux_ls(self, line):
        info = {}
        parsed = ' '.join(filter(lambda x: x, line.split(' ')))
        parsed = parsed.split(' ')
        info['type'] = parsed[0][0]
        info['mod'] = parsed[0][1:]
        info['link'] = int(parsed[1])
        info['owner'] = parsed[2]
        info['group'] = parsed[3]
        info['size'] = int(parsed[4])
        info['modify_time'] = ' '.join(parsed[5:8])
        info['name'] = parsed[8]
        # self.file_info.append(info)
        self.file_info[info['name']] = info
        return info

if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = FTPGUI()
    sys.exit(app.exec_())