#-------------------------------------------------------------------
#      D - F L A T   M A K E F I L E   -   W A T C O M   C
#-------------------------------------------------------------------

all : memopad.exe memopad.hlp
	echo all done

#-------------------------------------------------------------------
#  Delete the FULL macro for a minimal D-Flat application. You can
#  selectively remove features by deleting #define statements at
#  the beginning of DFLAT.H
#-------------------------------------------------------------------
FULL = BUILD_FULL_DFLAT
#-------------------------------------------------------------------
#  Delete the TESTING macro to eliminate the D-Flat Log and the Reload
#  Help file selection on the Help menu
#-------------------------------------------------------------------
TESTING = TESTING_DFLAT
#-------------------------------------------------------------------
MODEL = l
#------------------------------------------------
COMPILE = wcl /os /c /dWATCOM /d$(FULL) /d$(TESTING) /j /c /w4 /s /m$(MODEL)
#------------------------------------------------

.c.obj:
    $(COMPILE) $*

memopad.exe : memopad.obj dialogs.obj menus.obj dflat.lib
     wcl memopad.obj dialogs.obj menus.obj dflat.lib /k8192 /fe=memopad.exe

dflat.lib :   window.obj video.obj message.obj                         &
              mouse.obj console.obj textbox.obj listbox.obj            &
              normal.obj config.obj menu.obj menubar.obj popdown.obj   &
              rect.obj applicat.obj keys.obj sysmenu.obj editbox.obj   &
              dialbox.obj button.obj fileopen.obj msgbox.obj           &
              helpbox.obj log.obj lists.obj statbar.obj decomp.obj     &
              combobox.obj pictbox.obj calendar.obj barchart.obj       &
              clipbord.obj search.obj dfalloc.obj checkbox.obj         &
              text.obj radio.obj box.obj spinbutt.obj  watch.obj       &
              slidebox.obj direct.obj editor.obj
	del dflat.lib
	wlib dflat @dflat

huffc.exe : huffc.c htree.c
     wcl /dWATCOM /ml huffc.c htree.c

fixhelp.exe : fixhelp.c decomp.c
     wcl /dWATCOM /ml fixhelp.c decomp.c

# Note that if you're compiling in 64-bit Windows, huffc.exe and fixhelp.exe
# will fail to execute and you will have to do this last step manually in
# DOS.
memopad.hlp : memopad.txt huffc.exe fixhelp.exe
	huffc memopad.txt memopad.hlp
	fixhelp memopad
