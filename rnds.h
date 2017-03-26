#pragma once

#include "array.h"
#include "common.h"
#include "random.h"

#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace jngen {

typedef std::pair<long long, long long> HashBase; // (mod, base)
typedef std::pair<std::string, std::string> StringPair;

class StringRandom {
public:
    StringRandom() {
        static bool created = false;
        ENSURE(!created, "jngen::StringRandom should be created only once");
        created = true;
    }

    static std::string thueMorse(int len, char first = 'a', char second = 'b');

    static std::string abacaba(int len, char first = 'a');

    static StringPair antiHash(
            const std::vector<HashBase>& bases,
            const std::string& alphabet = "a-z",
            int length = -1);
};

JNGEN_EXTERN StringRandom rnds;

namespace detail {

inline int popcount(long long x) {
    int res = 0;
    while (x) {
        ++res;
        x &= x-1;
    }
    return res;
}

inline int trailingZeroes(long long x) {
    int res = 0;
    ENSURE(x != 0);
    while (!(x&1)) {
        ++res;
        x >>= 1;
    }
    return res;
}

inline std::string parseAllowedChars(std::string pattern) {
    std::set<char> result;
    pattern += "\0\0";
    for (size_t i = 0; i < pattern.length(); ++i) {
        if (pattern[i] == '-') {
            result.insert('-');
        } else if(pattern[i+1] == '-' && pattern[i+2] != '\0') {
            for (char c = pattern[i]; c <= pattern[i+2]; ++c) {
                result.insert(c);
            }
            i += 2;
        } else {
            result.insert(pattern[i]);
        }
    }
    return std::string(result.begin(), result.end());
}

inline std::vector<std::string> extendAntiHash(
        const std::vector<std::string>& chars,
        HashBase base,
        int count)
{
    ENSURE(count == 2, "Count != 2 is not supported (yet)");

    size_t baseLength = chars[0].size();
    for (const auto& s: chars) {
        ensure(s.size() == baseLength);
    }

    long long mod = base.first;
    long long p = base.second;

    long long pPower = 1;
    for (size_t i = 0; i != baseLength; ++i) {
        pPower = (pPower * p) % mod;
    }

    std::vector<long long> charHashes;
    for (const auto& s: chars) {
        long long hash = 0;
        for (char c: s) {
            hash = (hash * p + c) % mod;
        }
        charHashes.push_back(hash);
    }

    auto computeHash = [&charHashes, mod, pPower](const std::vector<int>& a) {
        long long hash = 0;
        for (int x: a) {
            hash = (hash * pPower + charHashes[x]) % mod;
        }
        return hash;
    };

    // This bounds were achieved empirically and should be justified.
    int needForMatch;
    if (count == 2) {
        needForMatch = 5 * pow(double(mod), 0.5);
    } else {
        ENSURE(false, "Only count = 2 is supported yet");
    }

    int length = 2;
    double wordCount = pow(double(chars.size()), double(length));

    while (true) {
        ++length;
        wordCount *= chars.size();
        if (wordCount < needForMatch) {
            continue;
        }

        std::vector<std::pair<long long, Array>> words;
        std::map<long long, int> hashCount;
        std::set<Array> used;

        for (int i = 0; i < needForMatch; ++i) {
            Array w = Array::random(length, chars.size());
            if (used.count(w)) {
                --i;
                continue;
            }
            used.insert(w);
            long long hash = computeHash(w);
            words.emplace_back(hash, w);
            if (++hashCount[hash] == count) {
                std::vector<std::string> result;
                for (const auto& kv: words) {
                    if (kv.first == hash) {
                        std::string word;
                        for (int c: kv.second) {
                            word += chars[c];
                        }
                        result.push_back(word);
                    }
                }
                return result;
            }
        }
    }
}

inline StringPair minimalAntiHashTest(
        std::vector<HashBase> bases,
        const std::string allowedChars)
{
    for (auto base: bases) {
        ensure(base.first >= 0, "0 < MOD must hold");
        ensure(
            base.first <= (long long)(2e9),
            "Modules larger than 2'000'000'000 are not supported yet");
        ensure(
            0 < base.second && base.second < base.first,
            "0 <= P < MOD must hold");
    }

    std::vector<int> counts;
    if (bases.size() == 1) {
        counts = {2};
    } else if (bases.size() == 2) {
        counts = {2, 2};
    } else {
        counts.assign(bases.size(), 2);
    }

    std::vector<std::string> cur;
    for (char c: allowedChars) {
        cur.emplace_back(1, c);
    }

    for (size_t i = 0; i != bases.size(); ++i) {
        cur = extendAntiHash(cur, bases[i], counts[i]);
        ensure(static_cast<int>(cur.size()) == counts[i],
            "Cannot generate long enough pair with same hash");
    }

    return {cur[0], cur[1]};
}

} // namespace detail

#ifndef JNGEN_DECLARE_ONLY

std::string StringRandom::thueMorse(int len, char first, char second) {
    ensure(len >= 0);
    std::string res(len, ' ');
    for (int i = 0; i < len; ++i) {
        res[i] = detail::popcount(i)%2 == 0 ? first : second;
    }
    return res;
}

std::string StringRandom::abacaba(int len, char first) {
    ensure(len >= 0);
    std::string res(len, ' ');
    for (int i = 0; i < len; ++i) {
        res[i] = first + detail::trailingZeroes(~i);
    }
    return res;
}

StringPair StringRandom::antiHash(
        const std::vector<HashBase>& bases,
        const std::string& alphabet,
        int length)
{
    std::string allowedChars = detail::parseAllowedChars(alphabet);
    StringPair result = detail::minimalAntiHashTest(bases, allowedChars);

    if (length == -1) {
        return result;
    }

    ensure(
        static_cast<int>(result.first.length()) <= length,
        "Cannot generate enough long anti-hash test");

    int extraLength = length - result.first.length();
    int leftSize = rnd.next(0, extraLength);

    std::string left = rnd.next(format("[%s]{%d}", alphabet.c_str(), leftSize));
    std::string right =
        rnd.next(format("[%s]{%d}", alphabet.c_str(), extraLength - leftSize));

    return {
        left + result.first + right,
        left + result.second + right
    };
}

#endif

} // namespace jngen

using jngen::rnds;
