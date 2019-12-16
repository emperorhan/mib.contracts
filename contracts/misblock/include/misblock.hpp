/*
 * Copyright 2019 IBCT
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

#include <vector>

#include "../../utils/common.h"

namespace misblock {
    using namespace std;
    using namespace eosio;

    struct [[eosio::table("config"), eosio::contract("mib.system")]] ConfigInfo {
        pointType totalPointSupply = 0;
        asset misByPoint;
    };

    struct [[eosio::table, eosio::contract("mib.system")]] HospitalInfo {
        // scope: code 
        name owner;
        string url;

        double serviceWeight = 0;

        // 초기화 필요 없음
        uint64_t customerCount = 0;
        uint64_t reviewCount = 0;

        // 초기화 필요함
        uint64_t emrCount = 0;
        uint64_t reviewVisitors = 0;

        uint64_t unpaidRewards = 0;
        time_point last_claim_time;
        
        uint64_t primary_key() const { return owner.value; }
    };

    struct [[eosio::table, eosio::contract("mib.system")]] CustomerInfo {
        // scope: code
        name owner;

        pointType point;

        vector<name> hospitals;
        uint64_t paidCount;

        uint64_t primary_key() const { return owner.value; }
    };

    struct [[eosio::table, eosio::contract("mib.system")]] ReviewInfo {
        // scope: hospital
        uuidType id;
        name owner;

        uint64_t likes; // 컨트랙트 내에서 처리해야 할까? 일별로 3개의 좋아요를 할 수 있는 제한을 구현하기가 애매하다. 시간으로?

        // 안아파톡에서 후기 게시글 고유 번호
        uuidType reviewId;
        
        uint64_t primary_key() const { return id; }
        uint64_t getOwner() const { return owner.value; }
    };
    
    typedef eosio::singleton< name("config"), ConfigInfo > configSingleton;

    typedef eosio::multi_index< name("hospitals"), HospitalInfo > hospitalsTable;
    typedef eosio::multi_index< name("customers"), CustomerInfo > customersTable;
    typedef eosio::multi_index< name("reviews"), ReviewInfo,
                                indexed_by< name("byowner"), const_mem_fun< ReviewInfo, uint64_t, &ReviewInfo::getOwner> >
                                > reviewsTable;

    class [[eosio::contract("mib.system")]] misblock : public eosio::contract {
        private:
            configSingleton _config;

            ConfigInfo _cstate;

            ConfigInfo getDefaultConfig();

        public:
            misblock( name receiver, name code, datastream<const char*> ds )
                : contract( receiver, code, ds ), _config( receiver, receiver.value ) {
                    _cstate = _config.exists() ? _config.get() : ConfigInfo{};
                }
            ~misblock() {
                _config.set( _cstate, get_self() );
            }

            [[eosio::action]]
            void reghospital( const name& owner, const string& url );

            [[eosio::action]]
            void claim( const name& hospital );

            [[eosio::action]]
            void exchangemis( const name& owner, const pointType& point );
    };
}
