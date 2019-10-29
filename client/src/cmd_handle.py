import os
import re
import wx
import threading
import socket
import ftp_client as fc

class Error(Exception): pass
class error_reply(Error): pass
class error_proto(Error): pass

MAX_BUF_SIZ = 2048

montonum = {
    'Jan': '01',
    'Feb': '02',
    'Mar': '03',
    'Apr': '04',
    'May': '05',
    'Jun': '06',
    'Jul': '07',
    'Aug': '08',
    'Sep': '09',
    'Oct': '10',
    'Nov': '11',
    'Dec': '12',
}

_227_re = re.compile(r'(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)', re.ASCII)

def parse227(resp):
    '''Parse the '227' response for PASV request.
    Raises error_proto if it does not contain '(h1,h2,h3,h4,p1,p2)'
    Return ('host.addr.as.numbers', port#) tuple.'''

    if resp[:3] != '227':
        raise error_reply(resp)
    res = _227_re.search(resp)
    if not res:
        raise error_proto(resp)
    args = res.groups()
    host = '.'.join(args[:4])
    port = (int(args[4]) * 256) + int(args[5])
    return host, port

def parse257(resp):
    '''Parse the '257' response for PWD request.'''

    if resp[:3] != '257':
        raise error_reply(resp)
    if resp[3:5] != ' "':
        return ''
    dirname = ''
    i = 5
    n = len(resp)
    while i < n:
        c = resp[i]
        i = i+1
        if c == '"':
            if i >= n or resp[i] != '"':
                break
            i = i+1
        dirname = dirname + c
    return dirname

_list_re = '^(?P<attrs>\
(?P<mode>([ldD-])([\-r][\-w][\-xst]){3})\s+\
(?P<filecode>(\d+))\s+\
(?P<user>(\w+))\s+\
(?P<group>(\w+))\s+)\
(?P<size>([0-9]{1,}))\s+\
(?P<timestamp>\
((?P<monthday>\
((?P<month>(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec))\s\
(?P<day>([0-9\s]{2}))))\s\
(\s(?P<year>([0-9]{4}))|\
(?P<time>([0-9]{2}\:[0-9]{2})))))\s+\
(?P<name>.+)$'
_list_re = re.compile(_list_re, re.ASCII)

def parselist(resp):
    """
    Parse the 'LIST' response.
    Including dir, attrbutes, size, timestamp and filename
    Returns a dict type value containg all these information(if existed)
    """

    global _list_re
    resp = resp.strip()
    if not resp or 'total ' in resp:
        return None
    res = _list_re.search(resp)
    if not res:
        raise error_proto(resp)
    ret = {
        'mode': '',
        'size': '',
        'timestamp': '',
        'month': '',
        'day': '',
        'time': '',
        'year': '',
        'name': '',
    }
    for key in ret.keys():
        ret[key] = res.group(key)
    return ret

def parse_timestamp(res):
    """
    Parse the timestamp returned by parselist['timestamp']
    """
    month = res['month']
    day   = res['day']
    if not (month or day):
        return ''

    # format: month-day-year
    year = res['year']
    month = montonum[month]
    time = res['time']
    if year:
        res['timestamp'] = '{:}-{:}-{:}'.format(year, month, day)
    # format: month-day-time
    elif time:
        res['timestamp'] = '2019-{:}-{:} {:}'.format(month, day, time)


class CmdHandler(wx.EvtHandler):
    """command handler"""

    def __init__(self, ftp_client):
        super().__init__()


        self.pasvmode = True
        self.pwd = ''

        self.ftp_client = ftp_client
        self.cmd_ctl  = ftp_client.FindWindowByName(fc.CMD_CTRL_NAME)
        self.host_ctl = ftp_client.FindWindowByName(fc.HOST_CTRL_NAME)
        self.port_ctl = ftp_client.FindWindowByName(fc.PORT_CTRL_NAME)
        self.user_ctl = ftp_client.FindWindowByName(fc.USER_CTRL_NAME)
        self.pass_ctl = ftp_client.FindWindowByName(fc.PASS_CTRL_NAME)
        self.btn_cnt  = ftp_client.FindWindowByName(fc.BTN_CONNECT_NAME)
        self.btn_dis  = ftp_client.FindWindowByName(fc.BTN_DISCONNECT_NAME)
        self.rm_files = ftp_client.FindWindowById(fc.REMOTE_FILE_LIST_ID)
        self.lc_files = ftp_client.FindWindowById(fc.LOCAL_FILE_LIST_ID)
        self.lc_path_edit = ftp_client.FindWindowByName(fc.LOCAL_PATH_EDIT_NAME)
        self.rm_path_edit = ftp_client.FindWindowByName(fc.REMOTE_PATH_EDIT_NAME)

        self.Bind(wx.EVT_BUTTON, self.btn_clicked)
        self.Bind(wx.EVT_TEXT_ENTER, self.text_entered)
        self.lc_files.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.lc_double_click)
        self.rm_files.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.rm_double_click)

    def lc_double_click(self, event):
        index = self.lc_files.GetFocusedItem()
        itemname = self.lc_files.GetItem(index, fc.COLUMN_NAME_IDX)
        itemmode = self.lc_files.GetItem(index, fc.COLUMN_MODE_IDX)
        if itemmode.GetText()[0] == 'd':
            newdir = os.path.join(os.getcwd(), itemname.GetText())
            os.chdir(newdir)
            self.lc_files.refresh_list()
            self.lc_path_edit.SetValue(os.getcwd())
        else:
            filename = itemname.GetText()
            self.upload(filename)


    def rm_double_click(self, event):
        index = self.rm_files.GetFocusedItem()
        itemname = self.rm_files.GetItem(index, fc.COLUMN_NAME_IDX)
        itemmode = self.rm_files.GetItem(index, fc.COLUMN_MODE_IDX)
        # directory
        if itemmode.GetText()[0] == 'd':
            # new_dir = os.path.join(self.pwd, itemname.GetText())
            resp = self.sendcmd('CWD ' + itemname.GetText())
            if resp[:1] == '2':
                self.pwd = parse257(self.sendcmd('PWD'))
                self.rm_path_edit.SetValue(self.pwd)
                self.refresh_rm_list()
        # file, auto-download when double clicked
        else:
            filename = itemname.GetText()
            self.download(filename)


    def show_msg_dialog(self, content, title='Message Dialog'):
        mlg = wx.MessageDialog(self.ftp_client, content, title)
        mlg.ShowModal()
        mlg.Destroy()


    def btn_clicked(self, event):
        """button connect or disconnect clicked"""
        name = event.GetEventObject().GetName()
        if name == fc.BTN_CONNECT_NAME:
            if not self.host_ctl.GetValue() or not self.port_ctl.GetValue():
                self.show_msg_dialog('Invalid host or port address!')
                return
            try:
                self.port = int(self.port_ctl.GetValue())
            except ValueError:
                self.show_msg_dialog('Invalid port address!')
                return
            if self.port < 0 or self.port > 65535:
                self.show_msg_dialog('Invalid port address!')
                return

            self.host = self.host_ctl.GetValue()
            self.connect_to_server()
        elif name == fc.BTN_CLEAR_NAME:
            self.cmd_ctl.Clear()
        elif name == fc.BTN_PASV_NAME:
            self.setpasv(True)
            self.show_msg_dialog('Data transfer mode set to PASV!')
        elif name == fc.BTN_PORT_NAME:
            self.setpasv(False)
            self.show_msg_dialog('Data transfer mode set to PORT!')
        elif name == fc.BTN_LC_DEL_NAME:
            index = self.lc_files.GetFocusedItem()
            item = self.lc_files.GetItem(index)
            path = os.path.join(os.getcwd(), item.GetText())
            if os.path.isdir(path):
                os.removedirs(path)
            else:
                os.remove(path)
            self.lc_files.refresh_list()
        elif name == fc.BTN_RM_DEL_NAME:
            index = self.rm_files.GetFocusedItem()
            item = self.rm_files.GetItem(index)
            resp = self.sendcmd('DELE ' + item.GetText())
            self.refresh_rm_list()
        elif name == fc.BTN_CUSTOM_CMD_NAME:
            dlg = wx.TextEntryDialog(self.ftp_client, 'Enter custom command: ', 'Enter command dialog')
            if dlg.ShowModal() == wx.ID_OK:
                cmd = dlg.GetValue()
                resp = self.sendcmd(cmd)
            dlg.Destroy()
        elif name == fc.BTN_DISCONNECT_NAME:
            resp = self.sendcmd('QUIT')
            self.add_status('Disconnected from server.')
            self.control_sock.close()
        elif name == fc.BTN_UPLOAD_NAME:
            index = self.lc_files.GetFocusedItem()
            item = self.lc_files.GetItem(index)
            if not item:
                self.show_msg_dialog('Please select one file to upload!')
            filename = item.GetText()
            self.upload(filename)
        elif name == fc.BTN_DOWNLOAD_NAME:
            index = self.rm_files.GetFocusedItem()
            item = self.rm_files.GetItem(index)
            if not item:
                self.show_msg_dialog('Please select one file to download!')
            filename = item.GetText()
            self.download(filename)


    def download(self, filename):
        if os.path.exists(filename):
            choices = ["overwrite","append","resume"]
            dlg = wx.MultiChoiceDialog(self.ftp_client, 'Choose download mode', 'Mode', choices)
            if dlg.ShowModal() == wx.ID_OK:
                se = dlg.GetSelections()
                if len(se) != 1:
                    self.show_msg_dialog('Please select one download mode!')
                    return
                elif se[0] == 0:
                    os.remove(filename)
                    self.filefd = os.open(filename, os.O_WRONLY | os.O_CREAT)
                elif se[0] == 1:
                    self.filefd = os.open(filename, os.O_WRONLY | os.O_APPEND)
                elif se[0] == 2:
                    self.filefd = os.open(filename, os.O_WRONLY | os.O_APPEND)
                    self.sendcmd('REST {:}'.format(os.path.getsize(filename)))

                self.add_status('Start download of file "{:}"'.format(filename))
                resp = self.get_data_binary('RETR ' + filename, self.cmdretr)
                self.add_response(resp)
                self.add_status('File "{:}" downloaded successfully.'.format(filename))
                self.lc_files.refresh_list()
                os.close(self.filefd)
            dlg.Destroy()
            return
        try:
            self.filefd = os.open(filename, os.O_WRONLY | os.O_CREAT)
        except IsADirectoryError:
            self.show_msg_dialog("You've selected one directory!")
            return
        self.add_status('Start download of file "{:}"'.format(filename))
        resp = self.get_data_binary('RETR ' + filename, self.cmdretr)
        self.add_response(resp)
        self.add_status('File "{:}" downloaded successfully.'.format(filename))
        self.lc_files.refresh_list()
        os.close(self.filefd)


    def upload(self, filename):
        try:
            # self.filefd = os.open(filename, os.O_RDONLY)
            self.filefd = open(filename, 'rb')
        except IsADirectoryError:
            self.show_msg_dialog("You've selected one directory!")
            return
        self.add_status('Start upload of file "{:}"'.format(filename))
        resp = self.cmdstor(self.filefd, 'STOR ' + filename)
        self.add_response(resp)
        self.refresh_rm_list()
        # os.close(self.filefd)
        self.filefd.close()


    def text_entered(self, event):
        """text entered in frame, namely command entered."""

        name = event.GetEventObject().GetName()
        if name == fc.REMOTE_PATH_EDIT_NAME:
            resp = self.sendcmd('CWD ' + self.rm_path_edit.GetValue())
            if resp[:3] == '550':
                mlg = wx.MessageDialog(self.ftp_client, 'Invalid path!', 'Message dialog')
                mlg.ShowModal()
                mlg.Destroy()
                self.rm_path_edit.SetValue(self.pwd)
            else:
                resp = parse257(self.sendcmd('PWD'))
                self.pwd = resp
                self.refresh_rm_list()

        event.Skip()


    def add_status(self, text):
        """Append status prompt to cmd_ctl"""
        self.cmd_ctl.BeginTextColour(wx.BLACK)
        self.cmd_ctl.WriteText('Status:\t' + text)
        self.cmd_ctl.EndTextColour()
        self.cmd_ctl.Newline()
        self.cmd_ctl.ShowPosition(self.cmd_ctl.GetCaretPosition())


    def add_error(self, text):
        """Append error prompt to cmd_ctl"""
        self.cmd_ctl.BeginTextColour(wx.RED)
        self.cmd_ctl.WriteText('Error:\t' + text)
        self.cmd_ctl.EndTextColour()
        self.cmd_ctl.Newline()
        self.cmd_ctl.ShowPosition(self.cmd_ctl.GetCaretPosition())


    def add_command(self, text):
        """Append command prompt to cmd_ctl"""
        self.cmd_ctl.BeginTextColour(wx.Colour(95, 181, 81))
        self.cmd_ctl.WriteText('Command:\t' + text)
        self.cmd_ctl.EndTextColour()
        self.cmd_ctl.Newline()
        self.cmd_ctl.ShowPosition(self.cmd_ctl.GetCaretPosition())


    def add_response(self, text):
        """Append response prompt to cmd_ctl"""
        self.cmd_ctl.BeginTextColour(wx.Colour(51, 153, 255))
        self.cmd_ctl.WriteText('Response:\t' + text)
        self.cmd_ctl.EndTextColour()
        self.cmd_ctl.Newline()
        self.cmd_ctl.ShowPosition(self.cmd_ctl.GetCaretPosition())


    def connect_to_server(self):
        self.add_status('Connecting to {:}:{:}.'.format(self.host, self.port))
        try:
            self.control_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.control_sock.settimeout(10)
            self.control_sock.connect((self.host, self.port))
        except ConnectionError:
            self.add_error('Connection attempt failed.')
        except OSError:
            self.add_error()('Bad file descriptor.')
        except socket.timeout:
            self.add_error('Connection timeout in 10 seconds.')
        else:
            self.add_status('Connection established, waiting for welcome message...')
            resp = self.getresp(self.control_sock)
            if resp[0] == '2':
                self.send_init_cmds()
            else:
                self.add_error(resp)


    def sendcmd(self, cmd):
        self.add_command(cmd)
        cmd = cmd.strip() + '\r\n'
        self.control_sock.sendall(cmd.encode())
        try:
            resp = self.getresp(self.control_sock)
        except socket.timeout:
            self.add_error('Getting response from server {:} timeout in 10 seconds'.format(self.host))
            return ''
        self.add_response(resp)
        return resp


    def getresp(self, socketfd):
        resp = socketfd.recv(MAX_BUF_SIZ).decode()
        return resp.strip('\x00').strip()


    def send_init_cmds(self):
        """
        Send USER, PASS, LIST commands to get the directory information of the server
        """
        username = self.user_ctl.GetValue().strip()
        password = self.pass_ctl.GetValue().strip()
        try:
            resp = self.login(username, password)
        except error_reply as e:
            self.add_error('Error: ' + e)
            return
        self.add_status('Logged in')
        resp = self.sendcmd('PWD')
        self.pwd = parse257(resp)
        self.rm_path_edit.SetValue(self.pwd)
        self.refresh_rm_list()


    def login(self, user = '', passwd = '', acct = ''):
        '''Login, default anonymous.'''
        if not user:
            user = 'anonymous'
        if not passwd:
            passwd = ''
        if not acct:
            acct = ''
        if user == 'anonymous' and passwd in {'', '-'}:
            passwd += 'anonymous@'
        resp = self.sendcmd('USER ' + user)
        if resp[0] == '3':
            resp = self.sendcmd('PASS ' + passwd)
        if resp[0] == '3':
            resp = self.sendcmd('ACCT ' + acct)
        if resp[0] != '2':
            raise error_reply(resp)
        return resp


    def setpasv(self, pasv):
        self.pasvmode = pasv


    def sendport(self, host, port):
        '''Send a PORT command with the current host and the given
        port number.
        '''
        hbytes = host.split('.')
        pbytes = [repr(port//256), repr(port%256)]
        bytes = hbytes + pbytes
        cmd = 'PORT ' + ','.join(bytes)
        return self.sendcmd(cmd)


    def get_new_port(self):
        '''Create a new socket and send a PORT command for it.'''
        err = None
        sock = None
        for res in socket.getaddrinfo(None, 0, self.control_sock.family, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
            af, socktype, proto, canonname, sa = res
            try:
                sock = socket.socket(af, socktype, proto)
                sock.bind(sa)
            except OSError as _:
                err = _
                if sock:
                    sock.close()
                sock = None
                continue
            break
        if sock is None:
            if err is not None:
                raise err
            else:
                raise OSError("getaddrinfo returns an empty list")
        sock.listen(1)
        port = sock.getsockname()[1] # Get proper port
        host = self.control_sock.getsockname()[0] # Get proper host
        resp = self.sendport(host, port)
        return sock


    def get_data_fd(self, cmd):
        """
        Return the data socket to be used in data connection
        established by commands like RETR, LIST, NLST, STOR
        """
        # PASV mode
        if self.pasvmode:
            try:
                resp = self.sendcmd('PASV')
                host, port = parse227(resp)
                datafd = socket.create_connection((host, port))
                resp = self.sendcmd(cmd)
                # 150 response mark from server
                if resp[0] != '1':
                    raise error_reply(resp)
            except:
                datafd.close()
                raise
        # PORT mode
        else:
            with self.get_new_port() as connfd:
                resp = self.sendcmd(cmd)
                if resp[0] != '1':
                    raise error_reply(resp)
                datafd, addr = connfd.accept()

        return datafd


    def get_data_lines(self, cmd, callback=print):
        """Retrieve data in line mode.  A new port is created for you.

        Args:
        callback: callback function for each line, default: print().

        Returns:
          The response code form server.
        """
        with self.get_data_fd(cmd) as datafd:
            with datafd.makefile('r', encoding='utf-8') as filefd:
                while True:
                    line = filefd.readline()
                    line = line.strip().strip('\x00')
                    # End of file
                    if not line:
                        break
                    callback(line)

        return self.getresp(self.control_sock)


    def get_data_binary(self, cmd, callback=print):
        """Get data in binary mode. A new port is created.

        Args:
        Returns:
        Same as get_data_lines.
        """
        self.sendcmd('TYPE I')
        with self.get_data_fd(cmd) as datafd:
            while True:
                # data = self.getresp(datafd)
                data = datafd.recv(MAX_BUF_SIZ)
                if not data:
                    break
                callback(data)

        return self.getresp(self.control_sock)


    def cmdline(self, line):
        """Callback of LIST command"""
        try:
            res = parselist(line)
        except error_proto:
            return
        if not res:
            return
        parse_timestamp(res)
        item = [
            res['name'],
            res['name'].rsplit('.')[-1],
            res['size'] + ' B',
            res['mode'],
            res['timestamp']
        ]
        self.rm_files.Append(item)
        j = self.rm_files.GetItemCount() - 1
        if res['mode'][0] == 'd' or res['mode'][0] == 'D':
            self.rm_files.SetItemImage(j, 0)
        else:
            self.rm_files.SetItemImage(j, 1)
        if j % 2 == 0:
            self.rm_files.SetItemBackgroundColour(j, fc.BG_COLOR)


    def cmdretr(self, data):
        """Callback of RETR command"""
        os.write(self.filefd, data)


    def cmdstor(self, filefd, cmd):
        self.sendcmd('TYPE I')

        with self.get_data_fd(cmd) as datafd:
            while True:
                # data = os.read(filefd, MAX_BUF_SIZ)
                data = filefd.read(MAX_BUF_SIZ)
                if not data:
                    break
                datafd.sendall(data)

        return self.getresp(self.control_sock)


    def refresh_rm_list(self):
        self.rm_files.DeleteAllItems()
        self.add_status('Retrieving directory listing...')
        resp = self.get_data_lines('LIST', self.cmdline)
        self.add_status('Directory listing of "{:}" successful.'.format(self.pwd))
        self.add_response(resp)
