#!/usr/bin/env python
# -*- coding: utf-8 -*-

# This file is part of Beremiz, a Integrated Development Environment for
# programming IEC 61131-3 automates supporting plcopen standard and CanFestival.
#
# Copyright (C) 2007: Edouard TISSERANT and Laurent BESSARD
#
# See COPYING file for copyrights details.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

from __future__ import absolute_import
import os

from c_ext.CFileEditor import CFileEditor
from CodeFileTreeNode import CodeFile


class CFile(CodeFile):
    XSD = """<?xml version="1.0" encoding="ISO-8859-1" ?>
    <xsd:schema xmlns:xsd="http://www.w3.org/2001/XMLSchema">
      <xsd:element name="CExtension">
        <xsd:complexType>
          <xsd:attribute name="CFLAGS" type="xsd:string" use="required"/>
          <xsd:attribute name="LDFLAGS" type="xsd:string" use="required"/>
        </xsd:complexType>
      </xsd:element>
    </xsd:schema>
    """
    CODEFILE_NAME = "CFile"
    SECTIONS_NAMES = [
        "includes",
        "globals",
        "initFunction",
        "cleanUpFunction",
        "retrieveFunction",
        "publishFunction"]
    EditorType = CFileEditor

    def GetIconName(self):
        return "Cfile"

    def CodeFileName(self):
        return os.path.join(self.CTNPath(), "cfile.xml")

    def CTNGenerate_C(self, buildpath, locations):
        """
        Generate C code
        @param current_location: Tupple containing confnode IEC location : %I0.0.4.5 => (0,0,4,5)
        @param locations: List of complete variables locations \
            [{"IEC_TYPE" : the IEC type (i.e. "INT", "STRING", ...)
            "NAME" : name of the variable (generally "__IW0_1_2" style)
            "DIR" : direction "Q","I" or "M"
            "SIZE" : size "X", "B", "W", "D", "L"
            "LOC" : tuple of interger for IEC location (0,1,2,...)
            }, ...]
        @return: [(C_file_name, CFLAGS),...] , LDFLAGS_TO_APPEND
        """
        current_location = self.GetCurrentLocation()
        # define a unique name for the generated C file
        location_str = "_".join(map(str, current_location))

        text = "/* Code generated by Beremiz c_ext confnode */\n\n"
        text += "#include <stdio.h>\n\n"

        # Adding includes
        text += "/* User includes */\n"
        text += self.CodeFile.includes.getanyText().strip()
        text += "\n"

        text += '#include "iec_types_all.h"\n\n'

        # Adding variables
        config = self.GetCTRoot().GetProjectConfigNames()[0]
        text += "/* User variables reference */\n"
        for variable in self.CodeFile.variables.variable:
            var_infos = {
                "name": variable.getname(),
                "global": "%s__%s" % (config.upper(),
                                      variable.getname().upper()),
                "type": "__IEC_%s_t" % variable.gettype()}
            text += "extern %(type)s %(global)s;\n" % var_infos
            text += "#define %(name)s %(global)s.value\n" % var_infos
        text += "\n"

        # Adding user global variables and routines
        text += "/* User internal user variables and routines */\n"
        text += self.CodeFile.globals.getanyText().strip()
        text += "\n"

        # Adding Beremiz confnode functions
        text += "/* Beremiz confnode functions */\n"
        text += "int __init_%s(int argc,char **argv)\n{\n" % location_str
        text += self.CodeFile.initFunction.getanyText().strip()
        text += "  return 0;\n}\n\n"

        text += "void __cleanup_%s(void)\n{\n" % location_str
        text += self.CodeFile.cleanUpFunction.getanyText().strip()
        text += "\n}\n\n"

        text += "void __retrieve_%s(void)\n{\n" % location_str
        text += self.CodeFile.retrieveFunction.getanyText().strip()
        text += "\n}\n\n"

        text += "void __publish_%s(void)\n{\n" % location_str
        text += self.CodeFile.publishFunction.getanyText().strip()
        text += "\n}\n\n"

        Gen_Cfile_path = os.path.join(buildpath, "CFile_%s.c" % location_str)
        cfile = open(Gen_Cfile_path, 'w')
        cfile.write(text)
        cfile.close()

        matiec_CFLAGS = '"-I%s"' % os.path.abspath(self.GetCTRoot().GetIECLibPath())

        return [(Gen_Cfile_path, str(self.CExtension.getCFLAGS() + matiec_CFLAGS))], str(self.CExtension.getLDFLAGS()), True
