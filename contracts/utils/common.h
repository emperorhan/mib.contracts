#pragma ones

#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>

#include "types.h"

#include <libc/bits/stdint.h>
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
static constexpr int64_t usecondsPerYear = int64_t(secondsPerYear) * 1000'000ll;
static constexpr int64_t usecondsPerMonth = int64_t(secondsPerMonth) * 1000'000ll;
static constexpr int64_t usecondsPerWeek = int64_t(secondsPerWeek) * 1000'000ll;
static constexpr int64_t usecondsPerDay = int64_t(secondsPerDay) * 1000'000ll;
static constexpr uint32_t blocksPerYear = 2 * secondsPerYear;    // half seconds per year
static constexpr uint32_t blocksPerMonth = 2 * secondsPerMonth;  // half seconds per month
static constexpr uint32_t blocksPerWeek = 2 * secondsPerWeek;    // half seconds per week
static constexpr uint32_t blocksPerDay = 2 * secondsPerDay;      // half seconds per day
static constexpr uint64_t nullID = UINT64_MAX;

static const symbol S_MIS("MIS", 4);
static const symbol S_LED("LED", 4);

static const asset rewards(10000000000, S_MIS);

static const char* charmap = "0123456789";

static constexpr uint32_t constHash( const char* p ) {
    return *p ? static_cast<uint32_t>(*p) + 33 * constHash( p + 1 ) : 5381;
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
};  // namespace common