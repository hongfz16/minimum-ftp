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

class LocalFileSystemModel(QStandardItemModel):
    def __init__(self):
        super().__init__()
        self.path=Path('/home/hongfz')
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
            two_more = ['.', '..']
            for x in two_more:
                item = QStandardItem(QApplication.style().standardIcon(QStyle.SP_DirIcon), x)
                item.setEditable(False)
                self.appendRow(item)
                info = {}
                info['type'] = 'd'
                info['mod'] = ''
                info['link'] = 0
                info['owner'] = ''
                info['group'] = ''
                info['size'] = ''
                info['modify_time'] = ''
                info['name'] = x
                self.file_info[x] = info
            for x in result[2]:
                if x=='':
                    continue
                info = self.parse_linux_ls(x)
                if info['name'] == '.' or info['name'] == '..':
                    continue
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

    def getinfo(self, path, filename):
        if not self.ftp.login_status:
            return {}
        old_path = self.path
        result = self.ftp.cd_handler(path)
        self.gui.printLog(result, 'cd '+path)
        if result[1] != 0:
            return {}
        result = self.ftp.ls_handler()
        self.gui.printLog(result, 'ls')
        if result[1] != 0:
            return {}
        ret_val = {}
        for x in result[2][1:]:
            info = self.single_parse_service(x)
            if info['name'] == filename:
                ret_val = info
                break
        result = self.ftp.cd_handler(old_path)
        self.gui.printLog(result, 'cd '+old_path)
        return ret_val

    def updatePath(self):
        if not self.ftp.login_status:
            return
        result = self.ftp.pwd_handler()
        self.gui.printLog(result, 'pwd')
        if result[1] == 0:
            self.path = result[2]

    def parse_linux_ls(self, line):
        info = self.single_parse_service(line)
        self.file_info[info['name']] = info
        return info

    def single_parse_service(self, line):
        info = {}
        parsed = ' '.join(filter(lambda x: x, line.split(' ')))
        parsed = parsed.split(' ', 8)
        info['type'] = parsed[0][0]
        info['mod'] = parsed[0][1:]
        info['link'] = int(parsed[1])
        info['owner'] = parsed[2]
        info['group'] = parsed[3]
        info['size'] = int(parsed[4])
        info['modify_time'] = ' '.join(parsed[5:8])
        info['name'] = parsed[8]
        return info
