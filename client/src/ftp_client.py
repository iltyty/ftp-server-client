import wx
import os
import stat
import time
import threading
import cmd_handle
import wx.richtext

BG_COLOR = '#F1FDEF'

COLUMN_COUNT = 5
COLUMN_NAME_IDX = 0
COLUMN_TYPE_IDX = 1
COLUMN_SIZE_IDX = 2
COLUMN_MODE_IDX = 3
COLUMN_MODI_IDX = 4

LOCAL_FILE_LIST_ID  = 1
REMOTE_FILE_LIST_ID = 2

LOCAL_PATH_LABEL  = "Local path: "
REMOTE_PATH_LABEL = "Server path: "

CMD_CTRL_NAME  = "cmd_ctl"
HOST_CTRL_NAME = "host_ctl"
PORT_CTRL_NAME = "port_ctl"
USER_CTRL_NAME = "user_ctl"
PASS_CTRL_NAME = "pass_ctl"

BTN_PASV_NAME       = "btn_pasv"
BTN_PORT_NAME       = "btn_port"
BTN_CLEAR_NAME      = "btn_cls"
BTN_LC_DEL_NAME     = "btn_lc_del"
BTN_RM_DEL_NAME     = "btn_rm_del"
BTN_CONNECT_NAME    = "btn_cnt"
BTN_DISCONNECT_NAME = "btn_dis"
BTN_UPLOAD_NAME     = "btn_upload"
BTN_DOWNLOAD_NAME   = "btn_download"
BTN_CUSTOM_CMD_NAME = "enter_custom_cmd"

LOCAL_PATH_EDIT_NAME    = "local_path_edit"
REMOTE_PATH_EDIT_NAME   = "remote_path_edit"
LOCAL_PATH_LABEL_NAME   = "local_path_label"
REMOTE_PATH_LABEL_NAME  = "remote_path_label"


class BaseFileList(wx.ListCtrl):
    """Base file list"""

    def __init__(self, parent, id):
        wx.ListCtrl.__init__(self, parent=parent, id=id, style=wx.LC_REPORT)

        width = self.GetParent().GetParent().GetParent().GetClientSize().width /(2 * COLUMN_COUNT)


        self.InsertColumn(COLUMN_NAME_IDX, 'Name', width=width)
        self.InsertColumn(COLUMN_TYPE_IDX, 'Type', width=width)
        self.InsertColumn(COLUMN_SIZE_IDX, 'Size', width=width)
        self.InsertColumn(COLUMN_MODE_IDX, 'Mode', width=width)
        self.InsertColumn(COLUMN_MODI_IDX, 'Last Modified', width=width)

        imgs = [
            'imgs/folder.jpeg',
            'imgs/file.jpeg'
        ]
        self.il = wx.ImageList(16, 16)
        for i in imgs:
            self.il.Add(wx.Bitmap(i))
        self.SetImageList(self.il, wx.IMAGE_LIST_SMALL)

        # self.Bind(wx.EVT_SIZE, self.on_resize)

    def on_resize(self, event):
        width = self.GetParent().GetParent().GetParent().GetClientSize().width / (2 * COLUMN_COUNT) - 10
        for i in range(COLUMN_COUNT):
            self.SetColumnWidth(i, width)
        event.Skip()

class LocalFileList(BaseFileList):
    """
    File list in local directory.
    """

    def __init__(self, parent, id):
        BaseFileList.__init__(self, parent, id)


        self.refresh_list()

    def refresh_list(self):
        self.DeleteAllItems()
        files = os.listdir('.')

        self.InsertItem(0, '..')
        self.SetItem(0, 3, 'drwx------')
        self.SetItemImage(0, 0)
        self.SetItemBackgroundColour(0, BG_COLOR)

        j = 1
        for i in files:
            try:
                (name, ext) = os.path.splitext(i)
                ex = ext[1:]
                size = os.path.getsize(i)
                sec = os.path.getmtime(i)
                self.InsertItem(j, i)

                pathname = os.path.join(os.getcwd(), i)
                filemode = self.get_file_mode(pathname)
            except:
                continue
            if filemode.startswith('d'):
                self.SetItemImage(j, 0)
                self.SetItem(j, 1, 'directory')
            else:
                if ex:
                    self.SetItem(j, 1, ex)
                else:
                    self.SetItem(j, 1, 'file')
                self.SetItemImage(j, 1)
            self.SetItem(j, 2, str(size) + ' B')
            self.SetItem(j, 3, filemode)
            self.SetItem(j, 4, time.strftime('%Y-%m-%d %H:%M', time.localtime(sec)))

            if (j % 2) == 0:
                self.SetItemBackgroundColour(j, BG_COLOR)
            j += 1


    def get_file_mode(self, filepath):
        st = os.stat(filepath)
        modes = [
            stat.S_IRUSR, stat.S_IWUSR, stat.S_IXUSR,
            stat.S_IRGRP, stat.S_IWGRP, stat.S_IXGRP,
            stat.S_IROTH, stat.S_IWOTH, stat.S_IXOTH,
        ]
        mode = st.st_mode
        fullmode = ''
        fullmode += os.path.isdir(filepath) and 'd' or '-'

        for i in range(9):
            fullmode += bool(mode & modes[i]) and 'rwxrwxrwx'[i] or '-'
        return fullmode


class RemoteFileList(BaseFileList):
    """
    File list in remote directory
    """

    def __init__(self, parent, id):
        BaseFileList.__init__(self, parent, id)


class TaskList(wx.ListCtrl):
    """Task list in progress"""

    def __init__(self, parent, name):
        wx.ListCtrl.__init__(self, parent=parent, name=name, style=wx.LC_REPORT)

        width = self.GetParent().GetParent().GetParent().GetClientSize().width / 3

        self.InsertColumn(0, 'Name', width=width)
        self.InsertColumn(1, 'Path', width=width)
        self.InsertColumn(2, 'Progress', width=width)

        self.Bind(wx.EVT_SIZE, self.on_resize)

    def on_resize(self, event):
        width = self.GetParent().GetParent().GetParent().GetClientSize().width / 3
        self.SetColumnWidth(0, width)
        self.SetColumnWidth(1, width)
        self.SetColumnWidth(2, width)
        event.Skip()


class InfoPanel(wx.Panel):

    """
    Panel containing basic connection information.
    Including: host, port, username, password, connect, disconnect.
    """

    def __init__(self, parent):
        wx.Panel.__init__(self, parent)

        # create horizontal sizer
        hbs = wx.BoxSizer(wx.HORIZONTAL)
        hbs_host = wx.BoxSizer(wx.HORIZONTAL)
        hbs_port = wx.BoxSizer(wx.HORIZONTAL)
        hbs_user = wx.BoxSizer(wx.HORIZONTAL)
        hbs_pass = wx.BoxSizer(wx.HORIZONTAL)

        host_lbl = wx.StaticText(self, label="Host: ", name="host_lbl")
        port_lbl = wx.StaticText(self, label="Port: ", name="port_lbl")
        user_lbl = wx.StaticText(self, label="Username: ", name="user_lbl")
        pass_lbl = wx.StaticText(self, label="Password: ", name="pass_lbl")

        host_ctl = wx.TextCtrl(self, name=HOST_CTRL_NAME)
        port_ctl = wx.TextCtrl(self, name=PORT_CTRL_NAME)
        user_ctl = wx.TextCtrl(self, name=USER_CTRL_NAME)
        pass_ctl = wx.TextCtrl(self, name=PASS_CTRL_NAME, style=wx.TE_PASSWORD)

        btn_cnt = wx.Button(self, label="Connect", name=BTN_CONNECT_NAME)
        btn_dis = wx.Button(self, label="Disconnect", name=BTN_DISCONNECT_NAME)

        hbs_host.Add(host_lbl)
        hbs_host.Add(host_ctl, flag=wx.EXPAND)

        hbs_port.Add(port_lbl)
        hbs_port.Add(port_ctl, flag=wx.EXPAND)

        hbs_user.Add(user_lbl)
        hbs_user.Add(user_ctl, flag=wx.EXPAND)

        hbs_pass.Add(pass_lbl)
        hbs_pass.Add(pass_ctl, flag=wx.EXPAND)

        hbs.Add(hbs_host, border=20, flag=wx.UP | wx.LEFT)
        hbs.AddSpacer(40)
        hbs.Add(hbs_port, border=20, flag=wx.UP)
        hbs.AddSpacer(40)
        hbs.Add(hbs_user, border=20, flag=wx.UP)
        hbs.AddSpacer(40)
        hbs.Add(hbs_pass, border=20, flag=wx.UP | wx.RIGHT)
        hbs.AddSpacer(40)

        hbs.Add(btn_cnt, border=20, flag=wx.UP)
        hbs.AddSpacer(40)
        hbs.Add(btn_dis, border=20, flag=wx.UP | wx.RIGHT)

        self.SetSizer(hbs)


class ContentPanel(wx.Panel):

    """
    Panel containing main content of the program.
    Including: command input, local file list, current transfer task.
    """

    def __init__(self, parent):
        wx.Panel.__init__(self, parent)

        # local current directory
        cur_dir = os.getcwd()

        # create sizers
        vbs = wx.BoxSizer(wx.VERTICAL)
        vbs_lc = wx.BoxSizer(wx.VERTICAL)
        vbs_rm = wx.BoxSizer(wx.VERTICAL)
        hbs_lc = wx.BoxSizer(wx.HORIZONTAL)
        hbs_rm = wx.BoxSizer(wx.HORIZONTAL)
        hbs_file = wx.BoxSizer(wx.HORIZONTAL)

        # create UI widgets
        self.local_path = wx.StaticText(self, label=LOCAL_PATH_LABEL, name=LOCAL_PATH_LABEL_NAME)
        self.remote_path = wx.StaticText(self, label=REMOTE_PATH_LABEL, name=REMOTE_PATH_LABEL_NAME)
        # self.task_lbl = wx.StaticText(self, label="Task: ", name="task_lbl")

        self.lc_file_ctl = LocalFileList(self, id=LOCAL_FILE_LIST_ID)
        self.rm_file_ctl = RemoteFileList(self, id=REMOTE_FILE_LIST_ID)
        self.btn_lc_del = wx.Button(self, label="Delete", name=BTN_LC_DEL_NAME)
        self.btn_rm_del = wx.Button(self, label="Delete", name=BTN_RM_DEL_NAME)
        self.btn_upload = wx.Button(self, label="Upload", name=BTN_UPLOAD_NAME)
        self.btn_download = wx.Button(self, label="Download", name=BTN_DOWNLOAD_NAME)
        # self.task_ctl = TaskList(self, name="task_ctl")
        self.local_path_edit = wx.TextCtrl(self, name=LOCAL_PATH_EDIT_NAME, value=cur_dir, style=wx.TE_PROCESS_ENTER | wx.TE_AUTO_URL)
        self.remote_path_edit = wx.TextCtrl(self, name=REMOTE_PATH_EDIT_NAME, value='', style=wx.TE_PROCESS_ENTER)

        hbs_lc.Add(self.btn_upload, proportion=1, border=20, flag=wx.RIGHT)
        hbs_lc.Add(self.btn_lc_del, proportion=1, border=20, flag=wx.LEFT)

        hbs_rm.Add(self.btn_download, proportion=1, border=20, flag=wx.RIGHT)
        hbs_rm.Add(self.btn_rm_del, proportion=1, border=20, flag=wx.LEFT)

        vbs_lc.Add(self.local_path, proportion=1, border=20, flag=wx.LEFT)
        vbs_lc.Add(self.local_path_edit, proportion=1, border=20, flag=wx.EXPAND | wx.LEFT)
        vbs_lc.Add(self.lc_file_ctl, proportion=10, border=20, flag=wx.LEFT | wx.UP | wx.EXPAND)
        vbs_lc.Add(hbs_lc, proportion=1, border=20, flag=wx.UP | wx.BOTTOM | wx.LEFT)

        vbs_rm.Add(self.remote_path, proportion=1, border=0, flag=wx.LEFT)
        vbs_rm.Add(self.remote_path_edit, proportion=1, border=10, flag=wx.EXPAND | wx.RIGHT)
        vbs_rm.Add(self.rm_file_ctl, proportion=10, border=20, flag=wx.RIGHT | wx.UP | wx.EXPAND)
        vbs_rm.Add(hbs_rm, proportion=1, border=20, flag=wx.UP | wx.BOTTOM | wx.RIGHT)

        hbs_file.Add(vbs_lc, proportion=1, border=10, flag=wx.EXPAND | wx.RIGHT)
        hbs_file.Add(vbs_rm, proportion=1, border=10, flag=wx.EXPAND | wx.LEFT)

        vbs.Add(hbs_file, proportion=10, border=5)

        self.SetSizer(vbs)
        self.Bind(wx.EVT_SIZE, self.on_resize)
        self.Bind(wx.EVT_TEXT_ENTER, self.text_entered)

    def on_resize(self, event):
        width = self.GetSize().x // 2
        self.local_path_edit.SetSize(width, -1)
        self.remote_path_edit.SetSize(width,  -1)
        for i in range(COLUMN_COUNT):
            self.lc_file_ctl.SetColumnWidth(i, width // COLUMN_COUNT - 10)
            self.rm_file_ctl.SetColumnWidth(i, width // COLUMN_COUNT - 10)
        event.Skip()

    def text_entered(self, event):
        try:
            os.chdir(self.local_path_edit.GetValue())
        except FileNotFoundError:
            mlg = wx.MessageDialog(self, 'Invalid path!', 'Message Dialog')
            mlg.ShowModal()
            mlg.Destroy()
            self.local_path_edit.SetValue(os.getcwd())
            return
        self.lc_file_ctl.refresh_list()
        event.Skip()

    def right_clicked(self, event):
        popup_menu = wx.Menu()
        popup_menu.Append(-1, 'Enter custom command')
        event.Skip()

class CmdPanel(wx.Panel):
    """
    Command panel
    """

    def __init__(self, parent):
        wx.Panel.__init__(self, parent)

        vbs = wx.BoxSizer(wx.VERTICAL)
        hbs = wx.BoxSizer(wx.HORIZONTAL)

        self.btn_cmd = wx.Button(self, name=BTN_CUSTOM_CMD_NAME, label="Enter Command")
        self.btn_cls = wx.Button(self, name=BTN_CLEAR_NAME, label="Clear")
        self.btn_pasv = wx.Button(self, name=BTN_PASV_NAME, label="Set PASV")
        self.btn_port = wx.Button(self, name=BTN_PORT_NAME, label="Set PORT")

        self.cmd_lbl = wx.StaticText(self, label="Commands: ")
        self.cmd_ctl = wx.richtext.RichTextCtrl(self, name=CMD_CTRL_NAME, style=wx.richtext.RE_MULTILINE
                                                | wx.richtext.RE_READONLY | wx.richtext.RE_CENTRE_CARET)
        self.cmd_ctl.SetFont(wx.Font(wx.FontInfo(10).Family(wx.FONTFAMILY_SWISS)))

        hbs.Add(self.btn_cmd, proportion=1, border=10, flag=wx.RIGHT | wx.BOTTOM)
        hbs.Add(self.btn_cls, proportion=1, border=10, flag=wx.RIGHT  | wx.LEFT | wx.BOTTOM)
        hbs.Add(self.btn_pasv, proportion=1, border=10, flag=wx.RIGHT | wx.LEFT | wx.BOTTOM)
        hbs.Add(self.btn_port, proportion=1, border=10, flag=wx.RIGHT | wx.LEFT | wx.BOTTOM)

        vbs.Add(self.cmd_lbl, proportion=1, border=20, flag=wx.LEFT)
        vbs.Add(hbs, proportion=1, border=20, flag=wx.LEFT)
        vbs.Add(self.cmd_ctl, proportion=5, border=20, flag=wx.EXPAND | wx.LEFT)

        self.SetSizer(vbs)

class MainPanel(wx.Panel):
    """
    Main panel of the program.
    Including: InfoPanel, ContentPanel.
    """

    def __init__(self, parent):
        wx.Panel.__init__(self, parent)

        # create vertical box sizer
        vbs = wx.BoxSizer(wx.VERTICAL)

        # other sub panels
        info_panel = InfoPanel(self)
        cmd_panel  = CmdPanel(self)
        cont_panel = ContentPanel(self)

        # add sub panels to main panel
        vbs.Add(info_panel, border=20, proportion=1, flag=wx.EXPAND | wx.BOTTOM)
        vbs.Add(cmd_panel, border=20, proportion=3, flag=wx.EXPAND | wx.UP | wx.BOTTOM)
        vbs.Add(cont_panel, proportion=3, flag=wx.ALL | wx.EXPAND)

        self.SetSizer(vbs)


class FtpClient(wx.Frame):
    """Ftp GUI client"""

    def __init__(self, *args, **kw):
        wx.Frame.__init__(self, *args, **kw)

        # set panel
        self.Panel = MainPanel(self)
        self.SetSize(1200, 900)


def main():
    app = wx.App()
    ftp_client = FtpClient(None, title="Ftp Client")
    cmd_handler = cmd_handle.CmdHandler(ftp_client)

    ftp_client.PushEventHandler(cmd_handler)
    ftp_client.Show()
    try:
        app.MainLoop()
    except wx._core.wxAssertionError:
        return


if __name__ == '__main__':
    main()
