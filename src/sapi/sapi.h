// Copyright (c) 2017 - 2018 - The SmartCash Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SMARTCASH_SAPI_H
#define SMARTCASH_SAPI_H

#include "httpserver.h"
#include "rpc/protocol.h"
#include "rpc/server.h"
#include <string>
#include <stdint.h>
#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>

namespace SAPI{

extern std::string versionSubPath;
extern std::string versionString;

struct Result;

enum Codes{
    Valid = 0,
    Undefined = 1,
    /* Parameter errors */
    ParameterMissing = 1000,
    InvalidType,
    NumberParserFailed,
    UnsignedExpected,
    IntOverflow,
    IntOutOfRange,
    UIntOverflow,
    UIntOutOfRange,
    DoubleOverflow,
    DoubleOutOfRange,
    InvalidSmartCashAddress,
    EmptyString,
    InvalidHexString,
    /* common errors */
    TimedOut = 2000,
    PageOutOfRange,
    BalanceTooLow,
    /* block errors */
    BlockHeightOutOfRange = 3000,
    BlockNotFound,
    BlockNotSpecified,
    /* address errors */
    AddressNotFound = 4000,
    NoDepositAvailble,
    NoUtxosAvailble,
    /* transaction errors */
    TxDecodeFailed = 5000,
    TxNotSpecified,
    TxNoValidInstantPay,
    TxRejected,
    TxMissingInputs,
    TxAlreadyInBlockchain,
    TxCantRelay,
    TxNotFound
};

namespace Keys{

    const std::string address = "address";
    const std::string timestampFrom = "from";
    const std::string timestampTo = "to";
    const std::string pageNumber = "pageNumber";
    const std::string pageSize = "pageSize";
    const std::string amount = "amount";
    const std::string rawtx = "data";
    const std::string instantpay = "instantpay";
    const std::string overridefees = "overrideFees";
    const std::string ascending = "ascending";
    const std::string descending = "descending";
    const std::string random = "random";
    const std::string maxInputs = "maxInputs";

}

namespace Validation{

    class Base{
        UniValue::VType type;
    public:
        Base(UniValue::VType type) : type(type) {}
        virtual SAPI::Result Validate(const std::string &parameter, const UniValue &value) const;
        UniValue::VType GetType() const { return type; }
    };

    class Bool : public Base{
    public:
        Bool() : Base(UniValue::VBOOL) {}
        SAPI::Result Validate(const std::string &parameter, const UniValue &value) const override;
    };


    class String : public Base{
    public:
        String() : Base(UniValue::VSTR) {}
        SAPI::Result Validate(const std::string &parameter, const UniValue &value) const override;
    };

    class HexString : public String{
    public:
        HexString() : String() {}
        SAPI::Result Validate(const std::string &parameter, const UniValue &value) const final;
    };

    class SmartCashAddress : public Base{
    public:
        SmartCashAddress() : Base(UniValue::VSTR) {}
        SAPI::Result Validate(const std::string &parameter, const UniValue &value) const final;
    };

    class Int : public Base{
    public:
        Int() : Base(UniValue::VNUM) {}
        SAPI::Result Validate(const std::string &parameter, const UniValue &value) const override;
    };

    class IntRange : public Int{
        int64_t min;
        int64_t max;
    public:
        IntRange( int64_t min, int64_t max ) : Int(), min(min), max(max) {}
        SAPI::Result Validate(const std::string &parameter, const UniValue &value) const final;
    };

    class UInt : public Base{
    public:
        UInt() : Base(UniValue::VNUM) {}
        SAPI::Result Validate(const std::string &parameter, const UniValue &value) const override;
    };

    class UIntRange : public UInt{
        uint64_t min;
        uint64_t max;
    public:
        UIntRange( uint64_t min, uint64_t max ) : UInt(), min(min), max(max) {}
        SAPI::Result Validate(const std::string &parameter, const UniValue &value) const final;
    };

    class Double : public Base{
    public:
        Double() : Base(UniValue::VNUM) {}
        SAPI::Result Validate(const std::string &parameter, const UniValue &value) const override;
    };

    class DoubleRange : public Double{
        double min;
        double max;
    public:
        DoubleRange( double min, double max ) : Double(), min(min), max(max) {}
        SAPI::Result Validate(const std::string &parameter, const UniValue &value) const final;
    };

    std::string ResultMessage(SAPI::Codes value);
}

struct BodyParameter{
    std::string key;
    const SAPI::Validation::Base *validator;
    bool optional;
    BodyParameter(const std::string &key,
                  const SAPI::Validation::Base *validator, bool optional = false) : key(key),
                                                                                    validator(validator),
                                                                                    optional(optional){}
};

struct Result{
    Codes code;
    std::string message;
    Result() : code(SAPI::Valid), message(std::string()) {}
    Result(SAPI::Codes code, std::string message) : code(code), message(message) {}
    friend bool operator==(const Result& a, const Codes& b)
    {
        return (a.code == b);
    }
    friend bool operator!=(const Result& a, const Codes& b)
    {
        return !(a == b);
    }
    UniValue ToUniValue() const {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("code", code);
        obj.pushKV("message", message);
        return obj;
    }
};

typedef struct {
    std::string path;
    HTTPRequest::RequestMethod method;
    UniValue::VType bodyRoot;
    bool (*handler)(HTTPRequest* req, const std::map<std::string, std::string> &mapPathParams, const UniValue &bodyParameter);
    std::vector<SAPI::BodyParameter> vecBodyParameter;
}Endpoint;

typedef struct{
    std::string prefix;
    std::vector<Endpoint> endpoints;
}EndpointGroup;

void AddDefaultHeaders(HTTPRequest* req);

bool Error(HTTPRequest* req, HTTPStatus::Codes status, const std::string &message);
bool Error(HTTPRequest* req, HTTPStatus::Codes status, const std::vector<SAPI::Result> &errors);
bool Error(HTTPRequest* req, SAPI::Codes code, const std::string &message);

void WriteReply(HTTPRequest *req, HTTPStatus::Codes status, const UniValue &obj);
void WriteReply(HTTPRequest *req, HTTPStatus::Codes status, const std::string &str);
void WriteReply(HTTPRequest *req, const UniValue& obj);
void WriteReply(HTTPRequest *req, const std::string &str);

bool CheckWarmup(HTTPRequest* req);

bool UnknownEndpointHandler(HTTPRequest* req, const std::string& strURIPart);

}

extern bool getAddressFromIndex(const int &type, const uint160 &hash, std::string &address);

extern bool ParseHashStr(const string& strHash, uint256& v);

inline std::string JsonString(const UniValue& obj);

/** Initialize SAPI server. */
bool InitSAPIServer();
/** Start SAPI server. */
bool StartSAPIServer();
/** Interrupt SAPI server threads */
void InterruptSAPIServer();
/** Stop SAPI server */
void StopSAPIServer();

/** Start SAPI.
 * This is separate from InitSAPIServer to give users race-condition-free time
 * to register their handlers between InitSAPIServer and StartSAPIServer.
 */
bool StartSAPI();
/** Interrupt SAPI server threads */
void InterruptSAPI();
/** Stop SAPI server */
void StopSAPI();

/** Handler for requests to a certain HTTP path */
typedef std::function<bool(HTTPRequest*, const std::map<std::string, std::string> &, const SAPI::Endpoint *)> SAPIRequestHandler;

/** SAPI request work item */
class SAPIWorkItem : public HTTPClosure
{
public:
    SAPIWorkItem(std::unique_ptr<HTTPRequest> req,
                 const std::map<std::string, std::string> &mapPathParams,
                 const SAPI::Endpoint *endpoint, const SAPIRequestHandler& func):
        req(std::move(req)), mapPathParams(mapPathParams), endpoint(endpoint), func(func)
    {
    }
    void operator()()
    {
        func(req.get(), mapPathParams, endpoint);
    }

    std::unique_ptr<HTTPRequest> req;

private:
    const std::map<std::string, std::string> mapPathParams;
    const SAPI::Endpoint *endpoint;
    SAPIRequestHandler func;
};

#endif // SMARTCASH_SAPI_H
