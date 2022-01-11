#pragma once

#include "clientDll.h"
#include "json.h"
#include "encoding.h"

char *makeResponse(std::string s)
{
    char *resp = (char *)(malloc(s.size() + 1));
    memset(resp, 0, s.size() + 1);
    strncpy(resp, s.c_str(), s.size());
    return resp;
}

char *Readitem(wchar_t *progID, wchar_t *host, wchar_t *item)
{
    COPCClient::init();
    COPCClient c;
    int ret = c.Init(progID, host);
    if (ret != S_OK)
    {
        COPCClient::stop();
        return makeResponse("server is unreachable");
    }

    ReadResult *readRet = c.ReadItem(item);
    COPCClient::stop();

    std::string s =
        ReadResult::String(ReadResult(readRet->GetType(), readRet->GetOK(), readRet->GetVal(), readRet->GetError()));
    delete (readRet);

    return makeResponse(s);
}

char* Readitems(wchar_t *progID, wchar_t *host, char *params)
{
    std::vector<std::wstring> v;
    picojson::value val;
    std::string err = picojson::parse(val, params);
    if (!err.empty())
    {
        return makeResponse("invalid param:"+err);
    }

    if (!val.is<picojson::array>())
    {
        return makeResponse("invalid param");
    }

    std::unordered_map<std::string, ReadResult> retMap;
    picojson::array paramArr = val.get<picojson::array>();

    COPCClient::init();
    COPCClient c;
    int ret = c.Init(progID, host);
    if (ret != S_OK)
    {
        COPCClient::stop();
        return makeResponse("server is unreachable");
    }

    for (int i = 0; i < paramArr.size(); i++)
    {
        std::string item = val.get(i).get<picojson::object>()["name"].get<std::string>();

        ReadResult *readRet = c.ReadItem(COPCHost::S2WS(item));
        retMap[item] = ReadResult(readRet->GetType(),readRet->GetOK(), 
            readRet->GetVal(), readRet->GetError());

        delete readRet;
    }
    COPCClient::stop();

    std::string s = ReadResult::String(retMap);
    return makeResponse(s);
}

void Freeresult(char *ret)
{
    free(ret);
}