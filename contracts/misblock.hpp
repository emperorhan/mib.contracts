#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

namespace misblock {
    using namespace std;
    using namespace eosio;

    typedef uint64_t pointType;

    class [[eosio::contract("mis.system")]] misblock : public eosio::contract {
        private:
            pointType totalAppPoint = 0;
            asset misByPoint = 0;

            struct [[eosio::table("info")]] state {
                /* data */
                asset supply = asset{0, symbol(symbol_code("TEMP"), 0)};
                public_key pubKey;
            };
            
            typedef eosio::singleton< "info"_n, state > infoSingleton;

            infoSingleton _info;

            state _info_state;

            struct [[eosio::table]] hospitalInfo {
                /* data */
                name owner;
                double serviceWeight = 0;
                
                uint64_t primary_key() const { return owner.value; }
            };

            struct [[eosio::table]] reviewInfo {
                uint64_t reviewId;
                name owner;
                name hospitalName;
                
                uint32_t likes;
                
                uint64_t primary_key() const { return reviewId; }
                uint64_t getOwner() const { return owner.value; }
            }

            struct [[eosio::table]] customerInfo {
                /* data */
                name owner;
                uint64_t appPoint = 0;

                uint64_t primary_key() const { return owner.value; }
            };

            typedef eosio::multi_index< "hospitals"_n, hospitalInfo > hospitalsTable;
            typedef eosio::multi_index< "customers"_n, customerInfo > customersTable;
            typedef eosio::multi_index< "reviews"_n, reviewInfo,
                                        indexed_by< "byowner"_n, const_mem_fun< reviewInfo, uint64_t, &reviewInfo::getOwner> >
                                        > reviewsTable;

        public:
            misblock( name self, name first_receiver, datastream<const char*> ds )
                : contract( self, first_receiver, ds ), _info( self, self.value ) {
                    _info_state = _info.exists() ? _info.get() : state{};
                }
            ~misblock() {
                _info.set( _info_state, get_self() );
            }
    };


}
