#!/usr/bin/env python
# -*- coding: utf-8 -*-

#This file is part of Beremiz, a Integrated Development Environment for
#programming IEC 61131-3 automates supporting plcopen standard and CanFestival. 
#
#Copyright (C) 2007: Edouard TISSERANT and Laurent BESSARD
#
#See COPYING file for copyrights details.
#
#This library is free software; you can redistribute it and/or
#modify it under the terms of the GNU General Public
#License as published by the Free Software Foundation; either
#version 2.1 of the License, or (at your option) any later version.
#
#This library is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#General Public License for more details.
#
#You should have received a copy of the GNU General Public
#License along with this library; if not, write to the Free Software
#Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import os, sys, getopt

def usage():
    print """
Usage of Beremiz PLC execution service :\n
%s {[-n name] [-i ip] [-p port] [-x enabletaskbar]|-h|--help} working_dir
           -n        - zeroconf service name
           -i        - ip of interface to bind to (x.x.x.x)
           -p        - port number
           -h        - print this help text and quit
           -x        - enable/disable wxTaskbarIcon (0:disable 1:enable)
           
           working_dir - directory where are stored PLC files
"""%sys.argv[0]

try:
    opts, args = getopt.getopt(sys.argv[1:], "i:p:n:x:h")
except getopt.GetoptError, err:
    # print help information and exit:
    print str(err) # will print something like "option -a not recognized"
    usage()
    sys.exit(2)

# default values
ip = ""
port = 3000
name = os.environ[{
     "linux2":"USER",
     "win32":"USERNAME",
     }.get(sys.platform, "USER")]
enablewx = True
havewx = False

for o, a in opts:
    if o == "-h":
        usage()
        sys.exit()
    elif o == "-i":
        if len(a.split(".")) == 4 or a == "localhost":
            ip = a
    elif o == "-p":
        # port: port that the service runs on
        port = int(a)
    elif o == "-n":
        name = a
    elif o == "-x":
        enablewx = int(a)
    else:
        usage()
        sys.exit()

if len(args) > 1:
    usage()
    sys.exit()
elif len(args) == 1:
    WorkingDir = args[0]
elif len(args) == 0:
    WorkingDir = os.getcwd()
    args=[WorkingDir]

if enablewx:
    try:
        import wx, re
        from wx.lib.embeddedimage import PyEmbeddedImage
        from threading import Thread
        from types import *
        havewx = True
    except:
        havewx = False

    if havewx:
        defaulticon = PyEmbeddedImage(
        "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")
        
        #----------------------------------------------------------------------
        starticon = PyEmbeddedImage(
        "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAABHNCSVQICAgIfAhkiAAABbpJ"
        "REFUSIl9lltsFNcdxn9nZnbHs15fd23j9TXYC0UCKzEhMQ+oIS2g1kQ1pbFStX0opFWsovSh"
        "rUqp2pS2ioTUolaKFOGHqGkiJcKRuDhOaZRiZCsCXyBgCBBfMfbu+oa9s17wzuzl9MH24mDD"
        "XzoPc/6fft+c72jOGSGlZEVlAU8D9cB20zQ9HR0duRcvXszq7e01EomEUlFREa+srLR8Pl+g"
        "sbHx3zk5ORcAFfACA8Bt4CFUSomUUkgpS6SUB6SUH5umOXLgwIEHqqrKJfGao7S0VB49ejRo"
        "2/YnUsrT+/fvb66pqSldYiKlRCytoBB4Gfjx6dOnq5qamjwTExOKqqqU+QrYUJFN7QY32Qbc"
        "vSeYCGtcux1i5M5dAPx+P1VVVQvnzp0ziouLfx8MBt9cXoAGZABbgZ1HjhwpO378eEEymaSi"
        "tIBjPy9lU5nKoyWExF2yjy+mN3HsH+/Q3d3NwMCAsZTI9pVaDXgK2Hr27Nn85ubmEpdh8IMX"
        "ffxirwshVrGXHBQSC/dIRvoZGuz/WkvTtHIhhCGlXABQgI2Tk5P5hw8f3uZwOGj8VjGHXnoC"
        "HJCpJFbkLtr8FXbX+XC79HRPVVW/qqre9LtIKX/S0NDwy76+vq1lhTr/fM2NAmTk+fHv/dea"
        "BlZkDHP0PHODH2NHg1gykw8/X7Dfb7vjTNgJqqurT3R1db0GoF2/fl0fGhqqdWca/K7RhZLO"
        "WSBU55oGGXlVZORVkeV7nsFPDqKL+9TWJCI3n9rojX2mYhjGj4QQv5FSziunTp0qdjqd4hvl"
        "Lnz5j49lrPMNhv7zM6b63knPuQpryMj3A9A2L++nvDaZXheqqrrXrVu3D0C5detWudPpxO/T"
        "Hk8HYnOD3J+8yr3bH6XnZNImHg3xfsgenfHo5QAyJwFAdnb2HiGEppmmWa3rOhtKrCcalNT9"
        "llTSwvBsXISn4nRdbJ5/czRsWvlGhQAEYtFg0kl2dnYZUKgB5U6nk5L82BMNXIU1X3uOWFH5"
        "eWIuy/YYWcjU4qQAxQ22bWMYhgfIU1RV/UrXdWaiDyOyUiLROktoJfDtC8fZfWQbb//v75ix"
        "MDlGnvjVC3+gflNDWiMQKPMalmVh2/a8w+HQFKAHIBR2ABCOS+uN6cTMoFstXmlwZbSba7tv"
        "8hfzT7z+7k+ZnZ0BoK5yR1qjCBV7MoVt29i2PaWqqq0BvUIIQqYORHlrKj6R9BoVj0b04oY9"
        "nEt+yvz3Y5yR/+Xap3XsDb/EtvV1aY1DdTA7HsW2bCKRyLiUclYBelRVldNWAfPSm4oV5ZQJ"
        "Vn/G9Zv2oWt6Ous7e4K81XiC1wNNBO6OIWKgB7Mwp000TYuFw+GxWCw2qbS2tk7k5uae/eDD"
        "Fn594p6SFyxRCjKLUBWF8fBoegTNMVLLm/kwdMyGGON/nePLklv0dl/Cii3gdrtvAzdg8aig"
        "vb296uDBgwMjIyMCwFvoZXv9NvRnIKqHSckUyQdJrtfexPqm5LGVAuNdVaofcCVywfpexLYD"
        "CsDOnTvnioqKzGXdzNQMV9tvkJEyUITyeOAjpYyAc9gxYc/GWyK2HYDF4xog6fV6h1i8FwCo"
        "LK/EncwhkWGxEH9AXLMXM2H1CpQBifI3yeapZ+70d43+cSo4+95yL23g8XiGFUWp3bVrV/Ty"
        "5ctZnR2ddHZ08uxzz1K9eT1GRhJls1gFlsfieK+WpJ5e/3z7pcuXzmia1rJSs3xlOg8dOvTD"
        "8fHx7wQCgb4tW7bMm6b55/Pnz+eGw+FFGJDT5iT1XRWlfxHMZ06+/Vz9dCAQeG9kZKR1x44d"
        "nSdPnkyuZSAArbq6eqOiKAP9/f3xlpaWgra2tlei0eiryWSyKGKa2TcaL+muwcxU5aDf9Gi+"
        "L0Oh0BehUOiaZVlnAoHAzFr7Ih75bVnVb2pqcvf09Phi0ei6+/rUC6lw1k0p5bSUctThcIwP"
        "Dw/HnwT4P6CDl+TMvD0JAAAAAElFTkSuQmCC")
        
        #----------------------------------------------------------------------
        stopicon = PyEmbeddedImage(
        "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")
        
    class ParamsEntryDialog(wx.TextEntryDialog):
        if wx.VERSION < (2, 6, 0):
            def Bind(self, event, function, id = None):
                if id is not None:
                    event(self, id, function)
                else:
                    event(self, function)
        
    
        def __init__(self, parent, message, caption = "Please enter text", defaultValue = "", 
                           style = wx.OK|wx.CANCEL|wx.CENTRE, pos = wx.DefaultPosition):
            wx.TextEntryDialog.__init__(self, parent, message, caption, defaultValue, style, pos)
            
            self.Tests = []
            if wx.VERSION >= (2, 8, 0):
                self.Bind(wx.EVT_BUTTON, self.OnOK, id=self.GetAffirmativeId())
            elif wx.VERSION >= (2, 6, 0):
                self.Bind(wx.EVT_BUTTON, self.OnOK, id=self.GetSizer().GetItem(3).GetSizer().GetAffirmativeButton().GetId())
            else:
                self.Bind(wx.EVT_BUTTON, self.OnOK, id=self.GetSizer().GetItem(3).GetSizer().GetChildren()[0].GetSizer().GetChildren()[0].GetWindow().GetId())
        
        def OnOK(self, event):
            value = self.GetValue()
            texts = {"value" : value}
            for function, message in self.Tests:
                if not function(value):
                    message = wx.MessageDialog(self, message%texts, "Error", wx.OK|wx.ICON_ERROR)
                    message.ShowModal()
                    message.Destroy()
                    return
            self.EndModal(wx.ID_OK)
            event.Skip()
        
        def GetValue(self):
            return self.GetSizer().GetItem(1).GetWindow().GetValue()
        
        def SetTests(self, tests):
            self.Tests = tests
            
    class BeremizTaskBarIcon(wx.TaskBarIcon):
        TBMENU_CHANGE_NAME = wx.NewId()
        TBMENU_CHANGE_PORT = wx.NewId()
        TBMENU_CHANGE_INTERFACE = wx.NewId()
        TBMENU_LIVE_SHELL = wx.NewId()
        TBMENU_CHANGE_WD = wx.NewId()
        TBMENU_QUIT = wx.NewId()
        
        def __init__(self, pyroserver):
            wx.TaskBarIcon.__init__(self)
            self.pyroserver = pyroserver
            # Set the image
            self.UpdateIcon(None)
    
            # bind some events
            self.Bind(wx.EVT_MENU, self.OnTaskBarChangeName, id=self.TBMENU_CHANGE_NAME)
            self.Bind(wx.EVT_MENU, self.OnTaskBarChangeInterface, id=self.TBMENU_CHANGE_INTERFACE)
            self.Bind(wx.EVT_MENU, self.OnTaskBarLiveShell, id=self.TBMENU_LIVE_SHELL)
            self.Bind(wx.EVT_MENU, self.OnTaskBarChangePort, id=self.TBMENU_CHANGE_PORT)
            self.Bind(wx.EVT_MENU, self.OnTaskBarChangeWorkingDir, id=self.TBMENU_CHANGE_WD)
            self.Bind(wx.EVT_MENU, self.OnTaskBarQuit, id=self.TBMENU_QUIT)
        
        def CreatePopupMenu(self):
            """
            This method is called by the base class when it needs to popup
            the menu for the default EVT_RIGHT_DOWN event.  Just create
            the menu how you want it and return it from this function,
            the base class takes care of the rest.
            """
            menu = wx.Menu()
            menu.Append(self.TBMENU_CHANGE_NAME, "Change Name")
            menu.Append(self.TBMENU_CHANGE_INTERFACE, "Change IP of interface to bind")
            menu.Append(self.TBMENU_LIVE_SHELL, "Launch a live Python shell")
            menu.Append(self.TBMENU_CHANGE_PORT, "Change Port Number")
            menu.AppendSeparator()
            menu.Append(self.TBMENU_CHANGE_WD, "Change working directory")
            menu.Append(self.TBMENU_QUIT, "Quit")
            return menu
    
        def MakeIcon(self, img):
            """
            The various platforms have different requirements for the
            icon size...
            """
            if "wxMSW" in wx.PlatformInfo:
                img = img.Scale(16, 16)
            elif "wxGTK" in wx.PlatformInfo:
                img = img.Scale(22, 22)
            # wxMac can be any size upto 128x128, so leave the source img alone....
            icon = wx.IconFromBitmap(img.ConvertToBitmap() )
            return icon
        
        def OnTaskBarChangeInterface(self,evt):
            dlg = ParamsEntryDialog(None, "Enter the ip of the interface to bind", defaultValue=pyroserver.ip)
            dlg.SetTests([(re.compile('\d{1,3}(?:\.\d{1,3}){3}$').match, "Ip is not valid!"),
                           ( lambda ip :len([x for x in ip.split(".") if 0 <= int(x) <= 255]) == 4, "Ip is not valid!")
                           ])
            if dlg.ShowModal() == wx.ID_OK:
                self.pyroserver.ip = dlg.GetValue()
                self.pyroserver.Stop()
            evt.Skip()
                
        def OnTaskBarChangePort(self,evt):
            dlg = ParamsEntryDialog(None, "Enter a port number ", defaultValue=str(pyroserver.port))
            dlg.SetTests([(UnicodeType.isdigit, "Port number must be an integer!"), (lambda port : 0 <= int(port) <= 65535 , "Port number must be 0 <= port <= 65535!")])
            if dlg.ShowModal() == wx.ID_OK:
                self.pyroserver.port = int(dlg.GetValue())
                self.pyroserver.Stop()
            evt.Skip()
        
        def OnTaskBarChangeWorkingDir(self,evt):
            dlg = wx.DirDialog(None, "Choose a working directory ", pyroserver.workdir, wx.DD_NEW_DIR_BUTTON)
            if dlg.ShowModal() == wx.ID_OK:
                self.pyroserver.workdir = dlg.GetPath()
                self.pyroserver.Stop()
            evt.Skip()
                
        def OnTaskBarChangeName(self,evt):
            dlg = ParamsEntryDialog(None, "Enter a name ", defaultValue=pyroserver.name)
            dlg.SetTests([(lambda name : len(name) is not 0 , "Name must not be null!")])
            if dlg.ShowModal() == wx.ID_OK:
                self.pyroserver.name = dlg.GetValue()
                self.pyroserver.Restart()
            evt.Skip()

        def OnTaskBarLiveShell(self,evt):
            if self.pyroserver.plcobj is not None and self.pyroserver.plcobj.python_threads_vars is not None:
                from wx import py
                frame = py.shell.ShellFrame(locals=self.pyroserver.plcobj.python_threads_vars)
                frame.Show()
            else:
                wx.MessageBox("No runnning PLC","Error")
            evt.Skip()

        def OnTaskBarQuit(self,evt):
            pyroserver.Quit()
            self.RemoveIcon()
            wx.GetApp().ExitMainLoop()
            evt.Skip()
            
        def UpdateIcon(self, plcstatus):
            if plcstatus is "Started" :
                currenticon = self.MakeIcon(starticon.GetImage())
            elif plcstatus is "Stopped":
                currenticon = self.MakeIcon(stopicon.GetImage())
            else:
                currenticon = self.MakeIcon(defaulticon.GetImage())
            self.SetIcon(currenticon, "Beremiz Service")

from runtime import PLCObject, ServicePublisher
import Pyro.core as pyro

if not os.path.isdir(WorkingDir):
    os.mkdir(WorkingDir)

class Server():
    def __init__(self, name, ip, port, workdir, args):
        self.continueloop = True
        self.daemon = None
        self.name = name
        self.ip = ip
        self.port = port
        self.workdir = workdir
        self.args = args
        self.plcobj = None
        self.servicepublisher = None
        self.statuschange = None
    
    def Loop(self, statuschange=None):
        self.statuschange = statuschange
        while self.continueloop:
            self.Start()
        
    def Restart(self):
        self.Stop()

    def Quit(self):
        self.continueloop = False
        self.Stop()

    def Start(self):
        pyro.initServer()
        self.daemon=pyro.Daemon(host=self.ip, port=self.port)
        self.plcobj = PLCObject(self.workdir, self.daemon, self.args, self.statuschange)
        uri = self.daemon.connect(self.plcobj,"PLCObject")
    
        print "The daemon runs on port :",self.port
        print "The object's uri is :",uri
        print "The working directory :",self.workdir
        
        # Configure and publish service
        # Not publish service if localhost in address params
        if self.ip != "localhost" and self.ip != "127.0.0.1":    
            print "Publish service on local network"
            self.servicepublisher = ServicePublisher.ServicePublisher()            
            self.servicepublisher.RegisterService(self.name, self.ip, self.port)
        
        sys.stdout.flush()
        
        self.daemon.requestLoop()
    
    def Stop(self):
        if self.servicepublisher is not None:
            self.servicepublisher.UnRegisterService()
            del self.servicepublisher
        self.daemon.shutdown(True)
        
pyroserver = Server(name, ip, port, WorkingDir, args)

if havewx:
    app=wx.App(redirect=False)
    taskbar_instance = BeremizTaskBarIcon(pyroserver)
    def statuschange(status):
        wx.CallAfter(taskbar_instance.UpdateIcon,status)
    pyro_thread=Thread(target=pyroserver.Loop, args=[statuschange])
    pyro_thread.start()
    app.MainLoop()
else:
    pyroserver.Loop()
