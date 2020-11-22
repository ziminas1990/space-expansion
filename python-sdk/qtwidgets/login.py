from typing import NamedTuple, Tuple, Optional

from PyQt5.QtWidgets import (
    QWidget,
    QDialog,
    QLabel,
    QLineEdit,
    QPushButton,
    QHBoxLayout,
    QGridLayout,
)


class LoginData(NamedTuple):
    local_ip: str
    local_port: int
    server_ip: str
    server_port: int
    login: str
    password: str


class LoginWidget(QWidget):

    def __init__(self,
                 default_server_ip_port: Optional[Tuple[str, int]] = None,
                 default_local_ip_port: Optional[Tuple[str, int]] = None,
                 default_login: Optional[str] = None,
                 *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.main_box = QGridLayout(self)

        self.lbl_server_ip_port = QLabel(self)
        self.lbl_server_ip_port.setText("Server IP:Port")
        self.linedit_server_ip = QLineEdit(self)
        self.linedit_server_port = QLineEdit(self)

        self.lbl_local_ip_port = QLabel(self)
        self.lbl_local_ip_port.setText("Local IP:Port")
        self.linedit_local_ip = QLineEdit(self)
        self.linedit_local_port = QLineEdit(self)

        self.lbl_login = QLabel(self)
        self.lbl_login.setText("Login")
        self.linedit_login = QLineEdit(self)

        self.lbl_password = QLabel(self)
        self.lbl_password.setText("Password")
        self.lineedit_password = QLineEdit(self)
        self.lineedit_password.setEchoMode(QLineEdit.Password)

        self.button_ok = QPushButton(self)
        self.button_cancel = QPushButton(self)
        self.button_ok.setText("GO!")
        self.button_cancel.setText("Cancel")

        self.bottom_box = QHBoxLayout()
        self.bottom_box.addStretch()
        self.bottom_box.addWidget(self.button_cancel)
        self.bottom_box.addWidget(self.button_ok)

        self.main_box.addWidget(self.lbl_server_ip_port, 0, 0)
        self.main_box.addWidget(self.linedit_server_ip, 0, 1)
        self.main_box.addWidget(self.linedit_server_port, 0, 2)
        self.main_box.addWidget(self.lbl_local_ip_port, 1, 0)
        self.main_box.addWidget(self.linedit_local_ip, 1, 1)
        self.main_box.addWidget(self.linedit_local_port, 1, 2)
        self.main_box.addWidget(self.lbl_login, 2, 0)
        self.main_box.addWidget(self.linedit_login, 2, 1, 1, 2)
        self.main_box.addWidget(self.lbl_password, 3, 0)
        self.main_box.addWidget(self.lineedit_password, 3, 1, 1, 2)
        self.main_box.addLayout(self.bottom_box, 4, 0, 1, 3)

        # default values
        if default_local_ip_port is not None:
            self.linedit_local_ip.setText(default_local_ip_port[0])
            self.linedit_local_port.setText(str(default_local_ip_port[1]))

        if default_server_ip_port is not None:
            self.linedit_server_ip.setText(default_server_ip_port[0])
            self.linedit_server_port.setText(str(default_server_ip_port[1]))

        if default_login:
            self.linedit_login.setText(default_login)

    def get_ok_button(self) -> QPushButton:
        return self.button_ok

    def get_cancel_button(self) -> QPushButton:
        return self.button_cancel

    def get_data(self) -> LoginData:
        return LoginData(
            local_ip=self.linedit_local_ip.text(),
            local_port=int(self.linedit_local_port.text()),
            server_ip=self.linedit_server_ip.text(),
            server_port=int(self.linedit_server_port.text()),
            login=self.linedit_login.text(),
            password=self.lineedit_password.text()
        )


class LoginDialog(QDialog):
    def __init__(self,
                 default_server_ip_port: Optional[Tuple[str, int]] = None,
                 default_local_ip_port: Optional[Tuple[str, int]] = None,
                 default_login: Optional[str] = None,
                 *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.widget = LoginWidget(default_server_ip_port,
                                  default_local_ip_port,
                                  default_login,
                                  self)

        self.main_box = QGridLayout(self)
        self.main_box.addWidget(self.widget, 0, 0)

        self.widget.get_ok_button().clicked.connect(self.accept)
        self.widget.get_cancel_button().clicked.connect(self.reject)

    def get_data(self) -> LoginData:
        return self.widget.get_data()