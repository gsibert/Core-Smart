// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef SMARTNODE_SYNC_H
#define SMARTNODE_SYNC_H

#include "../chain.h"
#include "../net.h"

#include <univalue.h>

static const bool DEFAULT_CACHE_NODES = false;
static const bool DEFAULT_CACHE_WINNERS= true;
static const bool DEFAULT_CACHE_NETFULLFILLED = true;
static const bool DEFAULT_CACHE_VOTING = true;

class CSmartnodeSync;

static const int SMARTNODE_SYNC_FAILED          = -1;
static const int SMARTNODE_SYNC_INITIAL         = 0; // sync just started, was reset recently or still in IDB
static const int SMARTNODE_SYNC_WAITING         = 1; // waiting after initial to see if we can get more headers/blocks
static const int SMARTNODE_SYNC_LIST            = 2;
static const int SMARTNODE_SYNC_MNW             = 3;
static const int SMARTNODE_SYNC_VOTING          = 4;
static const int SMARTNODE_SYNC_PROPOSAL        = 20;
static const int SMARTNODE_SYNC_PROPOSAL_VOTE   = 21;
static const int SMARTNODE_SYNC_FINISHED        = 999;

static const int SMARTNODE_SYNC_TICK_SECONDS    = 1;
static const int SMARTNODE_SYNC_REQUEST_VOTES_SECONDS = 10;
static const int SMARTNODE_SYNC_TIMEOUT_SECONDS = 15; // our blocks are 2.5 minutes so 30 seconds should be fine

static const int SMARTNODE_SYNC_ENOUGH_PEERS    = 3;

extern CSmartnodeSync smartnodeSync;
extern CCriticalSection cs_unknownpings;
extern std::map<COutPoint,int> mapTryUnknownPings;

//
// CSmartnodeSync : Sync smartnode assets in stages
//

class CSmartnodeSync
{
private:
    // Keep track of current asset
    int nRequestedSmartnodeAssets;
    // Count peers we've requested the asset from
    int nRequestedSmartnodeAttempt;

    // Time when current smartnode asset sync started
    int64_t nTimeAssetSyncStarted;
    // ... last bumped
    int64_t nTimeLastBumped;
    // ... or failed
    int64_t nTimeLastFailure;

    void Fail(CConnman& connman);
    void ClearFulfilledRequests(CConnman& connman);

public:
    CSmartnodeSync() { Reset(); }

    bool IsFailed() { return nRequestedSmartnodeAssets == SMARTNODE_SYNC_FAILED; }
    bool IsBlockchainSynced() { return nRequestedSmartnodeAssets > SMARTNODE_SYNC_INITIAL; }
    bool IsSmartNodeSyncStarted() { return nRequestedSmartnodeAssets > SMARTNODE_SYNC_WAITING; }
    bool IsSmartnodeListSynced() { return nRequestedSmartnodeAssets > SMARTNODE_SYNC_LIST; }
    bool IsWinnersListSynced() { return nRequestedSmartnodeAssets > SMARTNODE_SYNC_MNW; }
    bool IsProposalDataSynced() { return nRequestedSmartnodeAssets > SMARTNODE_SYNC_PROPOSAL_VOTE; }
    bool IsSynced() { return nRequestedSmartnodeAssets == SMARTNODE_SYNC_FINISHED; }

    int GetAssetID() { return nRequestedSmartnodeAssets; }
    int GetAttempt() { return nRequestedSmartnodeAttempt; }
    void BumpAssetLastTime(std::string strFuncName);
    int64_t GetAssetStartTime() { return nTimeAssetSyncStarted; }
    std::string GetAssetName();
    std::string GetSyncStatus();

    void Reset();
    void SwitchToNextAsset(CConnman& connman);

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
    void ProcessTick(CConnman& connman);

    void AcceptedBlockHeader(const CBlockIndex *pindexNew);
    void NotifyHeaderTip(const CBlockIndex *pindexNew, bool fInitialDownload, CConnman& connman);
    void UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload, CConnman& connman);
};

#endif
