#pragma once
#include "pxr/pxr.h"
#include "pxr/usd/ar/resolver.h"
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class SimpleMapResolver : public ArResolver {
public:
    SimpleMapResolver();
    ~SimpleMapResolver() override = default;

protected:
    // Remap hooks used for reading:
    std::string _CreateIdentifier(const std::string& assetPath,
                                  const ArResolvedPath& anchorAssetPath) const override;
    ArResolvedPath _Resolve(const std::string& assetPath) const override;

    // USD 25.05 required virtuals:
    std::string _CreateIdentifierForNewAsset(const std::string& assetPath,
                                             const ArResolvedPath& anchorAssetPath) const override;
    ArResolvedPath _ResolveForNewAsset(const std::string& assetPath) const override;
    std::shared_ptr<class ArAsset> _OpenAsset(const ArResolvedPath& resolvedPath) const override;
    std::shared_ptr<class ArWritableAsset> _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath, WriteMode writeMode) const override;

    // No custom context/caching:
    ArResolverContext _CreateDefaultContext() const override { return ArResolverContext(); }
    void _RefreshContext(const ArResolverContext&) override {}
    void _BeginCacheScope(VtValue*) override {}
    void _EndCacheScope(VtValue*) override {}

private:
    struct Map { std::string from, to; };

    static void _LoadConfigIfStale();
    static void _ParseConfig(const std::string& json);
    static std::string _Normalize(const std::string& in);
    static std::string _MapIfPrefixed(const std::string& in);

    static std::vector<Map> _maps;
    static std::atomic<long long> _cfgMtime;
    static std::string _cfgPath;
    static bool _normalizeBackslashes;
    static bool _caseInsensitiveDrive;
    static std::mutex _cfgMutex;
};

PXR_NAMESPACE_CLOSE_SCOPE
