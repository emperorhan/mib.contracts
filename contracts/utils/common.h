#pragma ones

#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>

#include "types.h"

#include <libc/bits/stdint.h>
#include <math.h>
#include <string>

using namespace types;
using namespace eosio;
using namespace std;

using std::string;
using std::vector;

namespace common {

static constexpr uint32_t secondsPerYear = 52 * 7 * 24 * 3600;
static constexpr uint32_t secondsPerMonth = 31449600;  // secondsPerYear / 12
static constexpr uint32_t secondsPerWeek = 24 * 3600 * 7;
static constexpr uint32_t secondsPerDay = 24 * 3600;
static constexpr uint32_t secondsPerHour = 3600;
static constexpr int64_t usecondsPerYear = int64_t(secondsPerYear) * 1000'000ll;
static constexpr int64_t usecondsPerMonth = int64_t(secondsPerMonth) * 1000'000ll;
static constexpr int64_t usecondsPerWeek = int64_t(secondsPerWeek) * 1000'000ll;
static constexpr int64_t usecondsPerDay = int64_t(secondsPerDay) * 1000'000ll;
static constexpr int64_t usecondsPerHour = int64_t(secondsPerHour) * 1000'000ll;
static constexpr uint32_t blocksPerYear = 2 * secondsPerYear;    // half seconds per year
static constexpr uint32_t blocksPerMonth = 2 * secondsPerMonth;  // half seconds per month
static constexpr uint32_t blocksPerWeek = 2 * secondsPerWeek;    // half seconds per week
static constexpr uint32_t blocksPerDay = 2 * secondsPerDay;      // half seconds per day
static constexpr uint64_t nullID = UINT64_MAX;

static const symbol S_MIS("MIS", 4);
static const symbol S_LED("LED", 4);

static const asset reward(10000000000, S_MIS);

static const char* charmap = "0123456789";

static constexpr uint32_t constHash(const char* p) {
    return *p ? static_cast<uint32_t>(*p) + 33 * constHash(p + 1) : 5381;
}

// static const checksum256    MIS_HASH = sha256("mis", 3);
// static const checksum256    NO_HASH = sha256("", 0);

// static void prove( const signature& sig, const public_key& key ) {
//     assert_recover_key(RIDL_HASH, sig, key);
// }

inline static uuidType
toUUID(string data) {
    return std::hash<string>{}(data);
}

template <typename T>
void cleanTable(name self, uint64_t scope = 0) {
    uint64_t s = scope ? scope : self.value;
    T db(self, s);
    while (db.begin() != db.end()) {
        auto itr = --db.end();
        db.erase(itr);
    }
}

void transferToken(const name& from, const name& to, const asset& quantity, const string& memo) {
    action(permission_level{from, name("active")},
           name("led.token"),
           name("transfer"),
           make_tuple(from, to, quantity, memo))
        .send();
}

// TODO: Json 형식에 맞는지 더 정밀하게 검사할 필요가 있음
void validateJson(const string& payload) {
    if (payload.size() <= 0)
        return;

    check(payload[0] == '{', "must be a JSON object");
    check(payload.size() < 32768, "should be shorter than 32768 bytes");
}

string uint64_to_string(const uint64_t& value) {
    std::string result;
    result.reserve(20);  // uint128_t has 40
    uint128_t helper = value;

    do {
        result += charmap[helper % 10];
        helper /= 10;
    } while (helper);
    std::reverse(result.begin(), result.end());
    return result;
}

template <typename CharT>
static std::string to_hex(const CharT* d, uint32_t s) {
    std::string r;
    const char* to_hex = "0123456789abcdef";
    uint8_t* c = (uint8_t*)d;
    for (uint32_t i = 0; i < s; ++i) {
        (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
    }
    return r;
}

std::string hex_to_string(const std::string& input) {
    static const char* const lut = "0123456789abcdef";
    size_t len = input.length();
    if (len & 1)
        abort();
    std::string output;
    output.reserve(len / 2);
    for (size_t i = 0; i < len; i += 2) {
        char a = input[i];
        const char* p = std::lower_bound(lut, lut + 16, a);
        if (*p != a)
            abort();
        char b = input[i + 1];
        const char* q = std::lower_bound(lut, lut + 16, b);
        if (*q != b)
            abort();
        output.push_back(((p - lut) << 4) | (q - lut));
    }
    return output;
}

const char* const ALPHABET =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const char ALPHABET_MAP[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1,
    -1, 9, 10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1,
    -1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46,
    47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1};

int base58encode(const std::string input, int len, unsigned char result[]) {
    unsigned char const* bytes = (unsigned const char*)(input.c_str());
    unsigned char digits[len * 137 / 100];
    int digitslen = 1;
    for (int i = 0; i < len; i++) {
        unsigned int carry = (unsigned int)bytes[i];
        for (int j = 0; j < digitslen; j++) {
            carry += (unsigned int)(digits[j]) << 8;
            digits[j] = (unsigned char)(carry % 58);
            carry /= 58;
        }
        while (carry > 0) {
            digits[digitslen++] = (unsigned char)(carry % 58);
            carry /= 58;
        }
    }
    int resultlen = 0;
    // leading zero bytes
    for (; resultlen < len && bytes[resultlen] == 0;)
        result[resultlen++] = '1';
    // reverse
    for (int i = 0; i < digitslen; i++)
        result[resultlen + i] = ALPHABET[digits[digitslen - 1 - i]];
    result[digitslen + resultlen] = 0;
    return digitslen + resultlen;
}

// 한국시간은 협정 세계시 +9:00
time_point currentTimePoint() {
    return current_time_point() += microseconds( usecondsPerHour * 9 );
}
};  // namespace common