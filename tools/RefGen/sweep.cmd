@echo off
del C:\RefGenWin\results\sweep.txt 2>nul
C:\Python312\python.exe -I C:\RefGenWin\tool\lib\sweep_win.py UE_4.3+UE_4.4+UE_4.5+UE_4.6+UE_4.7+UE_4.8+UE_4.9+UE_4.10+UE_4.11+UE_4.12+UE_4.13+UE_4.14+UE_4.15+UE_4.16+UE_4.17+UE_4.18+UE_4.19+UE_4.20+UE_4.21+UE_4.22+UE_4.23+UE_4.24+UE_4.25+UE_4.26+UE_4.27+UE_5.0+UE_5.1+UE_5.2+UE_5.3+UE_5.4+UE_5.5+UE_5.6+UE_5.7+UE_5.9 > C:\RefGenWin\sweep.log 2>&1
