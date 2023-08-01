Name "Dove"
OutFile "buids\Dove.exe"
Icon taskface\dove32w2.ico
InstallDir $PROGRAMFILES\Dove
Section "Uninstall"
   RMDir /r /REBOOTOK $INSTDIR
   RMDir /r /REBOOTOK "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Dove"
   DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dove"
   ExecWait 'schtasks.exe /delete /f /tn Dove_autostart_0x0190553455'
   MessageBox MB_YESNO|MB_ICONQUESTION "Необходимо перезагрузить компьютер. Перезагрузить прямо сейчас?" IDNO +2
   Reboot
SectionEnd
Section
   SetOutPath $INSTDIR
   File "taskserv\bin\Release\taskserv.exe"
   File "taskface\bin\Release\taskface.exe"
   File "disk\bin\Release\disk.exe"
   File "taskface\dove32w2.ico"
   CreateDirectory "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Dove"
   CreateShortcut "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Dove\Настройка диска.lnk" "$INSTDIR\disk.exe"
   CreateShortcut "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Dove\Отправить папку на диск.lnk" "$INSTDIR\taskface.exe"
   CreateShortcut "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Dove\Uninstall.lnk" "$INSTDIR\uninstall.exe"
   WriteUninstaller $INSTDIR\uninstall.exe
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dove" "DisplayName" "Dove"
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dove" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
   WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dove" "DisplayIcon" "$\"$INSTDIR\dove32w2.ico$\""
   ExecWait 'schtasks.exe /delete /f /tn Dove_autostart_0x0190553455'
   ExecWait 'schtasks.exe /create /sc onstart /ru system /rl highest /tn Dove_autostart_0x0190553455 /tr "\$\"C:\Program Files (x86)\Dove\taskserv.exe$\""'
   MessageBox MB_YESNO|MB_ICONQUESTION "Необходимо настроить диск. Настроить прямо сейчас?" IDNO +2
   ExecShellWait "runas" "$INSTDIR\disk.exe"
SectionEnd
