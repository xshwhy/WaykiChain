// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ENTITIES_ID_H
#define ENTITIES_ID_H

#include <memory>
#include <string>
#include <vector>
#include <variant>

#include "crypto/hash.h"
#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "key.h"
#include "commons/types.h"
#include "commons/leb128.h"

class CAccountDBCache;
class CUserID;
class CRegID;

enum AccountIDType {
    NULL_ID = 0,
    NICK_ID = 1,
    REG_ID  = 2,
    ADDRESS = 3,
};

typedef vector<uint8_t> UnsignedCharArray;
typedef CRegID CTxCord;

class CNullID {
public:
    friend bool operator==(const CNullID &a, const CNullID &b) { return true; }
    friend bool operator<(const CNullID &a, const CNullID &b) { return true; }

    inline constexpr bool IsEmpty() const { return true; }
    string ToString() const { return "Null"; }
};

class CRegIDKey;

class CRegID {
public:
    CRegID(const string &strRegID);
    CRegID(const vector<uint8_t> &vIn);
    CRegID(const uint32_t height = 0, const uint16_t index = 0);

    const vector<uint8_t> &GetRegIdRaw() const;

    void SetRegID(const vector<uint8_t> &vIn);
    CKeyID GetKeyId(const CAccountDBCache &accountCache) const;
    uint32_t GetHeight() const { return height; }
    uint16_t GetIndex() const { return index; }

    bool IsMature(uint32_t curHeight) const;

    bool operator==(const CRegID &other) const { return (this->height == other.height && this->index == other.index); }
    bool operator!=(const CRegID &other) const { return (this->height != other.height || this->index != other.index); }
    bool operator<(const CRegID &other) const {
        if (this->height == other.height) {
            return this->index < other.index;
        } else {
            return this->height < other.height;
        }
    }

    static bool IsSimpleRegIdStr(const string &str);
    static bool IsRegIdStr(const string &str);
    static bool GetKeyId(const string &str, CKeyID &keyId);
    inline constexpr bool IsEmpty() const { return (height == 0 && index == 0); }
    void SetEmpty() { Clear(); }
    bool Clear();
    string ToString() const;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(height));
        READWRITE(VARINT(index));
        if (fRead) {
            vRegID.clear();
            vRegID.insert(vRegID.end(), BEGIN(height), END(height));
            vRegID.insert(vRegID.end(), BEGIN(index), END(index));
        }
    )
private:
    uint32_t height;
    uint16_t index;
    mutable vector<uint8_t> vRegID;

    void SetRegID(string strRegID);
    void SetRegIDByCompact(const vector<uint8_t> &vIn);

    friend CUserID;
    friend class CRegIDKey;
};

class CRegIDKey {
public:
    CRegID regid;

    CRegIDKey() {}

    CRegIDKey(const CRegID &regidIn): regid(regidIn) {}

    IMPLEMENT_SERIALIZE(
        READWRITE_FIXED_UINT32(regid.height);
        READWRITE_FIXED_UINT16(regid.index);
    )

    inline bool IsEmpty() const { return regid.IsEmpty(); }
    void SetEmpty() { regid.SetEmpty(); }


    bool operator==(const CRegIDKey &other) const { return this->regid == other.regid; }
    bool operator!=(const CRegIDKey &other) const { return this->regid != other.regid; }
    bool operator<(const CRegIDKey &other) const { return this->regid < other.regid; }
};

class CNickID {
public:
    uint64_t value = 0 ;

    CNickID();

    CNickID(uint64_t nickIdIn);

    CNickID(string nickIdIn);

    bool IsMature(const uint32_t currHeight) const ;
    bool IsEmpty() const;
    void SetEmpty();
    void Clear();
    string ToString() const;

    IMPLEMENT_SERIALIZE(READWRITE(VARINT(value));)

    // Comparator implementation.
    friend bool operator==(const CNickID &a, const CNickID &b) { return a.value == b.value; }
    friend bool operator!=(const CNickID &a, const CNickID &b) { return a.value != b.value; }
    friend bool operator<(const CNickID &a, const CNickID &b) { return a.value < b.value; }
};

class CUserID {
private:
    std::variant<CNullID, CRegID, CKeyID, CPubKey, CNickID> uid;
    enum VarIndex: size_t {IDX_NULL_ID, IDX_REG_ID, IDX_KEY_ID, IDX_PUB_KEY_ID, IDX_NICK_ID};

public:
    enum SerializeFlag {
        FlagNullType    = 0,
        FlagRegIDMin    = 2,
        FlagRegIDMax    = 10,
        FlagKeyID       = 20,
        FlagPubKey      = 33,  // public key
        FlagNickID      = 100
    };

public:
    static std::shared_ptr<CUserID> ParseUserId(const string &idStr);
    static const CUserID NULL_ID;
    static const EnumTypeMap<VarIndex, string> ID_NAME_MAP;
public:
    CUserID() : uid(CNullID()) {}

    template <typename ID>
    CUserID(const ID &id) : uid(CNullID()) {
        set(id);
    }

    template <typename ID>
    CUserID &operator=(const ID &id) {
        set(id);
        return *this;
    }

    template <typename ID>
    ID& get() {
        return std::get<ID>(uid);
    }

    template <typename ID>
    const ID& get() const {
        return std::get<ID>(uid);
    }

    template <typename ID>
    inline constexpr bool is() const {
        return std::holds_alternative<ID>(uid);
    }

    inline bool IsEmpty() const {
        return is<CNullID>();
    }

    inline void SetEmpty() { uid = CNullID(); }

    template <typename ID>
    void set(const ID &idIn) {
        if (std::is_same_v<ID, CNullID> || !idIn.IsEmpty()) {
            uid = idIn;
        } else {
            SetEmpty();
        }
    }

    inline constexpr bool is_same_type(const CUserID &other) const {
        return this->uid.index() == other.uid.index();
    }

public:
    const std::string& GetIDName() const {
        auto it = ID_NAME_MAP.find((VarIndex)uid.index());
        if (it != ID_NAME_MAP.end())
            return it->second;
        return EMPTY_STRING;
    }

    string ToString() const {
        return std::visit([](auto&& uidIn){ return uidIn.ToString(); }, uid);
    }

    string ToDebugString() const;

    friend bool operator==(const CUserID &id1, const CUserID &id2) {
        return id1.uid == id2.uid;
    }
    friend bool operator!=(const CUserID &id1, const CUserID &id2) {
        return !(id1.uid == id2.uid);
    }

    json_spirit::Object ToJson() const {
        json_spirit::Object obj;
        string id = ToString();
        obj.push_back(json_spirit::Pair("id_type", GetIDName()));
        obj.push_back(json_spirit::Pair("id", id));
        return obj;
    }

    inline unsigned int GetSerializeSize(int nType, int nVersion) const {

        return std::visit([&](auto&& idIn) -> uint32_t {
            using ID = std::decay_t<decltype(idIn)>;
            if constexpr (std::is_same_v<ID, CRegID>) {
                unsigned int sz = idIn.GetSerializeSize(nType, nVersion);
                assert(FlagRegIDMin <= sz && sz <= FlagRegIDMax);
                return sizeof(uint8_t) + sz;
            } else if constexpr (std::is_same_v<ID, CKeyID>) {
                unsigned int sz = idIn.GetSerializeSize(nType, nVersion);
                assert(sz == FlagKeyID);
                return sizeof(uint8_t) + sz;
            } else if constexpr (std::is_same_v<ID, CPubKey>) {
                unsigned int sz = idIn.GetSerializeSize(nType, nVersion);
                // If the public key is empty, length of serialized data is 1, otherwise, 34.
                assert(sz == sizeof(uint8_t) + FlagPubKey);
                return sz;
            } else if constexpr (std::is_same_v<ID, CNickID>) {
                return sizeof(uint8_t) + idIn.GetSerializeSize(nType, nVersion);
            } else {  // CNullID
                assert( (std::is_same_v<ID, CNullID>) );
                return sizeof(uint8_t);
            }
        }, uid);
    }

    template <typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        std::visit([&](auto&& idIn) {
            using ID = std::decay_t<decltype(idIn)>;
            if constexpr (std::is_same_v<ID, CRegID>) {
                unsigned int sz = idIn.GetSerializeSize(nType, nVersion);
                assert(FlagRegIDMin <= sz && sz <= FlagRegIDMax);
                s << (uint8_t)sz << idIn;
            } else if constexpr (std::is_same_v<ID, CKeyID>) {
                assert(idIn.GetSerializeSize(nType, nVersion) == FlagKeyID);
                s << (uint8_t)FlagKeyID << idIn;
            } else if constexpr (std::is_same_v<ID, CPubKey>) {
                // If the public key is empty, length of serialized data is 1, otherwise, 34.
                assert(idIn.GetSerializeSize(nType, nVersion) == sizeof(uint8_t) + FlagPubKey);
                s << idIn;
            } else if constexpr (std::is_same_v<ID, CNickID>) {
                s << (uint8_t)FlagNickID << idIn;
            } else {  // CNullID
                assert( (std::is_same_v<ID, CNullID>) );
                s << (uint8_t)0;
            }
        }, uid);
    }

    template <typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        bool invalidId         = false;
        SerializeFlag typeFlag = (SerializeFlag)ReadCompactSize(s);

        if (typeFlag == FlagNickID) {  // idType >= 100
            CNickID nickId;
            s >> nickId;
            uid = nickId;
        } else {  // typeFlag is the length of id content
            int len = typeFlag;
            if (FlagRegIDMin <= len && len <= FlagRegIDMax) {
                CRegID regId(0, 0);
                s >> regId;
                uid = regId;
            } else if (len == FlagKeyID) {
                UnsignedCharArray vchData;
                vchData.resize(len);
                s.read((char *)&vchData[0], len * sizeof(vchData[0]));
                uint160 data = uint160(vchData);
                CKeyID keyId(data);
                uid = keyId;
            } else if (len == FlagPubKey) {  // public key
                UnsignedCharArray vchData;
                vchData.resize(len);
                s.read((char *)&vchData[0], len * sizeof(vchData[0]));
                CPubKey pubKey(vchData);
                uid = pubKey;
            } else if (len == FlagNullType) {  // null id type
                uid = CNullID();
            } else {
                invalidId = true;
            }
        }

        if (invalidId) {
            LogPrint(BCLog::ERROR, "Invalid Unserialize CUserID\n");
            throw ios_base::failure("Unserialize CUserID error");
        }
    }
};

#endif //ENTITIES_ID_H