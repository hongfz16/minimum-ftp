import sys
import os
from pathlib import Path
from PyQt5.QtCore import (Qt, QDir, QTimer, QEvent, pyqtSignal,
                          QRunnable, pyqtSlot, QObject)
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

class WorkerSignals(QObject):
    updatesignal = pyqtSignal(tuple)
    finishsignal = pyqtSignal(tuple)

class putWorker(QRunnable):
    def __init__(self, ftp, localname, remotename, skip_n, dlpb,
                 putUpdatePBSlot, putFinishSlot, pauseFlag):
        super(putWorker, self).__init__()
        self.ftp = ftp
        self.localname = localname
        self.remotename = remotename
        self.skip_n = skip_n
        self.dlpb = dlpb
        self.putUpdatePBSlot = putUpdatePBSlot
        self.putFinishSlot = putFinishSlot
        self.signals = WorkerSignals()
        self.pauseFlag = pauseFlag

    def put_update_callback(self, percentage):
        self.signals.updatesignal.emit((percentage, self.dlpb))

    @pyqtSlot()
    def run(self):
        self.signals.updatesignal.connect(self.putUpdatePBSlot)
        self.signals.finishsignal.connect(self.putFinishSlot)
        
        response, code = self.ftp.put_handler_callback(self.localname, self.remotename, self.skip_n,
                                                       self.put_update_callback, self.pauseFlag)

        self.signals.finishsignal.emit((response, code, self.remotename))

class getWorker(QRunnable):
    def __init__(self, ftp, localname, remotename, skip_flag, ulpb,
                 getUpdatePBSlot, getFinishSlot, pauseFlag):
        super(getWorker, self).__init__()
        self.ftp = ftp
        self.localname = localname
        self.remotename = remotename
        self.skip_flag = skip_flag
        self.ulpb = ulpb
        self.getUpdatePBSlot = getUpdatePBSlot
        self.getFinishSlot = getFinishSlot
        self.signals = WorkerSignals()
        self.pauseFlag = pauseFlag

    def get_update_callback(self, percentage):
        self.signals.updatesignal.emit((percentage, self.ulpb))

    @pyqtSlot()
    def run(self):
        self.signals.updatesignal.connect(self.getUpdatePBSlot)
        self.signals.finishsignal.connect(self.getFinishSlot)
        if self.skip_flag == False:
            response, code = self.ftp.get_handler_callback(self.remotename, self.localname,
                                                           self.get_update_callback, self.pauseFlag)
        else:
            response, code = self.ftp.restart_download_callback(self.remotename, self.localname,
                                                                self.get_update_callback, self.pauseFlag)

        self.signals.finishsignal.emit((response, code, self.remotename))
