#include <Windows.h>
#include <string>
#include <bitset>
#include <iostream>
#include <filesystem>
#include <ctime>
#include <cstdlib>

#define FLUSH_BUFFER do { char ch; while ((ch = getchar()) != '\n' && ch != EOF); } while (0);
#define MAXKEYS 64
HWND targetHwnd;

void sendChar(HWND hWnd, char ch);
void captureScreen(HWND hWnd, std::wstring destFile);
HWND findWindow(const char* name);
bool searchWstring(std::wstring src, std::wstring target);

int main() {
    srand(time(NULL));
    HWND child = findWindow("Multisim");

    if (targetHwnd == NULL) {
        std::cerr << "Target HWND is NULL | Exit" << std::endl;
        return -1;
    }

    if (child == NULL) {
        std::cerr << "Child HWND is NULL | Exit" << std::endl;
        return -1;
    }
  
    int inputs;
    do {
        std::cout << "Number of inputs [max:" << MAXKEYS << "]: " << std::endl;
        std::cin >> inputs;
        FLUSH_BUFFER;
    } while (inputs > MAXKEYS || inputs < 1);

    std::bitset<MAXKEYS> prevKeyStates(0);
    char* keys = new char[inputs];
    for (int i = 0; i < inputs; ++i) {
        do {
            std::cout << "Enter character for input (A-Z|0-9) [" << i + 1 << "]: ";

            char temp;
            std::cin >> temp;
            FLUSH_BUFFER;
            
            if (!isalnum(temp)) {
                std::cout << "Enter a character or a single digit number." << std::endl;
                continue;
            }

            if (isalpha(temp)) {
                temp = toupper(temp);
            }

            if (std::find(keys, keys + inputs, temp) != keys + inputs) {
                std::cout << "Key " << temp << " is already registered." << std::endl;
                continue;
            } else {
                keys[i] = temp;
                break;
            }
        } while (true);
    }

    wchar_t* _currentDirectory = new wchar_t[100]{};
    GetCurrentDirectory(100, _currentDirectory);
    std::wstring currentDirectory(_currentDirectory);
    delete[] _currentDirectory;
    
    std::wstring randomNumber = std::to_wstring(rand() % 10000);
    currentDirectory += L"\\Simulations-" + randomNumber + L"\\";
    std::wcout << "Find screenshots at: " << currentDirectory << std::endl;
    std::filesystem::create_directories(std::string(currentDirectory.begin(), currentDirectory.end()));

    std::cout << "***" << std::endl;
    std::cout << "1. Initialise all values to zero\n2. Adjust the window size as required.\n3. Do NOT touch multisim while simulation during simulation." << std::endl;
    std::cout << "***" << std::endl;

    std::cout << "Registered keys: { ";
    for (int i = 0; i < inputs; ++i) {
        std::cout << keys[i] << " ";
    }
    std::cout << "}" << std::endl;

    std::cout << "Press ENTER to start simulation ...";
    getchar();

    for (int times = 0; times < pow(2, inputs); times++) {
        std::bitset<MAXKEYS> currentStates(times);
        for (int i = 0; i < inputs; ++i) {
            if (currentStates[i] != prevKeyStates[i]) {
                sendChar(child, keys[i]);
                Sleep(200);

                prevKeyStates[i] = currentStates[i];
            }
        }

        captureScreen(child, currentDirectory + std::to_wstring(times+1) + L".bmp");
    }

    delete[] keys;
    return 0;
}

void sendChar(HWND hWnd, char ch) {
	SendMessage(hWnd, WM_CHAR, ch, NULL);
}

void captureScreen(HWND hWnd, std::wstring destFile) {
    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);

    HDC hdcScreen;
    HDC hdcWindow;
    HDC hdcMemDC = NULL;
    HBITMAP hbmScreen = NULL;
    BITMAP bmpScreen;
    DWORD dwBytesWritten = 0;
    DWORD dwSizeofDIB = 0;
    HANDLE hFile = NULL;
    char* lpbitmap = NULL;
    HANDLE hDIB = NULL;
    DWORD dwBmpSize = 0;

    hdcScreen = GetDC(hWnd);
    hdcWindow = GetDC(hWnd);

    hdcMemDC = CreateCompatibleDC(hdcWindow);
    if (!hdcMemDC) {
        std::cerr << "No compatible DC was created.\n";
        goto done;
    }

    RECT rcClient;
    GetClientRect(hWnd, &rcClient);
    SetStretchBltMode(hdcWindow, HALFTONE);
    if (!StretchBlt(hdcWindow,
        0, 0,
        rcClient.right, rcClient.bottom,
        hdcScreen,
        0, 0,
        rcClient.right,
        rcClient.bottom,
        SRCCOPY | CAPTUREBLT)) {
        std::cerr << "StretchBlt has failed." << std::endl;
        goto done;
    }

    hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
    if (!hbmScreen) {
        std::cerr << "CreateCompatibleBitmap Failed" << std::endl;
        goto done;
    }

    SelectObject(hdcMemDC, hbmScreen);
    if (!BitBlt(hdcMemDC,
        0, 0,
        rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
        hdcWindow,
        0, 0,
        SRCCOPY | CAPTUREBLT)) {
        std::cerr << "BitBlt has failed" << std::endl;
        goto done;
    }

    GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

    BITMAPFILEHEADER   bmfHeader;
    BITMAPINFOHEADER   bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
    hDIB = GlobalAlloc(GHND, dwBmpSize);
    lpbitmap = (char*)GlobalLock(hDIB);
    GetDIBits(hdcWindow, hbmScreen, 0,
        (UINT)bmpScreen.bmHeight,
        lpbitmap,
        (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    hFile = CreateFile(destFile.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwSizeofDIB;
    bmfHeader.bfType = 0x4D42;

    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    GlobalUnlock(hDIB);
    GlobalFree(hDIB);
    CloseHandle(hFile);
done:
    DeleteObject(hbmScreen);
    DeleteObject(hdcMemDC);
    ReleaseDC(NULL, hdcScreen);
    ReleaseDC(hWnd, hdcWindow);
}

BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lparam) {
	wchar_t title[50];
	GetWindowText(hwnd, title, 50);

	if (searchWstring(title, L"- Multisim -")) {
		targetHwnd = hwnd;
		return false;
	}

	return true;
}

bool searchWstring(std::wstring src, std::wstring target) {
    int index = src.find(target);
    return index >= 0 && index < src.length();
}

HWND findWindow(const char* name) {
	EnumWindows(enumWindowsProc, NULL);

    std::cout << "Window detection -- DEBUG INFO -- BEGIN" << std::endl;
	std::cout << "Target HWND: " << targetHwnd << std::endl;

	HWND child = GetWindow(targetHwnd, GW_CHILD);
	child = GetWindow(child, GW_CHILD);
	child = GetWindow(child, GW_CHILD);
	
    std::cout << "Child HWND: " << child << std::endl;
    std::cout << "Window detection -- DEBUG INFO -- END" << std::endl;
	return child;
}

