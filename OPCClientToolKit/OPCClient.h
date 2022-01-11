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

#pragma once

#pragma warning(disable : 4251) // can be ignored if deriving from a type in the Standard C++ Library..

#ifndef OPCCLIENT_H
#define OPCCLIENT_H

#include <COMCat.h>
#include <atlbase.h>
#include <atlcoll.h>
#include <atlexcept.h>
#include <atlstr.h>
#include <objbase.h>
#include <stdexcept>
#include <unordered_map>

#include "OPCClientToolKitDLL.h"
#include "OPCItemData.h"
#include "opcda.h"


#ifdef OPCDA_CLIENT_NAMESPACE
namespace opcda_client
{
#endif

class COPCHost;

class COPCServer;

class COPCGroup;

class COPCItem;

/**
 * Basic OPC exception
 */
class OPCException : public ATL::CAtlException
{
  private:
    std::wstring ExceptionMsg;

  public:
    OPCException(const std::wstring &msg, HRESULT code = 0) : ExceptionMsg(msg)
    {
        (void)code;
    }

    const std::wstring &reasonString() const
    {
        return ExceptionMsg;
    }

}; // OPCException

/**
 * Data received from the OnDataChange() method of the CAsyncDataCallback instance is delegated to an instance
 * of a child class implementing this interface. The Child class must obviously provide the desired behaviour
 * in the overriden OnDataChange() method. This interface is active only when the corresponding group is active
 * (achieved by the groups enableAsync() method.)
 */
class IAsyncDataCallback
{
  public:
    virtual void OnDataChange(COPCGroup &group, COPCItemDataMap &changes) = 0;

}; // IAsyncDataCallback

/**
 * Starting point for 'everything'. Utility class that creates host objects and handles COM memory management.
 * Effectively a singleton.
 */

enum OPCOLEInitMode
{
    APARTMENTTHREADED,
    MULTITHREADED
}; // OPCOLEInitMode

class ReadResult
{
  private:
    char type;
    bool ok;
    std::string val;
    std::string errormsg;

  public:
    ReadResult();
    ReadResult(const ReadResult &ret);
    ReadResult(char type, bool ok, std::string val, std::string errormsg);

    static std::string String(const ReadResult &ret);
    static std::string String(std::unordered_map<std::string, ReadResult> retMap);
    static std::string String(std::vector<ReadResult> retVector);

    char GetType();
    bool GetOK();
    std::string GetVal();
    std::string GetError();
};


class OPCDACLIENT_API COPCClient
{
  private:
    static ATL::CComPtr<IMalloc> iMalloc;
    COPCHost *host;
    COPCServer *opcServer;
    COPCGroup *demoGroup;

  public:
    static int ReleaseCount;

    static bool init(OPCOLEInitMode mode = APARTMENTTHREADED);

    static void stop();

    static void comFree(void *memory);

    static void comFreeVariant(VARIANT *memory, unsigned size);

    /**
     * make a host machine abstraction.
     * @param hostname - may be empty (in which case a local host is created).
     * @ returns host object (owned by caller).
     */
    static COPCHost *makeHost(const std::wstring &hostName);
    static const GUID CATID_OPCDAv10;
    static const GUID CATID_OPCDAv20;
    
    
    static CLSID SearchClassID(const std::wstring &progId, const std::wstring &server, 
        COPCHost *host, bool *found);
    int Init(std::wstring progId, std::wstring server);
    ReadResult *ReadItem(std::wstring item);
    static std::string VariantToString(const OPCItemData &data);
    ~COPCClient();
}; // COPCClient


#ifdef OPCDA_CLIENT_NAMESPACE
} // namespace opcda_client
#endif

#endif