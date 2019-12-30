#pragma once

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

#include "../../../utils/common.h"

using namespace types;
using namespace common;

namespace misblock {
    using namespace std;
    using namespace eosio;

    // 임시 테스트 테이블
    struct [[eosio::table, eosio::contract("misblock")]] TestPubInfo {
        uuidType    id;
        public_key  checkPubKey;
        signature   checkSig;

        uint64_t    primary_key() const { return id; }
    };
    typedef eosio::multi_index< name("testpubs"), TestPubInfo > testpubsTable;
    // -------------------------------------------------------------------------

    struct [[eosio::table("config"), eosio::contract("misblock")]] ConfigInfo {
        pointType   totalPointSupply = 0;
        uint64_t    misByPoint;
        pointType   likeReward;

        public_key  misPubKey;
        time_point  lastRewardsUpdate;
    };

    struct [[eosio::table, eosio::contract("misblock")]] HospitalInfo {
        // scope: code, ram payer: misblock
        name        owner;
        string      url;

        // EMR 판매 수 + 해당 병원 후기 게시글 수 + 좋아요 수 * 0.01 + 후기를 통한 환자 방문 수 * 100
        double      serviceWeight = 0;

        // 초기화 필요 없음
        uint32_t    reviewCount = 0;

        // 초기화 필요함
        uint32_t    emrSales = 0;
        uint32_t    reviewVisitors = 0;
        uint32_t    totalReviewsLike = 0;

        uint64_t    primary_key() const { return owner.value; }
        double      byWeight()    const { return -serviceWeight; }

        void        setWeight() { serviceWeight = reviewCount + emrSales + (reviewVisitors * 100) + (totalReviewsLike * 0.01); }
    };

    struct [[eosio::table, eosio::contract("misblock")]] CustomerInfo {
        // scope: code, ram payer: misblock
        name            owner;
        customerTiers   tier;
        pointType       point;

        set<name>       hospitals;

        // 현재 시간을 day로 나눠서 lastLikeTime 보다 크다면 하루가 지난것을 의미하기 때문에 하루에 세번 like 할 수 있도록 할 수 있다.
        // lastLikeTime이 (현재 시간 / day)와 같으면 remainLike가 있어야지 --remainLike 하고 Like 할 수 있도록
        uint8_t         remainLike = 3;
        time_point      lastLikeTime;

        uint64_t primary_key() const { return owner.value; }

        void     setTier() {
            switch (point) {
            case 0 ... 9999:
                tier = BABY;
                break;
            case 10000 ... 99999:
                tier = BRONZE;
                break;
            case 100000 ... 999999:
                tier = SILVER;
                break;
            case 1000000 ... 9999999:
                tier = GOLD;
                break;
            case 10000000 ... 99999999:
                tier = PLATINUM;
                break;
            default:
                tier = DIAMOND;
                break;
            }
        }
    };

    struct [[eosio::table, eosio::contract("misblock")]] ReviewInfo {
        // scope: code, ram payer: misblock
        uuidType    id;
        name        owner;
        name        hospital;

        bool        isExpired = 0;
        int32_t     likes = 0; // 컨트랙트 내에서 처리해야 할까? 일별로 3개의 좋아요를 할 수 있는 제한을 구현하기가 애매하다. 시간으로?

        string      title;
        
        uint64_t primary_key()  const { return id; }
        bool     Expired()      const { return isExpired; }
        uint64_t byOwner()      const { return owner.value; }
        uint64_t byHospital()   const { return hospital.value; }
        double   byWeight()     const { return isExpired ? (double)likes : -(double)likes; }
    };

    typedef eosio::singleton< name("config"), ConfigInfo > configSingleton;

    typedef eosio::multi_index< name("hospitals"), HospitalInfo,
                                indexed_by< name("byservice"), const_mem_fun< HospitalInfo, double, &HospitalInfo::byWeight > >
                                > hospitalsTable;
    typedef eosio::multi_index< name("customers"), CustomerInfo > customersTable;
    typedef eosio::multi_index< name("reviews"), ReviewInfo,
                                indexed_by< name("byowner"), const_mem_fun< ReviewInfo, uint64_t, &ReviewInfo::byOwner > >,
                                indexed_by< name("bylike"), const_mem_fun< ReviewInfo, double, &ReviewInfo::byWeight > >
                                > reviewsTable;

    class [[eosio::contract("misblock")]] misblock : public eosio::contract {
        private:
            configSingleton _config;

            ConfigInfo      _cstate;

            ConfigInfo      getDefaultConfig() {
                return ConfigInfo{
                    0,
                    100,
                    20,
                    public_key(),
                    current_time_point()
                };
            };

            template<typename T>
            void transferEventHandler( uint64_t sender, uint64_t receiver, T func );
            void paybillmis( const name& customer, const name& hospital, const asset& cost, const uuidType& reviewId = nullID );
            void paybillcash( const name& customer, const name& hospital, const asset& cost, const uuidType& reviewId = nullID );
            void addPoint( const name& owner, const pointType& point );
            void subPoint( const name& owner, const pointType& point );

        public:
            misblock( name receiver, name code, datastream<const char*> ds )
                : contract( receiver, code, ds ), _config( receiver, receiver.value ) {
                    _cstate = _config.exists() ? _config.get() : getDefaultConfig();
                }
            ~misblock() {
                _config.set( _cstate, get_self() );
            }

            [[eosio::action]]
            void setmisratio( const pointType& misByPoint );

            [[eosio::action]]
            void setpubkey( const public_key& misPubKey );

            [[eosio::action]]
            void setlikerwd( const pointType& likeReward );

            [[eosio::action]]
            void givepoint( const name& owner, const pointType& point, const string& memo );

            [[eosio::action]]
            void burnpoint( const name& owner, const pointType& point, const string& memo );

            [[eosio::action]]
            void giverewards();

            [[eosio::action]]
            void reghospital( const name& owner, const string& url );

            [[eosio::action]]
            void exchangemis( const name& owner, const pointType& point );

            [[eosio::action]]
            void postreview( const name& owner, const name& hospital, const uuidType& reviewId, const string& title, const string& reviewJson, const signature& sig = signature() );

            [[eosio::action]]
            void like( const name& owner, const uint64_t& reviewId );

            [[eosio::action]]
            void transferevnt( const uint64_t& sender, const uint64_t& receiver );

            using givepointAction = action_wrapper< name("givepoint"), &misblock::givepoint >;
            using burnpointAction = action_wrapper< name("burnpoint"), &misblock::burnpoint >;
    };
}
