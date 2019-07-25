// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_BASETX_H
#define COIN_BASETX_H

#include "commons/serialize.h"
#include "commons/uint256.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "entities/account.h"
#include "entities/asset.h"
#include "entities/id.h"
#include "persistence/contractdb.h"
#include "config/configuration.h"
#include "config/txbase.h"
#include "config/scoin.h"

#include <boost/variant.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

class CCacheWrapper;
class CValidationState;
class CContractDBCache;

typedef uint256 TxID;

string GetTxType(const TxType txType);
uint64_t GetTxMinFee(const TxType nTxType, int height);

class CBaseTx {
public:
    static uint64_t nMinTxFee;
    static uint64_t nMinRelayTxFee;
    static uint64_t nDustAmountThreshold;
    static const int32_t CURRENT_VERSION = nTxVersion1;

    int32_t nVersion;
    TxType nTxType;
    mutable CUserID txUid;
    int32_t nValidHeight;
    uint64_t llFees;
    UnsignedCharArray signature;

    uint64_t nRunStep;        //!< only in memory
    int32_t nFuelRate;        //!< only in memory
    mutable TxID sigHash;  //!< only in memory

public:
    CBaseTx(const CBaseTx &other) { *this = other; }

    CBaseTx(int32_t nVersionIn, TxType nTxTypeIn, CUserID txUidIn, int32_t nValidHeightIn, uint64_t llFeesIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn), txUid(txUidIn), nValidHeight(nValidHeightIn), llFees(llFeesIn),
        nRunStep(0), nFuelRate(0) {}

    CBaseTx(TxType nTxTypeIn, CUserID txUidIn, int32_t nValidHeightIn, uint64_t llFeesIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn), txUid(txUidIn), nValidHeight(nValidHeightIn), llFees(llFeesIn),
        nRunStep(0), nFuelRate(0) {}

    CBaseTx(int32_t nVersionIn, TxType nTxTypeIn) :
        nVersion(nVersionIn), nTxType(nTxTypeIn),
        nValidHeight(0), llFees(0), nRunStep(0), nFuelRate(0) {}

    CBaseTx(TxType nTxTypeIn) :
        nVersion(CURRENT_VERSION), nTxType(nTxTypeIn),
        nValidHeight(0), llFees(0), nRunStep(0), nFuelRate(0) {}

    virtual ~CBaseTx() {}

    virtual std::pair<TokenSymbol, uint64_t> GetFees() const { return std::make_pair(SYMB::WICC, llFees); }
    virtual TxID GetHash() const { return ComputeSignatureHash(); }
    virtual uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const { return 0; }

    virtual uint64_t GetFuel(int32_t nFuelRate);
    virtual double GetPriority() const {
        return kTransactionPriorityCeiling / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
    }
    virtual map<TokenSymbol, uint64_t> GetValues() const = 0;

    virtual TxID ComputeSignatureHash(bool recalculate = false) const = 0;
    virtual std::shared_ptr<CBaseTx> GetNewInstance()                 = 0;

    virtual string ToString(CAccountDBCache &accountCache)                           = 0;
    virtual Object ToJson(const CAccountDBCache &accountCache) const                 = 0;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state)                       = 0;
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state)     = 0;

    int32_t GetFuelRate(CContractDBCache &contractCache);
    bool IsValidHeight(int32_t nCurHeight, int32_t nTxCacheHeight) const;
    bool IsCoinBase() { return nTxType == BLOCK_REWARD_TX || nTxType == UCOIN_BLOCK_REWARD_TX; }

protected:
    bool CheckTxFeeSufficient(const uint64_t llFees, const int32_t height) const;
    bool CheckSignatureSize(const vector<unsigned char> &signature) const;
    static bool AddInvolvedKeyIds(vector<CUserID> uids, CCacheWrapper &cw, set<CKeyID> &keyIds);
    bool SaveTxAddresses(uint32_t height, uint32_t index, CCacheWrapper &cw,
                                CValidationState &state, const vector<CUserID> &userIds);
};

class CCoinPriceType {
public:
    TokenSymbol coinType;
    PriceSymbol priceType;

public:
    CCoinPriceType() {}

    CCoinPriceType(const TokenSymbol &coinTypeIn, const PriceSymbol &priceTypeIn) :
        coinType(coinTypeIn), priceType(priceTypeIn) {}

    CCoinPriceType(const CCoinPriceType &other) {
        *this = other;
    }

    ~CCoinPriceType() {}

public:
    bool operator<(const CCoinPriceType &coinPriceType) const {
        if (coinType == coinPriceType.coinType) {
            return priceType < coinPriceType.priceType;
        } else {
            return coinType < coinPriceType.coinType;
        }
    }

    bool operator==(const CCoinPriceType &rhs) const {
        return (this->coinType == rhs.coinType && this->priceType == rhs.priceType);
    }

    string ToString() { return strprintf("%u%u", coinType, priceType); }

    IMPLEMENT_SERIALIZE(
        READWRITE(coinType);
        READWRITE(priceType);)

    CCoinPriceType& operator=(const CCoinPriceType& other) {
        if (this == &other)
            return *this;

        this->coinType  = other.coinType;
        this->priceType = other.priceType;

        return *this;
    }
};

class CPricePoint {
public:
    CCoinPriceType coinPriceType;
    uint64_t price;

public:
    CPricePoint() {}

    CPricePoint(const CCoinPriceType &coinPriceTypeIn, const uint64_t priceIn)
        : coinPriceType(coinPriceTypeIn), price(priceIn) {}

    CPricePoint(const TokenSymbol &coinTypeIn, const PriceSymbol &priceTypeIn, const uint64_t priceIn)
        : coinPriceType(coinTypeIn, priceTypeIn), price(priceIn) {}

    CPricePoint(const CPricePoint& other) {
        *this = other;
    }

    ~CPricePoint() {}

public:
    uint64_t GetPrice() { return price; }
    CCoinPriceType GetCoinPriceType() { return coinPriceType; }

    string ToString() {
        return strprintf("coinType:%u, priceType:%u, price:%lld",
                        coinPriceType.coinType, coinPriceType.priceType, price);
    }

    json_spirit::Object ToJson() const {
        json_spirit::Object obj;

        obj.push_back(json_spirit::Pair("coin_type",    coinPriceType.coinType));
        obj.push_back(json_spirit::Pair("price_type",   coinPriceType.priceType));
        obj.push_back(json_spirit::Pair("price",        price));

        return obj;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(coinPriceType);
        READWRITE(VARINT(price));)

    CPricePoint& operator=(const CPricePoint& other) {
        if (this == &other)
            return *this;

        this->coinPriceType = other.coinPriceType;
        this->price         = other.price;

        return *this;
    }
};

#define IMPLEMENT_CHECK_TX_MEMO                                                                             \
    if (memo.size() > kCommonTxMemoMaxSize)                                                                 \
        return state.DoS(100, ERRORMSG("%s::CheckTx, memo's size too large", __FUNCTION__), REJECT_INVALID, \
                         "memo-size-toolarge");

#define IMPLEMENT_CHECK_TX_ARGUMENTS                                                                             \
    if (arguments.size() > kContractArgumentMaxSize)                                                             \
        return state.DoS(100, ERRORMSG("%s::CheckTx, arguments's size too large, __FUNCTION__"), REJECT_INVALID, \
                         "arguments-size-toolarge");

#define IMPLEMENT_CHECK_TX_FEE                                                                                     \
    if (!CheckBaseCoinRange(llFees))                                                                               \
        return state.DoS(100, ERRORMSG("%s::CheckTx, tx fee out of range", __FUNCTION__), REJECT_INVALID,          \
                         "bad-tx-fee-toolarge");                                                                   \
                                                                                                                   \
    if (!CheckTxFeeSufficient(llFees, height)) {                                                                   \
        return state.DoS(100, ERRORMSG("%s::CheckTx, tx fee smaller than MinTxFee", __FUNCTION__), REJECT_INVALID, \
                         "bad-tx-fee-toosmall");                                                                   \
    }

#define IMPLEMENT_CHECK_TX_REGID(txUidType)                                                                \
    if (txUidType != typeid(CRegID)) {                                                                     \
        return state.DoS(100, ERRORMSG("%s::CheckTx, txUid must be CRegID", __FUNCTION__), REJECT_INVALID, \
                         "txUid-type-error");                                                              \
    }

#define IMPLEMENT_CHECK_TX_APPID(appUidType)                                                                \
    if (appUidType != typeid(CRegID)) {                                                                     \
        return state.DoS(100, ERRORMSG("%s::CheckTx, appUid must be CRegID", __FUNCTION__), REJECT_INVALID, \
                         "appUid-type-error");                                                              \
    }

#define IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUidType)                                                                 \
    if ((txUidType != typeid(CRegID)) && (txUidType != typeid(CPubKey))) {                                            \
        return state.DoS(100, ERRORMSG("%s::CheckTx, txUid must be CRegID or CPubKey", __FUNCTION__), REJECT_INVALID, \
                         "txUid-type-error");                                                                         \
    }

#define IMPLEMENT_CHECK_TX_REGID_OR_KEYID(toUidType)                                                                 \
    if ((toUidType != typeid(CRegID)) && (toUidType != typeid(CKeyID))) {                                            \
        return state.DoS(100, ERRORMSG("%s::CheckTx, toUid must be CRegID or CKeyID", __FUNCTION__), REJECT_INVALID, \
                         "toUid-type-error");                                                                        \
    }

#define IMPLEMENT_CHECK_TX_SIGNATURE(signatureVerifyPubKey)                                                     \
    if (!CheckSignatureSize(signature)) {                                                                       \
        return state.DoS(100, ERRORMSG("%s::CheckTx, tx signature size invalid", __FUNCTION__), REJECT_INVALID, \
                         "bad-tx-sig-size");                                                                    \
    }                                                                                                           \
    uint256 sighash = ComputeSignatureHash();                                                                   \
    if (!VerifySignature(sighash, signature, signatureVerifyPubKey)) {                                          \
        return state.DoS(100, ERRORMSG("%s::CheckTx, tx signature error", __FUNCTION__), REJECT_INVALID,        \
                         "bad-tx-signature");                                                                   \
    }

#define IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache)                                          \
    CKeyID srcKeyId;                                                                            \
    accountCache.GetKeyId(txUid, srcKeyId);                                                     \
    result.push_back(Pair("txid",           GetHash().GetHex()));                               \
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));                               \
    result.push_back(Pair("ver",            nVersion));                                         \
    result.push_back(Pair("tx_uid",         txUid.ToString()));                                 \
    result.push_back(Pair("addr",           srcKeyId.ToAddress()));                             \
    result.push_back(Pair("fees",           llFees));                                           \
    result.push_back(Pair("valid_height",   nValidHeight));                                     \

#endif //COIN_BASETX_H
