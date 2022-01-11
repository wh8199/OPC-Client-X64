#include "encoding.h"
#include <iostream>

std::string U2A(const std::wstring &str) // Unicode×Ö·û×ªAscii×Ö·û
{
    std::string strDes;
    if (str.empty())
        goto __end;
    int nLen = ::WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size(), NULL, 0, NULL, NULL);
    if (0 == nLen)
        goto __end;
    char *pBuffer = new char[nLen + 1];
    memset(pBuffer, 0, nLen + 1);
    ::WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size(), pBuffer, nLen, NULL, NULL);
    pBuffer[nLen] = '\0';
    strDes.append(pBuffer);
    delete[] pBuffer;
__end:
    return strDes;
}

std::wstring A2U(const std::string &str)
{
    std::wstring strDes;
    if (str.empty())
    {
        return strDes;
    }
    
    int nLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
    if (0 == nLen)
    {
        return strDes;
    }

    wchar_t *pBuffer = new wchar_t[nLen+1];
    memset(pBuffer, 0, nLen + 1);
    ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), pBuffer, nLen);
    pBuffer[nLen] = '\0';
    strDes.append(pBuffer);
    delete[] pBuffer;

    return strDes;
}