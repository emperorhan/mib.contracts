#pragma ones

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/crypto.hpp>

#include "types.h"

using namespace types;
using namespace eosio;

using std::string;
using std::vector;

namespace common {

    static constexpr uint32_t seconds_per_year      = 52 * 7 * 24 * 3600;
    static constexpr uint32_t seconds_per_week      = 24 * 3600 * 7;
    static constexpr uint32_t seconds_per_day       = 24 * 3600;
    static constexpr int64_t  useconds_per_year     = int64_t(seconds_per_year) * 1000'000ll;
    static constexpr int64_t  useconds_per_week     = int64_t(seconds_per_week) * 1000'000ll;
    static constexpr int64_t  useconds_per_day      = int64_t(seconds_per_day) * 1000'000ll;
    static constexpr uint32_t blocks_per_year       = 2 * seconds_per_year;  // half seconds per year
    static constexpr uint32_t blocks_per_week       = 2 * seconds_per_week;  // half seconds per week
    static constexpr uint32_t blocks_per_day        = 2 * seconds_per_day;   // half seconds per day

    static const symbol         S_MIS("MIS", 4);
    static const symbol         S_LED("LED", 4);

    // static const checksum256    MIS_HASH = sha256("mis", 3);
    // static const checksum256    NO_HASH = sha256("", 0);

    // static void prove( const signature& sig, const public_key& key ) {
    //     assert_recover_key(RIDL_HASH, sig, key);
    // }

    inline static uuidType toUUID(string username){
        return std::hash<string>{}(username);
    }

    template <typename T>
    void cleanTable(name self, uint64_t scope = 0){
        uint64_t s = scope ? scope : self.value;
        T db(self, s);
        while(db.begin() != db.end()){
            auto itr = --db.end();
            db.erase(itr);
        }
    }

    void transferMIS(name& from, name to, asset quantity, string memo){
        check(quantity.symbol == S_MIS, "This is not a MIS token");
        action( permission_level{ name("mib.c"), name("active") },
                name("led.token"),
                name("transfer"),
                make_tuple(from, to, quantity, memo)
        ).send();
    }

    double weight = int64_t( (current_time_point().sec_since_epoch() - (block_timestamp::block_timestamp_epoch / 1000)) / (seconds_per_day * 7) )  / double( 52 );
};