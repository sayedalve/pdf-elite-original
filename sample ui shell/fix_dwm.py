import re

with open('MainWindow.cpp', 'r', encoding='utf-8') as f:
    code = f.read()

# 1. Fix DwmSetWindowAttribute being called before CreateWindow
bad_dwm = r'BOOL useDarkMode = TRUE;\s*DwmSetWindowAttribute\(m_hwnd, 20, &useDarkMode, sizeof\(useDarkMode\)\); // DWMWA_USE_IMMERSIVE_DARK_MODE\s*if \(!m_d2dFactory\) return E_FAIL;'
# Wait, let's see exactly what the code looks like around Initialize
