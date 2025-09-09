// --- ORDER MATTERS ---
#include "SimpleMapResolver.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/pxr.h"

#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/writableAsset.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"

#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(SimpleMapResolver);

// ---------- statics ----------
std::vector<SimpleMapResolver::Map> SimpleMapResolver::_maps;
std::atomic<long long> SimpleMapResolver::_cfgMtime{-1};
std::string SimpleMapResolver::_cfgPath;
bool SimpleMapResolver::_normalizeBackslashes = true;
bool SimpleMapResolver::_caseInsensitiveDrive = true;
std::mutex SimpleMapResolver::_cfgMutex;

// ---------- local helpers ----------
static long long _GetMTime(const std::string& p) {
    struct stat st; if (stat(p.c_str(), &st)==0) return (long long)st.st_mtime; return -1;
}
static std::string _GetEnv(const char* k) {
    const char* v = getenv(k); return v ? std::string(v) : std::string();
}

// ---------- ctor ----------
SimpleMapResolver::SimpleMapResolver() : ArResolver() {
    _cfgPath = _GetEnv("SIMPLE_RESOLVER_CONFIG");
    if (_cfgPath.empty()) _cfgPath = "/etc/usd/simple_resolver.json";
    _LoadConfigIfStale();
    TF_STATUS("SimpleMapResolver active. Config: %s", _cfgPath.c_str());
}

// ---------- config + parsing ----------
void SimpleMapResolver::_ParseConfig(const std::string& json) {
    _maps.clear();
    _normalizeBackslashes = true;
    _caseInsensitiveDrive = true;

    auto trim = [](std::string s){
        const char* ws=" \t\r\n";
        size_t a = s.find_first_not_of(ws);
        size_t b = s.find_last_not_of(ws);
        return (a==std::string::npos) ? std::string() : s.substr(a, b-a+1);
    };

    std::istringstream ss(json);
    std::string line, from, to;
    while (std::getline(ss, line)) {
        line = trim(line);
        if (line.find("\"normalizeBackslashes\"") != std::string::npos &&
            line.find("false") != std::string::npos) _normalizeBackslashes = false;
        if (line.find("\"caseInsensitiveDrive\"") != std::string::npos &&
            line.find("false") != std::string::npos) _caseInsensitiveDrive = false;

        if (line.find("\"from\"") != std::string::npos) {
            auto f = line.substr(line.find(':')+1);
            f.erase(std::remove(f.begin(), f.end(), '\"'), f.end());
            f.erase(std::remove(f.begin(), f.end(), ','),  f.end());
            from = trim(f);
        }
        if (line.find("\"to\"") != std::string::npos) {
            auto t = line.substr(line.find(':')+1);
            t.erase(std::remove(t.begin(), t.end(), '\"'), t.end());
            t.erase(std::remove(t.begin(), t.end(), ','),  t.end());
            to = trim(t);
        }
        if (!from.empty() && !to.empty()) {
            _maps.push_back({from, to});
            from.clear(); to.clear();
        }
    }
}

void SimpleMapResolver::_LoadConfigIfStale() {
    std::lock_guard<std::mutex> lk(_cfgMutex);
    long long mt = _GetMTime(_cfgPath);
    if (mt == _cfgMtime) return;

    std::ifstream ifs(_cfgPath);
    if (!ifs) { TF_WARN("SimpleMapResolver: config not found: %s", _cfgPath.c_str()); _maps.clear(); _cfgMtime = mt; return; }
    std::stringstream buf; buf << ifs.rdbuf();
    _ParseConfig(buf.str());
    _cfgMtime = mt;
}

// ---------- mapping helpers ----------
std::string SimpleMapResolver::_Normalize(const std::string& in) {
    std::string s = in;
    if (_normalizeBackslashes) {
        for (auto& c : s) if (c == '\\') c = '/';
    }
    if (_caseInsensitiveDrive &&
        s.size() >= 3 &&
        std::isalpha((unsigned char)s[0]) && s[1]==':' && (s[2]=='/'||s[2]=='\\')) {
        s[0] = std::toupper((unsigned char)s[0]);
    }
    return s;
}

std::string SimpleMapResolver::_MapIfPrefixed(const std::string& in) {
    std::string s = _Normalize(in);
    for (const auto& m : _maps) {
        if (s.rfind(m.from, 0) == 0) { // starts_with
            return m.to + s.substr(m.from.size());
        }
    }
    return s;
}

// ---------- required ArResolver methods ----------
std::string SimpleMapResolver::_CreateIdentifier(const std::string& assetPath,
                                                 const ArResolvedPath& anchor) const {
    _LoadConfigIfStale();
    const std::string mapped = _MapIfPrefixed(assetPath);
    return ArDefaultResolver().CreateIdentifier(mapped, anchor);
}

ArResolvedPath SimpleMapResolver::_Resolve(const std::string& assetPath) const {
    _LoadConfigIfStale();
    const std::string mapped = _MapIfPrefixed(assetPath);
    return ArDefaultResolver().Resolve(mapped);
}

// USD 25.05 new virtuals:
std::string SimpleMapResolver::_CreateIdentifierForNewAsset(
        const std::string& assetPath, const ArResolvedPath& anchor) const {
    _LoadConfigIfStale();
    const std::string mapped = _MapIfPrefixed(assetPath);
    return ArDefaultResolver().CreateIdentifierForNewAsset(mapped, anchor);
}

ArResolvedPath SimpleMapResolver::_ResolveForNewAsset(const std::string& assetPath) const {
    _LoadConfigIfStale();
    const std::string mapped = _MapIfPrefixed(assetPath);
    return ArDefaultResolver().ResolveForNewAsset(mapped);
}

std::shared_ptr<ArAsset> SimpleMapResolver::_OpenAsset(const ArResolvedPath& resolvedPath) const {
    _LoadConfigIfStale();
    const std::string mapped = _MapIfPrefixed(resolvedPath.GetPathString());
    return ArDefaultResolver().OpenAsset(ArResolvedPath(mapped));
}

std::shared_ptr<ArWritableAsset> SimpleMapResolver::_OpenAssetForWrite(
        const ArResolvedPath& resolvedPath, WriteMode mode) const {
    _LoadConfigIfStale();
    const std::string mapped = _MapIfPrefixed(resolvedPath.GetPathString());
    return ArDefaultResolver().OpenAssetForWrite(ArResolvedPath(mapped), mode);
}

PXR_NAMESPACE_CLOSE_SCOPE
