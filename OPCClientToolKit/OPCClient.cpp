/*
OPCClientToolKit
Copyright (C) 2005 Mark C. Beharrell

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA  02111-1307, USA.
*/

#include <process.h>
#include <iostream>

#include "OPCClient.h"
#include "OPCHost.h"
#include "OPCServer.h"
#include "OPCItem.h"
#include "error.h"
#include "json.h"

#ifdef OPCDA_CLIENT_NAMESPACE
namespace opcda_client
{
#endif

const GUID COPCClient::CATID_OPCDAv10 = IID_CATID_OPCDAServer10; // {63D5F430-CFE4-11d1-B2C8-0060083BA1FB}

const GUID COPCClient::CATID_OPCDAv20 = IID_CATID_OPCDAServer20; // {63D5F432-CFE4-11d1-B2C8-0060083BA1FB}

ATL::CComPtr<IMalloc> COPCClient::iMalloc;

int COPCClient::ReleaseCount = 0;

bool COPCClient::init(OPCOLEInitMode mode)
{
    HRESULT result = -1;
    if (mode == APARTMENTTHREADED)
    {
        result = CoInitialize(nullptr);
    }
    if (mode == MULTITHREADED)
    {
        result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    }

    if (FAILED(result))
    {
        throw OPCException(L"COPCClient::init: CoInitialize failed");
    }

    CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr,
                         EOAC_NONE, nullptr);

    if (!iMalloc)
    {
        result = CoGetMalloc(MEMCTX_TASK, &iMalloc);
        if (FAILED(result))
        {
            throw OPCException(L"COPCClient::init: CoGetMalloc FAILED");
        }
    } // if

    ++ReleaseCount;
    return true;

} // COPCClient::init

void COPCClient::stop()
{
    if (--ReleaseCount <= 0)
    {
        iMalloc.Release();
    }

    CoUninitialize();
} // COPCClient::stop

void COPCClient::comFree(void *memory)
{
    iMalloc->Free(memory);

} // COPCClient::comFree

void COPCClient::comFreeVariant(VARIANT *memory, unsigned size)
{
    for (unsigned i = 0; i < size; ++i)
        VariantClear(&(memory[i]));

    iMalloc->Free(memory);
} // COPCClient::comFreeVariant

COPCHost *COPCClient::makeHost(const std::wstring &hostName)
{
    if (!hostName.size() || (hostName == L"localhost") || (hostName == L"127.0.0.1"))
    {
        return new CLocalHost();
    }

    return new CRemoteHost(hostName);

} // COPCClient::makeHost


int COPCClient::Init(std::wstring progId, std::wstring server)
{
    this->host = COPCClient::makeHost(server);

    bool progExist = true;
    CLSID clsid = SearchClassID(progId, server, host,&progExist);
    if (!progExist)
    {
        delete (host);
        return -1;
    }

    this->opcServer = host->connectDAServer(clsid);
    if (this->opcServer == nullptr)
    {
        delete (host);
        return -1;
    }

    unsigned long refreshRate;
    this->demoGroup = opcServer->makeGroup(L"DemoGroup", true, 1000, refreshRate, 0.0);
    
    return 0;
}

ReadResult * COPCClient::ReadItem(std::wstring itemName)
{
    if (this->demoGroup == nullptr)
    {
        return new ReadResult(0, false, "", "empty group");
    }

    OPCItemData data;
    bool ret;
    COPCItem *readWritableItem;
    try
    {
        readWritableItem = demoGroup->addItem(itemName, true);
        ret = readWritableItem->readSync(data, OPC_DS_DEVICE);
    }
    catch (...)
    {
        return new ReadResult(0, false, "", "invalid item");
    }

    if (!ret)
    {
        return new ReadResult(0, false, "", "invalid item");
    }

    if (data.wQuality == 192)
    {
        return new ReadResult(data.Item->getDataType(), true, VariantToString(data), "");
    }
    else
    {
        return new ReadResult(0, false, "", "bad value");
    }
}

/*
std::string COPCClient::ReadItems(std::vector<std::wstring> itemNames)
{
    std::vector<COPCItem *> itemsCreated;
    std::vector<HRESULT> errors;

    std::unordered_map<std::wstring, ReadResult> retMap;

    demoGroup->addItems(itemNames, itemsCreated, errors, true);

    std::vector<COPCItem *> itemsRead;
    std::vector<std::wstring> itemsReadName;
    for (unsigned int i = 0; i < itemsCreated.size(); i++)
    {
        if (itemsCreated[i] == nullptr)
        {
            retMap[itemNames[i]] = ReadResult(0, false, "", "invalid item");
        }
        else
        {
            itemsRead.push_back(itemsCreated[i]);
            itemsReadName.push_back(itemNames[i]);
        }
    }

    if (itemsRead.size() == 0)
    {
        return ReadResult::String(retMap);
    }

    COPCItemDataMap itemDataMap;
    demoGroup->readSync(itemsRead, itemDataMap, OPC_DS_DEVICE);
    POSITION pos = itemDataMap.GetStartPosition();
    int index = 0;
    while (pos)
    {
        OPCHANDLE handle = itemDataMap.GetKeyAt(pos);
        OPCItemData *data = itemDataMap.GetNextValue(pos);
        if (data && data->item())
        {
            const COPCItem *item = data->item(); // retrieve original item pointer from item data..
            retMap[itemsReadName[index]] = ReadResult(data->Item->getDataType(), true, VariantToString(*data), "");
        }
        else
        {
            retMap[itemsReadName[index]] = ReadResult(0, false, "", "invalid item");
        }

        index++;
    } 
    return ReadResult::String(retMap);
}*/

COPCClient::~COPCClient()
{
    //if (host != nullptr)
    //{
   //     delete (host);
   // }
}

std::string COPCClient::VariantToString(const OPCItemData &data)
{
    std::string strValue;
    TCHAR szValue[1024] = {0x00};

    switch (data.Item->getDataType())
    {
    case VT_BSTR:  
    case VT_LPSTR:  
    case VT_LPWSTR:
        strValue = (LPCTSTR)data.vDataValue.bstrVal;
        break;

    case VT_I1:
    case VT_UI1:
        sprintf_s(szValue, _T("%d"), data.vDataValue.bVal);
        strValue = szValue;
        break;

    case VT_I2:
        sprintf_s(szValue, _T("%d"), data.vDataValue.iVal);
        strValue = szValue;
        break;

    case VT_UI2: 
        sprintf_s(szValue, _T("%d"), data.vDataValue.uiVal);
        strValue = szValue;
        break;

    case VT_INT: 
        sprintf_s(szValue, _T("%d"), data.vDataValue.intVal);
        strValue = szValue;
        break;

    case VT_I4: 
        sprintf_s(szValue, _T("%d"), data.vDataValue.lVal);
        strValue = szValue;
        break;

    case VT_I8: 
        sprintf_s(szValue, _T("%ld"), data.vDataValue.bVal);
        strValue = szValue;
        break;

    case VT_UINT: 
        sprintf_s(szValue, _T("%u"), data.vDataValue.uintVal);
        strValue = szValue;
        break;

    case VT_UI4: 
        sprintf_s(szValue, _T("%u"), data.vDataValue.ulVal);
        strValue = szValue;
        break;

    case VT_UI8: 
        sprintf_s(szValue, _T("%u"), data.vDataValue.ulVal);
        strValue = szValue;
        break;

    case VT_VOID:
        sprintf_s(szValue, _T("%8x"), (unsigned int)data.vDataValue.byref);
        strValue = szValue;
        break;

    case VT_R4: 
        sprintf_s(szValue, _T("%.4f"), data.vDataValue.fltVal);
        strValue = szValue;
        break;

    case VT_R8: 
        sprintf_s(szValue, _T("%.8f"), data.vDataValue.dblVal);
        strValue = szValue;
        break;

    case VT_DECIMAL: 
        sprintf_s(szValue, _T("%.4f"), data.vDataValue.decVal);
        strValue = szValue;
        break;

    case VT_CY: 
        //COleCurrency cy = data.vDataValue.cyVal;
        //strValue = cy.Format();
        break;

    case VT_BOOL: 
        strValue = data.vDataValue.boolVal ? "TRUE" : "FALSE";
        break;

    case VT_DATE: {
        DATE dt = data.vDataValue.date;
        // COleDateTime da = COleDateTime(dt);
        // strValue = da.Format("%Y-%m-%d %H:%M:%S");
        break;
    }
        

    case VT_NULL: 
        strValue = "VT_NULL";
        break;

    case VT_EMPTY: 
        strValue = "";
        break;

    case VT_UNKNOWN: 
    default:
        strValue = "UN_KNOWN";
        break;
    }

    return strValue;
}


CLSID COPCClient::SearchClassID(const std::wstring &progId, const std::wstring &server, COPCHost *host, bool *found)
{
    std::vector<CLSID> localClassIdList;
    std::vector<std::wstring> localServerList;
    HRESULT ret = host->getListOfDAServers(IID_CATID_OPCDAServer20, localServerList, localClassIdList);
    if (FAILED(ret))
    {
        return CLSID{0,0,0,0};
    }

    for (size_t i = 0; i < localServerList.size(); i++)
    {
        if (localServerList.at(i) == progId)
        {
            return localClassIdList.at(i);
        }
    }

    *found = false;
    return CLSID{0, 0, 0, 0};
}

ReadResult::ReadResult(char type, bool ok, std::string val, std::string errormsg)
{
    this->type = type;
    this->ok = ok;
    this->val = val;
    this->errormsg = errormsg;
}

ReadResult::ReadResult()
{
}

ReadResult::ReadResult(const ReadResult &ret)
{
    this->type = ret.type;
    this->errormsg = ret.errormsg;
    this->ok = ret.ok;
    this->val = ret.val;
}

char ReadResult::GetType()
{
    return this->type;
}

bool ReadResult::GetOK()
{
    return this->ok;
}

std::string ReadResult::GetVal()
{
    return this->val;
}

std::string ReadResult::GetError()
{
    return this->errormsg;
}

std::string ReadResult::String(const ReadResult & ret)
{
    picojson::value v;

    v.set<picojson::object>(picojson::object());
    v.get<picojson::object>()["ok"] = picojson::value(ret.ok);
    v.get<picojson::object>()["type"] = picojson::value(double(ret.type));
    v.get<picojson::object>()["val"] = picojson::value(ret.val);
    v.get<picojson::object>()["error"] = picojson::value(ret.errormsg);

   return v.serialize();
}

std::string ReadResult::String(std::unordered_map<std::string, ReadResult> retMap)
{
    picojson::value v;
    v.set<picojson::object>(picojson::object());
    for (auto iter = retMap.begin(); iter != retMap.end(); iter++)
    {
        picojson::value retValue;
        retValue.set<picojson::object>(picojson::object());
        retValue.get<picojson::object>()["ok"] = picojson::value(iter->second.ok);
        retValue.get<picojson::object>()["type"] = picojson::value(double(iter->second.type));
        retValue.get<picojson::object>()["val"] = picojson::value(iter->second.val);
        retValue.get<picojson::object>()["error"] = picojson::value(iter->second.errormsg);
        v.get<picojson::object>()[iter->first] = retValue;
    }

    return v.serialize();
}

std::string ReadResult::String(std::vector<ReadResult> retVector)
{
    picojson::value v;
   
    for (auto iter = retVector.begin(); iter != retVector.end(); iter++)
    {
        /*
        picojson::value retValue;
        retValue.set<picojson::array>(picojson::object());
        retValue.get<picojson::object>()["ok"] = picojson::value(iter->second.ok);
        retValue.get<picojson::object>()["type"] = picojson::value(double(iter->second.type));
        retValue.get<picojson::object>()["val"] = picojson::value(iter->second.val);
        retValue.get<picojson::object>()["error"] = picojson::value(iter->second.errormsg);
        v.get<picojson::object>()[COPCHost::WS2S(iter->first)] = retValue;*/
    }

    return v.serialize();
}

#ifdef OPCDA_CLIENT_NAMESPACE
} // namespace opcda_client
#endif
