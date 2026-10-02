#include "Logger.h"
#include <windows.h>
#include <strsafe.h>
#include <stdio.h>

namespace ShaiyaOverlay
{
    HANDLE Logger::ConsoleHandle = nullptr;
    HANDLE Logger::FileHandle = INVALID_HANDLE_VALUE;
    bool Logger::Initialized = false;

    bool Logger::Initialize()
    {
        if (Initialized)
            return true;

        if (AllocConsole())
        {
            SetConsoleTitleA("Shaiya Hardcore Overlay - Debug Console");
            ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

            FILE* Stream = nullptr;
            freopen_s(&Stream, "CONOUT$", "w", stdout);
            freopen_s(&Stream, "CONOUT$", "w", stderr);
        }

        FileHandle = CreateFileA(
            "ShaiyaOverlay.log",
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        Initialized = true;
        Info("Logger initialized successfully.");
        return true;
    }

    void Logger::Uninitialize()
    {
        if (!Initialized)
            return;

        Info("Logger shutting down.");

        if (FileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(FileHandle);
            FileHandle = INVALID_HANDLE_VALUE;
        }

        if (ConsoleHandle)
        {
            FreeConsole();
            ConsoleHandle = nullptr;
        }

        Initialized = false;
    }

    void Logger::WriteOutput(const char* Level, const char* Message)
    {
        SYSTEMTIME Time;
        GetLocalTime(&Time);

        char Formatted[1024] = { 0 };
        StringCchPrintfA(
            Formatted,
            sizeof(Formatted),
            "[%02d:%02d:%02d] [%s] %s\r\n",
            Time.wHour, Time.wMinute, Time.wSecond,
            Level, Message
        );

        DWORD Len = (DWORD)lstrlenA(Formatted);

        if (ConsoleHandle)
        {
            DWORD Written = 0;
            WriteConsoleA(ConsoleHandle, Formatted, Len, &Written, nullptr);
        }

        if (FileHandle != INVALID_HANDLE_VALUE)
        {
            DWORD Written = 0;
            WriteFile(FileHandle, Formatted, Len, &Written, nullptr);
            FlushFileBuffers(FileHandle);
        }
    }

    void Logger::Log(const char* Format, ...)
    {
        char Buffer[896] = { 0 };
        va_list Args;
        va_start(Args, Format);
        StringCchVPrintfA(Buffer, sizeof(Buffer), Format, Args);
        va_end(Args);

        WriteOutput("LOG", Buffer);
    }

    void Logger::Info(const char* Format, ...)
    {
        char Buffer[896] = { 0 };
        va_list Args;
        va_start(Args, Format);
        StringCchVPrintfA(Buffer, sizeof(Buffer), Format, Args);
        va_end(Args);

        WriteOutput("INFO", Buffer);
    }

    void Logger::Error(const char* Format, ...)
    {
        char Buffer[896] = { 0 };
        va_list Args;
        va_start(Args, Format);
        StringCchVPrintfA(Buffer, sizeof(Buffer), Format, Args);
        va_end(Args);

        WriteOutput("ERROR", Buffer);
    }
}
