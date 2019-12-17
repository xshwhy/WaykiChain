#ifndef TX_WASM_CONTRACT_TX_H
#define TX_WASM_CONTRACT_TX_H

#include "tx.h"
#include "wasm/types/inline_transaction.hpp"
#include "wasm/wasm_trace.hpp"
#include "chrono"

using std::chrono::microseconds;
using std::chrono::system_clock;

struct signature_type {
    uint64_t          account;
    UnsignedCharArray signature;

    IMPLEMENT_SERIALIZE (
    READWRITE(VARINT(account ));
    READWRITE(signature);
    )

};

void to_variant( const wasm::permission &t, json_spirit::Value &v ) ;
void to_variant( const wasm::inline_transaction &t, json_spirit::Value &v , CCacheWrapper &database) ;
void to_variant( const wasm::inline_transaction_trace &t, json_spirit::Value &v, CCacheWrapper &database) ;
void to_variant( const wasm::transaction_trace &t, json_spirit::Value &v, CCacheWrapper &database ) ;

class CWasmContractTx : public CBaseTx {
public:
    vector<wasm::inline_transaction> inlinetransactions;
    vector<signature_type>           signatures;
    //uint64_t payer;

public:
    bool mining;
    system_clock::time_point pseudo_start;
    std::chrono::microseconds billed_time = chrono::microseconds(0);

    void pause_billing_timer();
    void resume_billing_timer();

public:
    CWasmContractTx(const CBaseTx *pBaseTx): CBaseTx(WASM_CONTRACT_TX) {
        assert(WASM_CONTRACT_TX == pBaseTx->nTxType);
        *this = *(CWasmContractTx *)pBaseTx;
    }
    CWasmContractTx(): CBaseTx(WASM_CONTRACT_TX) {}
    ~CWasmContractTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(inlinetransactions);
        READWRITE(signatures);
        READWRITE(VARINT(llFees));
        READWRITE(signature);
        )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
                << inlinetransactions
               << VARINT(llFees);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CWasmContractTx>(*this); }
    virtual uint64_t GetFuel(int32_t height, uint32_t fuelRate);
    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

public:
    void contract_validation(CTxExecuteContext &context);
    void authorization_validation(const std::vector<uint64_t> &authorization_accounts);
    void execute_inline_transaction( wasm::inline_transaction_trace &trace,
                                      wasm::inline_transaction &trx,
                                      uint64_t receiver,
                                      CCacheWrapper &database,
                                      vector<CReceipt> &receipts,
                                      //CValidationState &state,
                                      uint32_t recurse_depth);
    void verify_accounts_from_signatures(CCacheWrapper &database, 
                                          std::vector<uint64_t> &authorization_accounts);

};

#endif //TX_WASM_CONTRACT_TX_H