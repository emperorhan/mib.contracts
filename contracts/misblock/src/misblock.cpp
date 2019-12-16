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

#include "../include/misblock.hpp"
#include "../../utils/common.h"

namespace misblock {
    void misblock::giverewards() {
        // misblock이 매달 상위 16개의 병원에게 100만 MIS 토큰을 보상함
        require_auth( get_self() );

        // 한달에 한번 보상해야함
        const auto ct = current_time_point();
        check( ct - _cstate.lastRewardsUpdate > microseconds(common::usecondsPerMonth), "already gave rewards within this month" ); 

        // 상위 16개의 병원
        hospitalsTable hospitaltable( get_self(), get_first_receiver().value );
        auto hospitalIdx = hospitaltable.get_index<name("byweight")>();

        int cnt = 0;
        for ( auto it = hospitalIdx.cbegin(); it != hospitalIdx.cend() && cnt < 16 && 0 < it->serviceWeight; ++it ) {
            common::transferToken(get_self(), it->owner, common::rewards, "monthly rewards");
            // 지급한 대상의 weight를 초기화해야함
            hospitaltable.modify( *it, same_payer, [&](auto& h) {
                h.emrSales = 0;
                h.reviewVisitors = 0;
                h.setWeight();
            });
            cnt++;
        }

        _cstate.lastRewardsUpdate = ct;
    }

    void misblock::reghospital( const name& owner, const string& url ) {
        require_auth( owner );

        check( url.size() < 512, "url too long" );

        hospitalsTable hospitaltable( get_self(), get_first_receiver().value );
        auto hitr = hospitaltable.find( owner.value );

        if ( hitr == hospitaltable.end() ) {
            hospitaltable.emplace( owner, [&]( auto& h ) {
                h.owner             = owner;
                h.url               = url;
                h.serviceWeight     = 0;
                h.reviewCount       = 0;
                h.totalReviewsLike  = 0;
                h.emrSales          = 0;
                h.reviewVisitors    = 0;
            });
        } else {
            hospitaltable.modify( hitr, same_payer, [&]( auto& h ) {
                h.url = url;
            });
        }
    }

    void misblock::exchangemis( const name& owner, const pointType& point ) {
        // mib.c가 고객에게 misByPoint 만큼의 비율로 point를 차감하고 MIS 토큰을 지급
        is_account( owner );

        customersTable customertable( get_self(), get_first_receiver().value );
        auto existingCustomer = customertable.find( owner.value );
        check(existingCustomer != customertable.end(), "customer does not exist");
        // const auto& customer = *existingCustomer;

        const asset quantity = asset( uint64_t(point * 10000) / _cstate.misByPoint, common::S_MIS );
        customertable.modify( existingCustomer, same_payer, [&]( auto& c ) {
            c.point -= point;
        });
        common::transferToken( get_self(), owner, quantity, "exchange mistoken" );
    }

    void misblock::postreview( const name& owner, const string& title, const string& reviewJson ) {
        require_auth( owner );
        check( title.size() < 512, "title should be less than 512 characters long" );
    }

    void misblock::paybill( const name& customer, const uuidType& billId, const uuidType& reviewId ) {

    }

    void misblock::providebill( const name& hospital, const name& customer, const string& content, const uint64_t price ) {

    }
}