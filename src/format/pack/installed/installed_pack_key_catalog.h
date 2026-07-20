#ifndef TMNF_INSTALLED_PACK_KEY_CATALOG_H
#define TMNF_INSTALLED_PACK_KEY_CATALOG_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct InstalledPackKeyEntry {
  std::uint8_t flags = 0u;
  std::string name;
  std::string keyString;
};

// Parsed packlist.dat state. One catalog can derive keys for every installed
// pack without decoding or authenticating the list again.
class InstalledPackKeyCatalog {
public:
  int LoadFromMemory(const void *packlistBytes, std::size_t packlistByteCount,
                     const char *productName = "");
  void Clear();
  const InstalledPackKeyEntry *Find(const char *packName) const;
  std::uint32_t Seed() const;
  const std::vector<InstalledPackKeyEntry> &Entries() const;

private:
  std::uint32_t seed_ = 0u;
  std::vector<InstalledPackKeyEntry> entries_;
};

#endif
