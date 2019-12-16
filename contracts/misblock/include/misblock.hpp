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

using namespace types;
using namespace common;

namespace misblock {
    using namespace std;
    using namespace eosio;

    struct [[eosio::table("config"), eosio::contract("mib.system")]] ConfigInfo {
        pointType   totalPointSupply = 0;
        uint64_t    misByPoint;
        time_point  lastRewardsUpdate;
    };

    struct [[eosio::table, eosio::contract("mib.system")]] HospitalInfo {
        // scope: code, ram payer: hospital
        name        owner;
        string      url;

        // EMR 판매 수 + 해당 병원 후기 게시글 수 + 좋아요 수 * 0.01 + 후기를 통한 환자 방문 수 * 100
        double      serviceWeight = 0;

        // 초기화 필요 없음
        uint64_t    reviewCount = 0;
        uint32_t    totalReviewsLike = 0;

        // 초기화 필요함
        uint64_t    emrSales = 0;
        uint64_t    reviewVisitors = 0;

        uint64_t    primary_key() const { return owner.value; }
        double      byWeight() const { return -serviceWeight; }

        void        setWeight() { serviceWeight = reviewCount + totalReviewsLike + emrSales + (reviewVisitors * 100); }
    };

    struct [[eosio::table, eosio::contract("mib.system")]] CustomerInfo {
        // scope: code, ram payer: customer
        name            owner;

        pointType       point;

        vector<name>    hospitals;
        // uint64_t paidCount;

        // 현재 시간을 day로 나눠서 lastLikeTime 보다 크다면 하루가 지난것을 의미하기 때문에 하루에 세번 like 할 수 있도록 할 수 있다.
        // lastLikeTime이 (현재 시간 / day)와 같으면 remainLike가 있어야지 --remainLike 하고 Like 할 수 있도록
        uint8_t         remainLike = 3;
        time_point      lastLikeTime;

        uint64_t primary_key() const { return owner.value; }
    };

    struct [[eosio::table, eosio::contract("mib.system")]] ReviewInfo {
        // scope: hospital, ram payer: customer
        // 안아파톡에서 후기 게시글 고유 번호 reviwer, title, reviewJson을 hash하여 만듬
        uuidType    id;
        name        owner;

        uint64_t    likes; // 컨트랙트 내에서 처리해야 할까? 일별로 3개의 좋아요를 할 수 있는 제한을 구현하기가 애매하다. 시간으로?

        string      title;
        
        uint64_t primary_key() const { return id; }
        uint64_t byOwner() const { return owner.value; }
    };

    struct [[eosio::table, eosio::contract("mib.system")]] BillInfo {
        // scope: customer, ram payer: hospital
        uuidType    id;
        name        hospital;
        string      content;
        uint64_t    price;

        uint64_t primary_key() const { return id; }
        uint64_t byHospital() const { return hospital.value; }
    };
    
    typedef eosio::singleton< name("config"), ConfigInfo > configSingleton;

    typedef eosio::multi_index< name("hospitals"), HospitalInfo,
                                indexed_by< name("byweight"), const_mem_fun< HospitalInfo, double, &HospitalInfo::byWeight > >
                                > hospitalsTable;
    typedef eosio::multi_index< name("customers"), CustomerInfo > customersTable;
    typedef eosio::multi_index< name("reviews"), ReviewInfo,
                                indexed_by< name("byowner"), const_mem_fun< ReviewInfo, uint64_t, &ReviewInfo::byOwner > >
                                > reviewsTable;
    typedef eosio::multi_index< name("bills"), BillInfo,
                                indexed_by< name("byhospital"), const_mem_fun< BillInfo, uint64_t, &BillInfo::byHospital > >
                                > billsTable;

    class [[eosio::contract("mib.system")]] misblock : public eosio::contract {
        private:
            configSingleton _config;

            ConfigInfo      _cstate;

            ConfigInfo      getDefaultConfig();

        public:
            misblock( name receiver, name code, datastream<const char*> ds )
                : contract( receiver, code, ds ), _config( receiver, receiver.value ) {
                    _cstate = _config.exists() ? _config.get() : getDefaultConfig();
                }
            ~misblock() {
                _config.set( _cstate, get_self() );
            }

            [[eosio::action]]
            void giverewards();

            [[eosio::action]]
            void reghospital( const name& owner, const string& url );

            [[eosio::action]]
            void exchangemis( const name& owner, const pointType& point );

            [[eosio::action]]
            void postreview( const name& owner, const string& title, const string& reviewJson );

            [[eosio::action]]
            void like( const name& customer, const string& reviewId );

            [[eosio::action]]
            void providebill( const name& hospital, const name& customer, const string& content, const uint64_t price );

            [[eosio::action]]
            void paybill( const name& customer, const uuidType& billId, const uuidType& reviewId );
    };
}
