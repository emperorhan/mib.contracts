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

#include "../../utils/common.h"

namespace misblock {
    using namespace std;
    using namespace eosio;

    struct [[eosio::table("config"), eosio::contract("mib.system")]] config_info {
        /* data */
        pointType total_point = 0;
        asset mis_by_point;
        
        // public_key pubKey;
    };

    struct [[eosio::table, eosio::contract("mib.system")]] hospital_info {
        /* data */
        name owner;
        string url;
        double service_weight = 0;

        uint64_t unpaid_rewards = 0;
        time_point last_claim_time;
        
        uint64_t primary_key() const { return owner.value; }
    };

    struct [[eosio::table, eosio::contract("mib.system")]] review_info {
        uint64_t review_id;
        name owner;
        name hospital_name;
        
        uint32_t likes;
        
        uint64_t primary_key() const { return review_id; }
        uint64_t get_owner() const { return owner.value; }
    };

    struct [[eosio::table, eosio::contract("mib.system")]] customer_info {
        /* data */
        name owner;
        pointType app_point = 0;

        uint64_t primary_key() const { return owner.value; }
    };
    
    typedef eosio::singleton< name("config"), config_info > config_singleton;

    typedef eosio::multi_index< name("hospitals"), hospital_info > hospitals_table;
    typedef eosio::multi_index< name("customers"), customer_info > customers_table;
    typedef eosio::multi_index< name("reviews"), review_info,
                                indexed_by< name("byowner"), const_mem_fun< review_info, uint64_t, &review_info::get_owner> >
                                > reviews_table;

    class [[eosio::contract("mib.system")]] misblock : public eosio::contract {
        private:
            config_singleton _config;

            config_info _cstate;

            config_info get_default_config();

        public:
            misblock( name receiver, name code, datastream<const char*> ds )
                : contract( receiver, code, ds ), _config( receiver, receiver.value ) {
                    _cstate = _config.exists() ? _config.get() : config_info{};
                }
            ~misblock() {
                _config.set( _cstate, get_self() );
            }

            [[eosio::action]]
            void reghospital( const name& owner, const string& url);

            [[eosio::action]]
            void claim( name hospital );
    };


}
