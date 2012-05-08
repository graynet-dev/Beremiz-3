import wx
import os, sys
from xml.dom import minidom

from ConfigTreeNode import opjimg
from py_ext import PythonCodeTemplate

class RootClass(PythonCodeTemplate):

    ConfNodeMethods = [
        {"bitmap" : opjimg("editWXGLADE"),
         "name" : _("WXGLADE GUI"),
         "tooltip" : _("Edit a WxWidgets GUI with WXGlade"),
         "method" : "_editWXGLADE"},
    ]

    def _getWXGLADEpath(self):
        # define name for IEC raw code file
        return os.path.join(self.CTNPath(), "hmi.wxg")

    def launch_wxglade(self, options, wait=False):
        from wxglade import __file__ as fileName
        path = os.path.dirname(fileName)
        glade = os.path.join(path, 'wxglade.py')
        if wx.Platform == '__WXMSW__':
            glade = "\"%s\""%glade
        mode = {False:os.P_NOWAIT, True:os.P_WAIT}[wait]
        os.spawnv(mode, sys.executable, ["\"%s\""%sys.executable] + [glade] + options)


    def CTNGenerate_C(self, buildpath, locations):
        """
        Return C code generated by iec2c compiler 
        when _generate_softPLC have been called
        @param locations: ignored
        @return: [(C_file_name, CFLAGS),...] , LDFLAGS_TO_APPEND
        """
        
        current_location = self.GetCurrentLocation()
        # define a unique name for the generated C file
        location_str = "_".join(map(lambda x:str(x), current_location))
        
        runtimefile_path = os.path.join(buildpath, "runtime_%s.py"%location_str)
        runtimefile = open(runtimefile_path, 'w')
        
        hmi_frames = {}
        
        wxgfile_path=self._getWXGLADEpath()
        if os.path.exists(wxgfile_path):
            wxgfile = open(wxgfile_path, 'r')
            wxgtree = minidom.parse(wxgfile)
            wxgfile.close()
            
            for node in wxgtree.childNodes[1].childNodes:
                if node.nodeType == wxgtree.ELEMENT_NODE:
                    hmi_frames[node._attrs["name"].value] =  node._attrs["class"].value
                    
            hmipyfile_path=os.path.join(self._getBuildPath(), "hmi.py")
            if wx.Platform == '__WXMSW__':
                wxgfile_path = "\"%s\""%wxgfile_path
                wxghmipyfile_path = "\"%s\""%hmipyfile_path
            else:
                wxghmipyfile_path = hmipyfile_path
            self.launch_wxglade(['-o', wxghmipyfile_path, '-g', 'python', wxgfile_path], wait=True)
            
            hmipyfile = open(hmipyfile_path, 'r')
            runtimefile.write(hmipyfile.read())
            hmipyfile.close()
        
        runtimefile.write(self.GetPythonCode())
        runtimefile.write("""
%(declare)s

def _runtime_%(location)s_begin():
    global %(global)s
    
    def OnCloseFrame(evt):
        wx.MessageBox(_("Please stop PLC to close"))
    
    %(init)s
    
def _runtime_%(location)s_cleanup():
    global %(global)s
    
    %(cleanup)s

""" % {"location": location_str,
       "declare": "\n".join(map(lambda x:"%s = None" % x, hmi_frames.keys())),
       "global": ",".join(hmi_frames.keys()),
       "init": "\n".join(map(lambda x: """
    %(name)s = %(class)s(None)
    %(name)s.Bind(wx.EVT_CLOSE, OnCloseFrame)
    %(name)s.Show()
""" % {"name": x[0], "class": x[1]},
                             hmi_frames.items())),
       "cleanup": "\n    ".join(map(lambda x:"%s.Destroy()" % x, hmi_frames.keys()))})
        runtimefile.close()
        
        return [], "", False, ("runtime_%s.py"%location_str, file(runtimefile_path,"rb"))

    def _editWXGLADE(self):
        wxg_filename = self._getWXGLADEpath()
        open_wxglade = True
        if not self.GetCTRoot().CheckProjectPathPerm():
            dialog = wx.MessageDialog(self.GetCTRoot().AppFrame,
                                      _("You don't have write permissions.\nOpen wxGlade anyway ?"),
                                      _("Open wxGlade"),
                                      wx.YES_NO|wx.ICON_QUESTION)
            open_wxglade = dialog.ShowModal() == wx.ID_YES
            dialog.Destroy()
        if open_wxglade:
            if not os.path.exists(wxg_filename):
                hmi_name = self.BaseParams.getName()
                open(wxg_filename,"w").write("""<?xml version="1.0"?>
    <application path="" name="" class="" option="0" language="python" top_window="%(name)s" encoding="UTF-8" use_gettext="0" overwrite="0" use_new_namespace="1" for_version="2.8" is_template="0">
        <object class="%(class)s" name="%(name)s" base="EditFrame">
            <style>wxDEFAULT_FRAME_STYLE</style>
            <title>frame_1</title>
        </object>
    </application>
    """ % {"name": hmi_name, "class": "Class_%s" % hmi_name})
            if wx.Platform == '__WXMSW__':
                wxg_filename = "\"%s\""%wxg_filename
            self.launch_wxglade([wxg_filename])
