#include "format/pack/installed/installed_pack_key_catalog.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <utility>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <zlib.h>

#include "format/archive/archive_binary.h"
#include "format/pack/installed/plug_file_pack_crypto_constants.h"

namespace {

constexpr std::size_t PackListHeaderByteCount = 10u;
constexpr std::size_t PackListSignatureByteCount = 16u;
constexpr std::size_t PackListEncodedKeyByteCount = 32u;
constexpr std::size_t PackListMaximumNameByteCount = 31u;
constexpr std::uint8_t SupportedPackListVersion = 1u;

bool Md5Bytes(const unsigned char *bytes, std::size_t byteCount,
              unsigned char digest[16]) {
  if ((bytes == nullptr && byteCount != 0u) || digest == nullptr) {
    return false;
  }
  EVP_MD_CTX *context = EVP_MD_CTX_new();
  unsigned int digestSize = 0u;
  if (context == nullptr) {
    return false;
  }
  const bool ok = EVP_DigestInit_ex(context, EVP_md5(), nullptr) == 1 &&
                  EVP_DigestUpdate(context, bytes, byteCount) == 1 &&
                  EVP_DigestFinal_ex(context, digest, &digestSize) == 1 &&
                  digestSize == 16u;
  EVP_MD_CTX_free(context);
  return ok;
}

bool Md5String(const std::string &text, unsigned char digest[16]) {
  return Md5Bytes(reinterpret_cast<const unsigned char *>(text.data()),
                  text.size(), digest);
}

bool IsAsciiPackNameCharacter(unsigned char value) {
  return value >= 0x21u && value <= 0x7eu &&
         value != static_cast<unsigned char>('/') &&
         value != static_cast<unsigned char>('\\');
}

bool IsAsciiHexCharacter(unsigned char value) {
  return (value >= static_cast<unsigned char>('0') &&
          value <= static_cast<unsigned char>('9')) ||
         (value >= static_cast<unsigned char>('A') &&
          value <= static_cast<unsigned char>('F')) ||
         (value >= static_cast<unsigned char>('a') &&
          value <= static_cast<unsigned char>('f'));
}

char AsciiLower(char value) {
  const unsigned char byte = static_cast<unsigned char>(value);
  if (byte >= static_cast<unsigned char>('A') &&
      byte <= static_cast<unsigned char>('Z')) {
    return static_cast<char>(byte - static_cast<unsigned char>('A') +
                             static_cast<unsigned char>('a'));
  }
  return value;
}

bool NormalizeProductName(const char *productName, std::string *out) {
  if (productName == nullptr || out == nullptr) {
    return false;
  }
  const std::size_t byteCount = std::strlen(productName);
  if (byteCount > 23u) {
    return false;
  }
  try {
    out->assign(productName, byteCount);
  } catch (const std::bad_alloc &) {
    return false;
  }
  std::transform(out->begin(), out->end(), out->begin(), AsciiLower);
  return std::all_of(out->begin(), out->end(), [](char value) {
    const unsigned char byte = static_cast<unsigned char>(value);
    return byte >= 0x20u && byte <= 0x7eu;
  });
}

bool NormalizePackName(const char *packName, std::string *out) {
  if (packName == nullptr || out == nullptr) {
    return false;
  }
  std::string normalized;
  try {
    normalized.assign(packName);
  } catch (const std::bad_alloc &) {
    return false;
  }
  if (normalized.size() > PackListMaximumNameByteCount + 4u ||
      normalized.empty() ||
      !std::all_of(normalized.begin(), normalized.end(), [](char value) {
        return IsAsciiPackNameCharacter(static_cast<unsigned char>(value));
      })) {
    return false;
  }
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 AsciiLower);
  if (normalized.size() > 4u &&
      normalized.compare(normalized.size() - 4u, 4u, ".pak") == 0) {
    normalized.resize(normalized.size() - 4u);
  }
  if (normalized.empty() || normalized.size() > PackListMaximumNameByteCount) {
    return false;
  }
  *out = std::move(normalized);
  return true;
}

bool VerifySignature(const unsigned char *bytes, std::size_t byteCount,
                     const std::string &seedText) {
  if (bytes == nullptr ||
      byteCount < PackListHeaderByteCount + PackListSignatureByteCount) {
    return false;
  }
  unsigned char signatureKey[16];
  if (!Md5String(std::string(TmnfPackListSignatureSalt) + seedText,
                 signatureKey)) {
    return false;
  }
  unsigned char computed[EVP_MAX_MD_SIZE];
  unsigned int computedSize = 0u;
  const std::size_t signedByteCount = byteCount - PackListSignatureByteCount;
  if (HMAC(EVP_md5(), signatureKey, static_cast<int>(sizeof(signatureKey)),
           bytes, signedByteCount, computed, &computedSize) == nullptr ||
      computedSize != PackListSignatureByteCount) {
    return false;
  }
  return CRYPTO_memcmp(computed, bytes + signedByteCount,
                       PackListSignatureByteCount) == 0;
}

} // namespace

int InstalledPackKeyCatalog::LoadFromMemory(const void *packlistBytes,
                                            std::size_t packlistByteCount,
                                            const char *productName) {
  Clear();
  try {
    if (packlistBytes == nullptr ||
        packlistByteCount <
            PackListHeaderByteCount + PackListSignatureByteCount) {
      return 0;
    }
    const auto *bytes = static_cast<const unsigned char *>(packlistBytes);
    if (bytes[0] != SupportedPackListVersion) {
      return 0;
    }
    const std::uint32_t entryCount = bytes[1];
    const std::uint32_t productCrc =
        TmnfFormat::ArchiveBinary::ReadU32LE(bytes + 2u);
    const std::uint32_t loadedSeed =
        TmnfFormat::ArchiveBinary::ReadU32LE(bytes + 6u);
    char seedBuffer[32];
    const int seedSize =
        std::snprintf(seedBuffer, sizeof(seedBuffer), "%u", loadedSeed);
    std::string normalizedProduct;
    if (entryCount == 0u || seedSize <= 0 ||
        seedSize >= static_cast<int>(sizeof(seedBuffer)) ||
        !NormalizeProductName(productName, &normalizedProduct)) {
      return 0;
    }
    const std::string seedText(seedBuffer, static_cast<std::size_t>(seedSize));
    if (!VerifySignature(bytes, packlistByteCount, seedText)) {
      return 0;
    }

    const std::uint32_t normalizedProductCrc = static_cast<std::uint32_t>(
        crc32(0u, reinterpret_cast<const Bytef *>(normalizedProduct.data()),
              static_cast<uInt>(normalizedProduct.size())));
    const bool productMatches = productCrc == normalizedProductCrc;

    unsigned char nameDigest[16];
    if (!Md5String(std::string(TmnfPackListNameSalt) + seedText, nameDigest)) {
      return 0;
    }

    std::vector<InstalledPackKeyEntry> loadedEntries;
    try {
      loadedEntries.reserve(entryCount);
    } catch (const std::bad_alloc &) {
      return 0;
    }
    const std::size_t payloadEnd =
        packlistByteCount - PackListSignatureByteCount;
    std::size_t offset = PackListHeaderByteCount;
    for (std::uint32_t entryIndex = 0u; entryIndex < entryCount; ++entryIndex) {
      if (offset > payloadEnd || payloadEnd - offset < 2u) {
        return 0;
      }
      InstalledPackKeyEntry entry;
      entry.flags = bytes[offset];
      const std::size_t nameByteCount = bytes[offset + 1u];
      if (nameByteCount == 0u || nameByteCount > PackListMaximumNameByteCount ||
          payloadEnd - offset <
              2u + nameByteCount + PackListEncodedKeyByteCount) {
        return 0;
      }
      try {
        entry.name.resize(nameByteCount);
        entry.keyString.resize(PackListEncodedKeyByteCount);
      } catch (const std::bad_alloc &) {
        return 0;
      }
      for (std::size_t index = 0u; index < nameByteCount; ++index) {
        const unsigned char decoded = static_cast<unsigned char>(
            bytes[offset + 2u + index] ^ nameDigest[index & 0x0fu]);
        if (!IsAsciiPackNameCharacter(decoded)) {
          return 0;
        }
        entry.name[index] = AsciiLower(static_cast<char>(decoded));
      }

      std::string keySeed = entry.name + seedText;
      if ((entry.flags & 1u) != 0u) {
        if (!productMatches) {
          return 0;
        }
        keySeed += TmnfPackKeySaltOdd;
        keySeed += normalizedProduct;
      } else {
        keySeed += TmnfPackKeySaltEven;
      }
      unsigned char keyDigest[16];
      if (!Md5String(keySeed, keyDigest)) {
        return 0;
      }
      const std::size_t encodedKeyOffset = offset + 2u + nameByteCount;
      for (std::size_t index = 0u; index < PackListEncodedKeyByteCount;
           ++index) {
        const unsigned char decoded = static_cast<unsigned char>(
            bytes[encodedKeyOffset + index] ^ keyDigest[index & 0x0fu]);
        if (!IsAsciiHexCharacter(decoded)) {
          return 0;
        }
        entry.keyString[index] = static_cast<char>(decoded);
      }
      if (std::any_of(loadedEntries.begin(), loadedEntries.end(),
                      [&entry](const InstalledPackKeyEntry &existing) {
                        return existing.name == entry.name;
                      })) {
        return 0;
      }
      try {
        loadedEntries.push_back(std::move(entry));
      } catch (const std::bad_alloc &) {
        return 0;
      }
      offset = encodedKeyOffset + PackListEncodedKeyByteCount;
    }
    if (offset != payloadEnd) {
      return 0;
    }

    seed_ = loadedSeed;
    entries_ = std::move(loadedEntries);
    return 1;
  } catch (const std::bad_alloc &) {
    Clear();
    return 0;
  }
}

void InstalledPackKeyCatalog::Clear() {
  seed_ = 0u;
  entries_.clear();
}

const InstalledPackKeyEntry *
InstalledPackKeyCatalog::Find(const char *packName) const {
  std::string normalized;
  if (!NormalizePackName(packName, &normalized)) {
    return nullptr;
  }
  const auto found =
      std::find_if(entries_.begin(), entries_.end(),
                   [&normalized](const InstalledPackKeyEntry &entry) {
                     return entry.name == normalized;
                   });
  return found != entries_.end() ? &*found : nullptr;
}

std::uint32_t InstalledPackKeyCatalog::Seed() const { return seed_; }

const std::vector<InstalledPackKeyEntry> &
InstalledPackKeyCatalog::Entries() const {
  return entries_;
}
