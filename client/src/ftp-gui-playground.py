import sys
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import (QMainWindow, QApplication,
                             QDesktopWidget, QAction, qApp,
                             QMenu, QGridLayout, QPushButton,
                             QWidget, QLabel, QLineEdit,
                             QTextEdit, QLCDNumber, QSlider,
                             QVBoxLayout, QFileDialog)
from PyQt5.QtGui import QIcon

class Example(QMainWindow):

    def __init__(self):
        super().__init__()

        self.initUI()

    def initUI(self):
        self.widget = QWidget(self)
        self.setCentralWidget(self.widget)
        self.setupmenu()
        self.addToolbar()
        self.setupGrid(self.widget)
        self.resize(640, 320)
        self.center()
        self.setWindowTitle('Playground')
        self.show()


    def setupmenu(self):
        self.statusbar = self.statusBar()
        self.statusbar.showMessage('Ready')
        menubar = self.menuBar()
        menubar.setNativeMenuBar(False)
        fileMenu = menubar.addMenu('File')
        viewMenu = menubar.addMenu('View')

        exitAct = QAction(QIcon('icon.png'), '&Exit', self)        
        exitAct.setShortcut('Ctrl+Q')
        exitAct.setStatusTip('Exit application')
        exitAct.triggered.connect(qApp.quit)

        impMenu = QMenu('Import', self)
        impAct = QAction('Import mail', self)
        impMenu.addAction(impAct)
        newAct = QAction('New', self)
        newAct.triggered.connect(self.showFileDialog)

        viewStatAct = QAction('View statusbar', self, checkable=True)
        viewStatAct.setStatusTip('View statusbar')
        viewStatAct.setChecked(True)
        viewStatAct.triggered.connect(self.toggleMenu)

        fileMenu.addAction(newAct)
        fileMenu.addMenu(impMenu)
        fileMenu.addAction(exitAct)

        viewMenu.addAction(viewStatAct)

    def toggleMenu(self, state):
        if state:
            self.statusbar.show()
        else:
            self.statusbar.hide()

    def center(self):

        qr = self.frameGeometry()
        cp = QDesktopWidget().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())

    def setupGrid(self, wid):
        title = QLabel('Title')
        author = QLabel('Author')
        review = QLabel('Review')

        titleEdit = QLineEdit()
        authorEdit = QLineEdit()
        reviewEdit = QTextEdit()

        grid = QGridLayout()
        grid.setSpacing(10)

        grid.addWidget(title, 1, 0)
        grid.addWidget(titleEdit, 1, 1)

        grid.addWidget(author, 2, 0)
        grid.addWidget(authorEdit, 2, 1)

        grid.addWidget(review, 3, 0)
        grid.addWidget(reviewEdit, 3, 1, 5, 1)

        lcd = QLCDNumber(self)
        sld = QSlider(Qt.Horizontal, self)

        vbox = QVBoxLayout()
        vbox.addWidget(lcd)
        vbox.addWidget(sld)

        grid.addLayout(vbox, 8, 0, 5, 2)
        sld.valueChanged.connect(lcd.display)

        wid.setLayout(grid)

    def addToolbar(self):
        exitAct = QAction(QIcon('icon.png'), 'Exit', self)
        exitAct.setShortcut('Ctrl+Q')
        exitAct.triggered.connect(qApp.quit)

        self.toolbar = self.addToolBar('Exit')
        self.toolbar.addAction(exitAct)

    def contextMenuEvent(self, event):
        cmenu = QMenu(self)
        newAct = cmenu.addAction("New")
        opnAct = cmenu.addAction("Open")
        quitAct = cmenu.addAction("Quit")
        action = cmenu.exec_(self.mapToGlobal(event.pos()))

        if action == quitAct:
            qApp.quit()

    def keyPressEvent(self, e):
        if e.key() == Qt.Key_Escape:
            self.close()

    def showFileDialog(self):
        fname = QFileDialog.getOpenFileName(self, 'Open file', '/')
        if fname[0]:
            print(fname)


if __name__ == '__main__':

    app = QApplication(sys.argv)
    ex = Example()
    sys.exit(app.exec_())

# import sys

# from PyQt5.QtWidgets import *
# from PyQt5.QtCore import *

# class Widget(QWidget):
#     def __init__(self, *args, **kwargs):
#         QWidget.__init__(self, *args, **kwargs)
#         hlay = QHBoxLayout(self)
#         self.treeview = QTreeView()
#         self.listview = QListView()
#         hlay.addWidget(self.treeview)
#         hlay.addWidget(self.listview)

#         path = QDir.rootPath()

#         self.dirModel = QFileSystemModel()
#         self.dirModel.setRootPath(QDir.rootPath())
#         self.dirModel.setFilter(QDir.NoDotAndDotDot | QDir.AllDirs)

#         self.fileModel = QFileSystemModel()
#         self.fileModel.setFilter(QDir.NoDotAndDotDot |  QDir.Files)

#         self.treeview.setModel(self.dirModel)
#         self.listview.setModel(self.fileModel)

#         self.treeview.setRootIndex(self.dirModel.index(path))
#         self.listview.setRootIndex(self.fileModel.index(path))

#         self.treeview.clicked.connect(self.on_clicked)

#     def on_clicked(self, index):
#         path = self.dirModel.fileInfo(index).absoluteFilePath()
#         self.listview.setRootIndex(self.fileModel.setRootPath(path))


# if __name__ == '__main__':
#     app = QApplication(sys.argv)
#     w = Widget()
#     w.show()
#     sys.exit(app.exec_())