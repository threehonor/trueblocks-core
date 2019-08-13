/*-------------------------------------------------------------------------
 * This source code is confidential proprietary information which is
 * Copyright (c) 2017 by Great Hill Corporation.
 * All Rights Reserved
 *------------------------------------------------------------------------*/
#include "options.h"

//---------------------------------------------------------------------------------------------------
static const COption params[] = {
// BEG_CODE_OPTIONS
    COption("mode_list", "", "list<enum[scraper|monitors|names|abis|blocks|txs|traces|slurps|prices|some*|all]>", OPT_REQUIRED | OPT_POSITIONAL, "one or more of [scraper|monitors|names|abis|blocks|txs|traces|slurps|prices|some*|all]"),
    COption("details", "d", "", OPT_SWITCH, "include details about items found in monitors, slurps, abis, or price caches"),
    COption("list", "l", "", OPT_SWITCH, "display results in Linux ls -l format (assumes --detail)"),
    COption("fmt", "x", "enum[none|json*|txt|csv|api]", OPT_HIDDEN | OPT_FLAG, "export format (one of [none|json*|txt|csv|api])"),
    COption("", "", "", 0, "Report on status of one or more TrueBlocks caches."),
// END_CODE_OPTIONS
};
static const size_t nParams = sizeof(params) / sizeof(COption);

//---------------------------------------------------------------------------------------------------
bool COptions::parseArguments(string_q& command) {

    ENTER("parseArguments");
    if (!standardOptions(command))
        EXIT_NOMSG(false);

    Init();
    explode(arguments, command, ' ');
    for (auto arg : arguments) {
        if (arg == "-d" || arg == "--details") {
            details = true;
        } else if (arg == "-l" || arg == "--list") {
            ls = true;

        } else {
            string_q options = params[0].description;
            replaceAny(options, "[]*", "|");
            if (!contains(options, "|" + arg + "|"))
                return usage("Provided value for 'mode' (" + arg + ") not " + substitute(params[0].description, "enum", "") + ". Quitting.");
            mode += (arg + "|");
        }
    }

    if (mode.empty() || contains(mode, "some"))
        mode = "scraper|monitors|names|abis|slurps|prices";

    if (contains(mode, "all"))
        mode = "scraper|monitors|names|abis|blocks|txs|traces|slurps|prices";

    mode = "|" + trim(mode, '|') + "|";

    if (!details) {
        HIDE_FIELD(CMonitorCache, "items");
        HIDE_FIELD(CSlurpCache, "items");
        HIDE_FIELD(CPriceCache, "items");
        HIDE_FIELD(CAbiCache, "items");
    }

    EXIT_NOMSG(true);
}

//---------------------------------------------------------------------------------------------------
void COptions::Init(void) {
    registerOptions(nParams, params);
    optionOn(OPT_PREFUND | OPT_OUTPUT);
    optionOff(OPT_VERBOSE);

    ls = false;
    details = false;
    mode = "";
    char hostname[HOST_NAME_MAX];  gethostname(hostname, HOST_NAME_MAX);
    char username[LOGIN_NAME_MAX]; getlogin_r(username, LOGIN_NAME_MAX);

    if (isTestMode()) {
        status.host = "--hostname-- (--username--)";
        status.rpc_provider = status.api_provider = status.balance_provider = "--providers--";
    } else {
        status.host = string_q(hostname) + " (" + username + ")";
        status.rpc_provider = getGlobalConfig()->getConfigStr("settings", "rpcProvider", "http://localhost:8545");
        status.api_provider = getGlobalConfig()->getConfigStr("settings", "apiProvider", "http://localhost:8080");
        status.balance_provider = getGlobalConfig()->getConfigStr("settings", "balanceProvider", status.api_provider);
    }
    status.client_version = getVersionFromClient();
    status.trueblocks_version = getVersionStr();
    status.is_scraping = isRunning("chifra scrape", true);
}

//---------------------------------------------------------------------------------------------------
COptions::COptions(void) {
    Init();

    CStatus::registerClass();
    CCache::registerClass();
    CAddressCache::registerClass();
    CAbiCache::registerClass();
    CChainCache::registerClass();
    CMonitorCache::registerClass();
    CMonitorCacheItem::registerClass();
    CSlurpCache::registerClass();
    CPriceCache::registerClass();
    CPriceCacheItem::registerClass();
    CAbiCacheItem::registerClass();

    HIDE_FIELD(CAccountWatch,  "statement");
    HIDE_FIELD(CAccountWatch,  "stateHistory");
    HIDE_FIELD(CAccountWatch,  "nodeBal");
    HIDE_FIELD(CAccountWatch,  "abi_spec");

    HIDE_FIELD(CAccountName,   "is_contract");
    HIDE_FIELD(CAccountName,   "is_custom");
    HIDE_FIELD(CAccountName,   "is_shared");
    UNHIDE_FIELD(CAccountName, "nRecords");
    UNHIDE_FIELD(CAccountName, "sizeInBytes");
    UNHIDE_FIELD(CAccountName, "firstAppearance");
    UNHIDE_FIELD(CAccountName, "latestAppearance");

    minArgs = 0;
}

//--------------------------------------------------------------------------------
COptions::~COptions(void) {
}

//--------------------------------------------------------------------------------
void COptions::getStatus(ostream& os) {

    if (ls)
        cout << "Listing mode:" << endl;

    CAddressCache aid;
    if (contains(mode, "|scraper|")) {
        aid.type = aid.getRuntimeClass()->m_ClassName;
        aid.path = getIndexPath("");
        forEveryFileInFolder(aid.path, countFiles, &aid);
        status.caches.push_back(&aid);
    }

    CMonitorCache md;
    if (contains(mode, "|monitors|")) {
        md.type = md.getRuntimeClass()->m_ClassName;
        md.path = getCachePath("monitors/");
        forEveryFileInFolder(md.path, countFiles, &md);
        status.caches.push_back(&md);
        if (details) {
            CItemCounter counter(this);
            counter.cachePtr = &md;
            counter.monitorArray = &md.items;
            forEveryFileInFolder(md.path, noteMonitor, &counter);
        }
    }

//names

    CAbiCache abi;
    if (contains(mode, "|abis|")) {
        abi.type = abi.getRuntimeClass()->m_ClassName;
        abi.path = getCachePath("abis/");
        forEveryFileInFolder(abi.path, countFiles, &abi);
        status.caches.push_back(&abi);
        if (details) {
            CItemCounter counter(this);
            counter.cachePtr = &abi;
            counter.abiArray = &abi.items;
            forEveryFileInFolder(abi.path, noteABI, &counter);
        }
    }

    CChainCache cd_blocks;
    if (contains(mode, "|blocks|")) {
        cd_blocks.type = cd_blocks.getRuntimeClass()->m_ClassName;
        cd_blocks.path = getCachePath("blocks/");
        forEveryFileInFolder(cd_blocks.path, countFiles, &cd_blocks);
        status.caches.push_back(&cd_blocks);
    }

    CChainCache cd_txs;
    if (contains(mode, "|txs|")) {
        cd_txs.type = cd_txs.getRuntimeClass()->m_ClassName;
        cd_txs.path = getCachePath("txs/");
        forEveryFileInFolder(cd_txs.path, countFiles, &cd_txs);
        status.caches.push_back(&cd_txs);
    }

    CChainCache cd_traces;
    if (contains(mode, "|traces|")) {
        cd_traces.type = cd_traces.getRuntimeClass()->m_ClassName;
        cd_traces.path = getCachePath("traces/");
        forEveryFileInFolder(cd_traces.path, countFiles, &cd_traces);
        status.caches.push_back(&cd_traces);
    }

    CSlurpCache slurps;
    if (contains(mode, "|slurps|")) {
        slurps.type = slurps.getRuntimeClass()->m_ClassName;
        slurps.path = getCachePath("slurps/");
        forEveryFileInFolder(slurps.path, countFiles, &slurps);
        status.caches.push_back(&slurps);
        if (details) {
            CItemCounter counter(this);
            counter.cachePtr = &slurps;
            counter.monitorArray = &slurps.items;
            forEveryFileInFolder(slurps.path, noteMonitor, &counter);
        }
    }

    CPriceCache pd;
    if (contains(mode, "|prices|")) {
        pd.type = pd.getRuntimeClass()->m_ClassName;
        pd.path = getCachePath("prices/");
        forEveryFileInFolder(pd.path, countFiles, &pd);
        status.caches.push_back(&pd);
        if (details) {
            CItemCounter counter(this);
            counter.cachePtr = &pd;
            counter.priceArray = &pd.items;
            forEveryFileInFolder(pd.path, notePrice, &counter);
        }
    }

    os << exportPreamble(JSON1, expContext().fmtMap["header"], GETRUNTIME_CLASS(CStatus));
    os << status << endl;
    os << exportPostamble(JSON1, expContext().fmtMap["meta"]);

    return;
}

//---------------------------------------------------------------------------
bool countFiles(const string_q& path, void *data) {
    CCache *counter = (CCache *)data;
    if (endsWith(path, '/')) {
        if (!contains(path, "monitors/staging"))
            counter->noteFolder(path);
        return forEveryFileInFolder(path + "*", countFiles, data);

    } else if (endsWith(path, ".bin") || endsWith(path, ".json")) {
        counter->noteFile(path);
        counter->valid_counts = true;

    }
    return true;
}

//---------------------------------------------------------------------------
bool noteMonitor(const string_q& path, void *data) {

    if (contains(path, "/staging"))
        return true;

    if (endsWith(path, '/')) {
        return forEveryFileInFolder(path + "*", noteMonitor, data);

    } else if (endsWith(path, ".bin") || endsWith(path, ".json")) {

        CItemCounter *counter = (CItemCounter*)data;
        ASSERT(counter->options);

        CMonitorCacheItem mdi;
        mdi.type = mdi.getRuntimeClass()->m_ClassName;
        mdi.address = substitute(substitute(substitute(substitute(path, counter->cachePtr->path, ""),".acct", ""),".bin", ""), ".json", "");
        counter->options->getNamedAccount(mdi, mdi.address);
        mdi.firstAppearance = 1001001;
        mdi.latestAppearance = 8101001;
        mdi.nRecords = fileSize(path) / sizeof(CAppearance_base);
        mdi.sizeInBytes = fileSize(path);
        counter->monitorArray->push_back(mdi);

    }
    return true;
}

//---------------------------------------------------------------------------
bool noteABI(const string_q& path, void *data) {

    if (contains(path, "/staging"))
        return true;

    if (endsWith(path, '/')) {
        return forEveryFileInFolder(path + "*", noteABI, data);

    } else if (endsWith(path, ".bin") || endsWith(path, ".json")) {

        CItemCounter *counter = (CItemCounter*)data;
        ASSERT(counter->options);

        CAbiCacheItem abii;
        abii.type = abii.getRuntimeClass()->m_ClassName;
        abii.address = substitute(substitute(substitute(substitute(path, counter->cachePtr->path, ""),".acct", ""),".bin", ""), ".json", "");
        CAccountName n;
        counter->options->getNamedAccount(n, abii.address);
        abii.name = n.name;
        CAbi abi;
        abi.loadAbiFromFile(path, false);
        abii.nFunctions = abi.nFunctions();
        abii.nEvents = abi.nEvents();
        abii.nOther = abi.nOther();
        abii.sizeInBytes = fileSize(path);
        counter->abiArray->push_back(abii);

    }
    return true;
}

//---------------------------------------------------------------------------
bool notePrice(const string_q& path, void *data) {

    if (endsWith(path, '/')) {
        return forEveryFileInFolder(path + "*", notePrice, data);

    } else if (endsWith(path, ".bin")) {

        CItemCounter *counter = (CItemCounter*)data;
        ASSERT(counter->options);

        CPriceCacheItem price;
        price.type = price.getRuntimeClass()->m_ClassName;
        price.pair = substitute(substitute(substitute(substitute(path, counter->cachePtr->path, ""),".acct", ""),".bin", ""), ".json", "");
        price.sizeInBytes = fileSize(path);
        price.nRecords = fileSize(path) / sizeof(CPriceQuote);
        counter->priceArray->push_back(price);
    }
    return true;
}

#if 0
/*-------------------------------------------------------------------------
 * This source code is confidential proprietary information which is
 * Copyright (c) 2017 by Great Hill Corporation.
 * All Rights Reserved
 *------------------------------------------------------------------------*/
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include "options.h"
#include "question.h"

//------------------------------------------------------------------------------------------------
bool COptions::handle_status(void) {

    ENTER4("handle_" + mode);
    nodeNotRequired();

    ostringstream os;
    os << cGreen << "Monitor path: " << cWhite << getMonitorPath("") << endl;

    bool stats = contains(tool_flags, "-l") || contains(tool_flags, "--long");

    CStringArray files;
    if (isTestMode()) {
        files.push_back("0xfb6916095ca1df60bb79ce92ce3ea74c37c5d359.accts.bin");

    } else {
        if (stats && addrs.size()) {
            for (auto addr : addrs) {
                string_q fn = getMonitorPath(addr + ".acct.bin");
                if (fileExists(fn))
                    files.push_back(fn);
                else
                    LOG_WARN(fn, " not found.");
            }
        } else {
            listFilesInFolder(files, getMonitorPath("*.*"), false);
        }
    }

    CAccountNameArray accounts;
    for (auto file : files) {
        if (endsWith(file, ".acct.bin")) {
            replace(file, getMonitorPath(""), "");
            CAccountName item;
            item.address = nextTokenClear(file, '.');
            getNamedAccount(item, item.address);
            item.name = substitute(substitute(item.name, "(", ""), ")", "");
            item.path = substitute(getMonitorPath(item.address), getCachePath(""), "./");
            item.sizeInBytes = fileSize(getMonitorPath(item.address));
            item.latestAppearance = str_2_Uint(asciiFileToString(getMonitorLast(item.address)));
            item.lastExport = str_2_Uint(asciiFileToString(getMonitorExpt(item.address)));
            item.nRecords = fileSize(getMonitorPath(item.address)) / sizeof(CAppearance_base);
            accounts.push_back(item);
        }
    }
    if (accounts.size() == 0) {
        if (api_mode) {
            CAccountName item;
            item.address = "0x0";
            item.name = "none";
            accounts.push_back(item);
        } else {
            LOG_WARN("No monitors found.");
            EXIT_NOMSG4(true);
        }
    }
    sort(accounts.begin(), accounts.end());

    if (api_mode) {
        SHOW_FIELD(CAccountName, "path");
        SHOW_FIELD(CAccountName, "sizeInBytes");
        SHOW_FIELD(CAccountName, "latestAppearance");
        SHOW_FIELD(CAccountName, "lastExport");
        SHOW_FIELD(CAccountName, "nRecords");
        ostringstream oss;
        if (accounts.size() > 1)
            oss << "[";
        bool first = true;
        for (auto acct : accounts) {
            if (!first)
                oss << ",";
            oss << acct.Format();
            first = false;
        }
        if (accounts.size() > 1)
            oss << "]";
        cout << substitute(substitute(oss.str(), "\n", ""), "\t", "") << endl;
        EXIT_NOMSG4(true);
    }

    if (stats) {
        for (auto acct : accounts) {
            string_q fmt =
                "[Address:  -c1-{ADDRESS}-off-\n]"
                "[\tName:        -c2-{NAME}-off-\n]"
                "[\tFile name:   -c2-{PATH}-off-\n]"
                "[\tFile size:   -c2-{SIZEINBYTES}-off-\n]"
                "[\tLast block:  -c2-{LATESTAPPEARANCE}-off-\n]"
                "[\tLast export: -c2-{LASTEXPORT}-off-\n]"
                "[\tnRecords:    -c2-{NRECORDS}-off-\n]";
            replaceAll(fmt, "-c1-", cTeal);
            replaceAll(fmt, "-c2-", cYellow);
            replaceAll(fmt, "-off-", cOff);
            os << acct.Format(fmt);
        }
    } else {
#ifdef TIOCGSIZE
        struct ttysize ts;
        ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
        size_t ncols = ts.ts_cols;
#elif defined(TIOCGWINSZ)
        struct winsize ts;
        ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
        size_t ncols = ts.ws_col;
#else
        size_t ncols = 1;
#endif /* TIOCGSIZE */
        ncols = max(ncols, (size_t)1);

        os << cGreen << "Current monitors:" << cOff << endl;

        // find the longest name (max 25 chars)
        size_t longest = 0;
        for (auto acct : accounts)
            longest = max(longest, acct.name.length() + 3); // two parens
        longest = min(longest, (size_t)23); // max 23
        ncols = max(((size_t)1), (ncols / (size_t(42) + longest)));

        uint64_t cnt = 0;
        for (auto acct : accounts) {
            string_q name = acct.name;
            if (!name.empty())
                name = "(" + name.substr(0,20) + ") ";
            os << " " << cTeal << acct.address << " " << padRight(name, longest);
            if (!(++cnt % ncols))
                os << endl;
        }
        os << cOff << endl;
    }

    if (isTestMode())
        cout << substitute(os.str(), getCachePath(""), "$BLOCK_CACHE/");
    else
        cout << os.str();

    EXIT_NOMSG4(true);
}
#endif

