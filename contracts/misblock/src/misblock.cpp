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
#include "../../utils/common.hpp"

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
            hospitaltable.modify( *it, same_payer, [&]( HospitalInfo& h ) {
                h.emrSales = 0;
                h.reviewVisitors = 0;
                h.totalReviewsLike = 0;
                h.setWeight();
            });
            cnt++;
        }

        // 게시글 포인트 리워드
        reviewsTable reviewtable( get_self(), get_first_receiver().value );
        customersTable customertable( get_self(), get_first_receiver().value );
        auto reviewIdx = reviewtable.get_index<name("byweight")>();

        cnt = 0;
        for ( auto it = reviewIdx.cbegin(); it != reviewIdx.cend() && cnt < 10 && 100 <= it->likes && !it->isExpired; ++it ) {
            types::pointType rewardPoint = 2000 - ( 200 * ( cnt ) );
            _cstate.totalPointSupply += rewardPoint;
            auto citr = customertable.find( it->owner.value );
            customertable.modify( citr, same_payer, [&]( CustomerInfo& c ) {
                c.point += rewardPoint;
            });
            reviewtable.modify( *it, same_payer, [&]( ReviewInfo& r ) {
                r.isExpired = true;
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
            hospitaltable.emplace( owner, [&]( HospitalInfo& h ) {
                h.owner             = owner;
                h.url               = url;
                h.serviceWeight     = 0;
                h.reviewCount       = 0;
                h.emrSales          = 0;
                h.reviewVisitors    = 0;
                h.totalReviewsLike  = 0;
            });
        } else {
            hospitaltable.modify( hitr, same_payer, [&]( HospitalInfo& h ) {
                h.url = url;
            });
        }
    }

    void misblock::exchangemis( const name& owner, const pointType& point ) {
        // mib.c가 고객에게 misByPoint 만큼의 비율로 point를 차감하고 MIS 토큰을 지급
        is_account( owner );
        check( point >= _cstate.misByPoint, "minimum quantity is 1.0000 MIS" );

        customersTable customertable( get_self(), get_first_receiver().value );
        auto citr = customertable.find( owner.value );
        check( citr != customertable.end(), "customer does not exist" );
        check( citr->point >= point, "customer's points are insufficient" );

        const asset quantity = asset( uint64_t(point * 10000) / _cstate.misByPoint, common::S_MIS );
        customertable.modify( citr, same_payer, [&]( CustomerInfo& c ) {
            c.point -= point;
        });
        common::transferToken( get_self(), owner, quantity, "exchange mistoken" );
    }

    void misblock::postreview( const name& owner, const name& hospital, const string& title, const string& reviewJson, const signature& sig ) {
        require_auth( owner );
        // TODO: 작성자가 해당 병원에 대해 bill이 있는지 확인 필요

        check( title.size() < 512, "title should be less than 512 characters long" );
        common::validateJson( reviewJson );

        // misblock은 작성자, 병원, 제목, 리뷰내용을 config state에 등록된 공개키에 해당하는 개인키로 시그니처를 만들어서 제공해야함(무분별한 post 방지)
        string data = owner.to_string() + hospital.to_string() + title + reviewJson;
        const checksum256 digest = sha256( &data[0], data.size() );
        assert_recover_key( digest, sig, _cstate.misPubKey );

        hospitalsTable hospitaltable( get_self(), get_first_receiver().value );
        auto hitr = hospitaltable.find( hospital.value );
        check( hitr != hospitaltable.end(), "hospital does not exist" );

        reviewsTable reviewtable( get_self(), get_first_receiver().value );
        uint64_t reviewId = reviewtable.available_primary_key();
        reviewtable.emplace( owner, [&]( ReviewInfo& r ) {
            // owner, hospital, title, reviewJson를 해시해서 id를 만듬 - 미정(std::hash가 안전하지 않은 해시)
            // r.id = common::toUUID( data );
            r.id            = reviewId;
            r.owner         = owner;
            r.hospital      = hospital;
            r.likes         = 0;
            r.title         = title;
        });

        hospitaltable.modify( hitr, same_payer, [&]( HospitalInfo& h ) {
            h.reviewCount++;
            h.setWeight();
        });
    }

    void misblock::like( const name& owner, const uint64_t& reviewId ) {
        require_auth( owner );

        customersTable customertable( get_self(), get_first_receiver().value );
        auto citr = customertable.find( owner.value );

        reviewsTable reviewtable( get_self(), get_first_receiver().value );
        auto ritr = reviewtable.require_find( reviewId, "review does not exist" );

        hospitalsTable hospitaltable( get_self(), get_first_receiver().value );
        auto hitr = hospitaltable.require_find( ritr->hospital.value, "hospital does not exist" );


        // 하루에 세번 좋아요
        const auto ct = current_time_point();
        customertable.modify( citr, same_payer, [&]( CustomerInfo& c ) {
            // 하루가 지났으면
            if ( (ct - citr->lastLikeTime) > microseconds(common::usecondsPerDay) ) {
                c.remainLike = 2;
            } else {
                check( citr->remainLike, "there are no remaining likes" );
                c.remainLike--;
            }
            c.point += 5;
            c.lastLikeTime = ct;
        });
        _cstate.totalPointSupply += 5;

        reviewtable.modify( ritr, same_payer, [&]( ReviewInfo& r ) {
            r.likes++;
        });

        hospitaltable.modify( hitr, same_payer, [&]( HospitalInfo& h ) {
            h.totalReviewsLike++;
        });
    }

    void misblock::providebill( const name& hospital, const name& customer, const string& content, const asset& price ) {
        require_auth( hospital );
        is_account( customer );
        
        check( content.size() < 512, "content should be less than 512 characters long" );

        check( price.is_valid(), "invalid price" );
        check( price.amount > 10000, "minimum price is 1 MIS" );
        check( price.symbol == common::S_MIS, "price symbol must be MIS" );
        
        billsTable billtable( get_self(), customer.value );
        uint64_t billId = billtable.available_primary_key();
        billtable.emplace( hospital, [&]( BillInfo& b ) {
            b.id        = billId;
            b.hospital  = hospital;
            b.content   = content;
            b.price     = price;
        });
    }

    void misblock::paybill( const name& customer, const uuidType& billId, const uuidType& reviewId ) {
        require_auth( customer );

        billsTable billtable( get_self(), customer.value );
        auto bitr = billtable.require_find( billId, "bill does not exist" );

        reviewsTable reviewtable( get_self(), get_first_receiver().value );
        auto ritr = reviewtable.require_find( reviewId, "review does not exist" );

        common::transferToken( get_self(), owner, quantity, "exchange mistoken" );



    }
}