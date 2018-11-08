import sys
import os
from pathlib import Path
from PyQt5.QtCore import (Qt, QDir, QTimer)
from PyQt5.QtWidgets import (QMainWindow, QApplication,
                             QDesktopWidget, QAction, qApp,
                             QMenu, QGridLayout, QPushButton,
                             QWidget, QLabel, QLineEdit,
                             QTextEdit, QLCDNumber, QSlider,
                             QVBoxLayout, QFileDialog, QListView,
                             QHBoxLayout, QSplitter, QFrame,
                             QFileSystemModel, QStyle)
from PyQt5.QtGui import QStandardItemModel, QIcon, QStandardItem

class FTPGUI(QMainWindow):
    def __init__(self):
        super().__init__()

        self.initUI()

    def initUI(self):
        self.centerwidget = QWidget(self)
        self.setCentralWidget(self.centerwidget)
        self.setupLayout(self.centerwidget)

        self.setupWidget()

        self.resize(1200, 960)
        self.center()
        self.setWindowTitle('Playground')
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

        self.connectBtn = QPushButton('Connect', self)
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

        localpathlay.addWidget(localfileLabel)
        localpathlay.addWidget(self.localfileLine)
        remotepathlay.addWidget(remotefileLabel)
        remotepathlay.addWidget(self.remotefileLine)
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

    def setupWidget(self):
        self.commandText.setReadOnly(True)
        self.localfileLine.setReadOnly(True)
        self.remotefileLine.setReadOnly(True)

        self.fileModel = LocalFileSystemModel()
        self.localfileList.setModel(self.fileModel)
        self.localfileList.doubleClicked.connect(self.on_item_changed)
        self.localfileLine.setText(str(self.fileModel.path))

    def on_item_changed(self, index):
        self.fileModel.changeDir(index.data())
        self.localfileLine.setText(str(self.fileModel.path))

class LocalFileSystemModel(QStandardItemModel):
    def __init__(self):
        super().__init__()
        self.path=Path('/')
        self.setupItem()

    def setupItem(self):
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
            self.removeRows(0, self.rowCount())
            self.setupItem()



if __name__ == '__main__':

    app = QApplication(sys.argv)
    ex = FTPGUI()
    sys.exit(app.exec_())