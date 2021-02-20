$ !
$ ! DCL link procedure for NEdit
$ !
$ SET NOVERIFY
OBJS :=	 nedit, file, menu, window, selection, search, undo, shift, -
 help, preferences, tags, userCmds, regularExp, macro, text, -
 textSel, textDisp, textBuf, textDrag, server, server_common, -
 clearcase
OBJS2 := nc, server_common, clearcase
$ SET VERIFY
$ LINK 'OBJS', NEDIT_OPTIONS_FILE/OPT, [-.util]vmsUtils/lib, libUtil/lib
$ LINK 'OBJS2', NEDIT_OPTIONS_FILE/OPT, [-.util]vmsUtils/lib, libUtil/lib
