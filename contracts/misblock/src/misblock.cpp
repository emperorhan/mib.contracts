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

#include "../include/misblock/misblock.hpp"
// #include "../../utils/common.h"

namespace misblock {
    void misblock::setmisratio( const types::pointType& misByPoint ) {
        require_auth( get_self() );
        check( misByPoint > 0, "must set positive value" );
        _cstate.misByPoint = misByPoint;
    }

    void misblock::setpubkey( const public_key& misPubKey ) {
        require_auth( get_self() );
        check( misPubKey != public_key(), "public key should not be the default value" );
        _cstate.misPubKey = misPubKey;
    }

    void misblock::givepoint( const name& owner, const types::pointType& point ) {
        require_auth( get_self() );
        check( point > 0, "must set positive point" );
        addPoint( owner, point );
    }

    void misblock::giverewards() {
        // misblock이 매달 상위 16개의 병원에게 100만 MIS 토큰을 보상함
        require_auth( get_self() );

        // 한달에 한번 보상해야함
        const auto ct = current_time_point();
        check( ( ct.sec_since_epoch() / common::secondsPerMonth ) > ( _cstate.lastRewardsUpdate.sec_since_epoch() / common::secondsPerMonth ), "already gave rewards within this month" ); 

        // 상위 16개의 병원
        hospitalsTable hospitaltable( get_self(), get_self().value );
        auto hospitalIdx = hospitaltable.get_index<name("byservice")>();

        int cnt = 0;
        for ( auto it = hospitalIdx.cbegin(); it != hospitalIdx.cend() && cnt < 16 && 0 < it->serviceWeight; ++it ) {
            common::transferToken(get_self(), it->owner, common::rewards, "monthly rewards");
            // 지급한 대상의 weight를 초기화해야함
            hospitaltable.modify( *it, get_self(), [&]( HospitalInfo& h ) {
                h.emrSales = 0;
                h.reviewVisitors = 0;
                h.totalReviewsLike = 0;
                h.setWeight();
            });
            cnt++;
        }

        // 게시글 포인트 리워드
        reviewsTable reviewtable( get_self(), get_self().value );
        customersTable customertable( get_self(), get_self().value );
        auto reviewIdx = reviewtable.get_index<name("bylike")>();

        cnt = 0;
        for ( auto it = reviewIdx.cbegin(); it != reviewIdx.cend() && cnt < 10 && 100 <= it->likes && !it->Expired(); ++it ) {
            types::pointType rewardPoint = 2000 - ( 200 * ( cnt ) );
            addPoint( it->owner, rewardPoint );
            reviewtable.modify( *it, get_self(), [&]( ReviewInfo& r ) {
                r.isExpired = true;
            });
            cnt++;
        }
        _cstate.lastRewardsUpdate = ct;
    }

    void misblock::reghospital( const name& owner, const string& url ) {
        check( url.size() < 512, "url too long" );

        hospitalsTable hospitaltable( get_self(), get_self().value );
        auto hitr = hospitaltable.find( owner.value );

        if ( hitr == hospitaltable.end() ) {
            require_auth( get_self() );

            hospitaltable.emplace( get_self(), [&]( HospitalInfo& h ) {
                h.owner             = owner;
                h.url               = url;
                h.serviceWeight     = 0;
                h.reviewCount       = 0;
                h.emrSales          = 0;
                h.reviewVisitors    = 0;
                h.totalReviewsLike  = 0;
            });
        } else {
            require_auth( owner );

            hospitaltable.modify( hitr, get_self(), [&]( HospitalInfo& h ) {
                h.url       = url;
            });
        }
    }

    void misblock::exchangemis( const name& owner, const pointType& point ) {
        // mib.c가 고객에게 misByPoint 만큼의 비율로 point를 차감하고 MIS 토큰을 지급
        is_account( owner );
        check( point >= _cstate.misByPoint, "minimum quantity is 1.0000 MIS" );

        customersTable customertable( get_self(), get_self().value );
        auto citr = customertable.find( owner.value );
        check( citr != customertable.end(), "customer does not exist" );
        check( citr->point >= point, "customer's points are insufficient" );

        const asset quantity = asset( uint64_t(point * 10000) / _cstate.misByPoint, common::S_MIS );

        subPoint( owner, point );
        common::transferToken( get_self(), owner, quantity, "exchange mistoken" );
    }

    void misblock::postreview( const name& owner, const name& hospital, const uuidType& reviewId, const string& title, const string& reviewJson, const signature& sig ) {
        require_auth( owner );

        // customersTable customertable( get_self(), get_self().value );
        // auto citr = customertable.require_find( owner.value, "you are not a customer" );
        // check( citr->hospitals.find( hospital ) != citr->hospitals.end(), "you must pay medical bills of that hospital through paymedical" );

        check( title.size() < 512, "title should be less than 512 characters long" );
        common::validateJson( reviewJson );

        // misblock은 작성자, 병원, 제목, 리뷰내용을 config state에 등록된 공개키에 해당하는 개인키로 시그니처를 만들어서 제공해야함(무분별한 post 방지)
        string data = owner.to_string() + hospital.to_string() + common::uint64_to_string(reviewId) + title + reviewJson;
        
        const checksum256 digest = sha256( data.data(), data.size() );


        // test code
        auto pk = recover_key( digest, sig );

        testpubsTable testpubtable( get_self(), get_self().value );
        auto pitr = testpubtable.find( 0 );
        if ( pitr == testpubtable.end() ) {
            testpubtable.emplace(get_self(), [&]( TestPubInfo& p ) {
                p.checkPubKey = pk;
                p.checkSig    = sig;
            });
        } else {
            testpubtable.modify(pitr, get_self(), [&]( TestPubInfo& p ) {
                p.checkPubKey = pk;
                p.checkSig    = sig;
            });
        }
        // -----------------------------------------

        
        // string aa;
        // for(auto it = _cstate.misPubKey.data.begin(); it != _cstate.misPubKey.data.end(); ++it){
        //     aa += *it;
        // }

        // string pubhex = to_hex(aa.c_str(), aa.size()).substr(2); // remove leading '00'
        // string tmp = hex_to_string(pubhex.c_str());
        // aa = tmp;

        // checksum160 chksm = ripemd160(aa.c_str(), 33);

        // tmp = hex_to_string(pubhex + to_hex(&chksm, 20).substr(0,8)); // append checksum

        // unsigned char encoded[37  * 137 / 100];
        // base58encode(tmp, 37, encoded);
        // tmp = "EOS" + std::string(reinterpret_cast<char*>(encoded));
        // assert(tmp.length() == 53);
        // print(tmp);

        eosio::printl( data.c_str(), data.size() );
        digest.print();
        // eosio::printl( "\n", 1);
        // string aa;
        // string bb;
        // for(auto it = _cstate.misPubKey.data.begin(); it != _cstate.misPubKey.data.end(); ++it){
        //     aa += *it;
        // }
        // for(auto it = test.data.begin(); it != test.data.end(); ++it){
        //     bb += *it;
        // }
        // eosio::printl(aa.c_str(), aa.size());
        // eosio::printl( "\n", 1);
        // eosio::printl(bb.c_str(), bb.size());
        // eosio::printl( "\n", 1);
        
        // check( test == _cstate.misPubKey, "sadasdadsad!!!!" );

        /* 잠시 주석처리
        assert_recover_key( digest, sig, _cstate.misPubKey );
        
        hospitalsTable hospitaltable( get_self(), get_self().value );
        auto hitr = hospitaltable.find( hospital.value );
        check( hitr != hospitaltable.end(), "hospital does not exist" );

        reviewsTable reviewtable( get_self(), get_self().value );
        check( reviewtable.find( reviewId ) == reviewtable.end(), "reviewId alreay exist" );
        reviewtable.emplace( get_self(), [&]( ReviewInfo& r ) {
            r.id            = reviewId;
            r.owner         = owner;
            r.hospital      = hospital;
            r.likes         = 0;
            r.title         = title;
        });

        hospitaltable.modify( hitr, get_self(), [&]( HospitalInfo& h ) {
            h.reviewCount++;
            h.setWeight();
        });

        customertable.modify( citr, get_self(), [&]( CustomerInfo& c ) {
            c.hospitals.erase( hospital );
        });
        */
    }

    void misblock::like( const name& owner, const uint64_t& reviewId ) {
        require_auth( owner );

        reviewsTable reviewtable( get_self(), get_self().value );
        auto ritr = reviewtable.require_find( reviewId, "review does not exist" );

        hospitalsTable hospitaltable( get_self(), get_self().value );
        auto hitr = hospitaltable.require_find( ritr->hospital.value, "hospital does not exist" );

        addPoint( owner, 5 );

        // 하루에 세번 좋아요
        customersTable customertable( get_self(), get_self().value );
        auto citr = customertable.find( owner.value );
        const auto ct = current_time_point();
        customertable.modify( citr, get_self(), [&]( CustomerInfo& c ) {
            // 하루가 지났으면
            if ( ( ct.sec_since_epoch() / common::secondsPerDay) > ( citr->lastLikeTime.sec_since_epoch() / common::secondsPerDay ) ) {
                c.remainLike = 2;
            } else {
                check( citr->remainLike, "there are no remaining likes" );
                c.remainLike--;
            }
            c.lastLikeTime = ct;
        });

        reviewtable.modify( ritr, get_self(), [&]( ReviewInfo& r ) {
            r.likes++;
        });

        hospitaltable.modify( hitr, get_self(), [&]( HospitalInfo& h ) {
            h.totalReviewsLike++;
            h.setWeight();
        });
    }

    // void misblock::paymedical( const name& customer, const name& hospital, const string& service, const asset& cost, const bool& isCash, const signature& bill, const types::uuidType& reviewId = common::nullID ) {
    //     require_auth( customer );
    //     is_account( hospital );
    //     check( service.size() < 32768, "service should be less than 32768 characters long" );
    //     check( cost.is_valid(), "invalid cost" );
    //     check( cost.amount >= 10000, "minimum cost is 1 MIS" );
    //     check( cost.symbol == common::S_MIS, "cost symbol must be MIS" );

    //     string data = hospital.to_string() + customer.to_string() + service + cost.to_string() + ( isCash ? "Cash" : "MIS" );
    //     const checksum256 digest = sha256( &data[0], data.size() );

    //     hospitalsTable hospitaltable( get_self(), get_self().value );
    //     auto hitr = hospitaltable.require_find( hospital.value, "hospital does not exist" );

    //     assert_recover_key( digest, bill, hitr->hosPubKey );

    //     reviewsTable reviewtable( get_self(), get_self().value );

    //     const asset fee( cost.amount / 10, cost.symbol );
    //     if ( reviewId != common::nullID ) {
    //         auto ritr = reviewtable.require_find( reviewId, "review does not exist" );
    //         check( ritr->hospital == hospital, "invalid reviewId" );

    //         const asset payReward( fee.amount * 0.5, fee.symbol );
    //         const asset reviewReward( fee.amount * 0.4, fee.symbol );

    //         if ( isCash == true ) {
    //             common::transferToken( hospital, get_self(), fee, "misblock fee" );
    //             common::transferToken( get_self(), customer, payReward, "misblock pay reward" );
    //             common::transferToken( get_self(), ritr->owner, reviewReward, "misblock review reward" );    
    //         } else {
    //             common::transferToken( customer, hospital, cost, "pay medical bills" );
    //             common::transferToken( hospital, get_self(), fee, "misblock fee" );
    //             common::transferToken( get_self(), customer, payReward, "misblock pay reward" );
    //             common::transferToken( get_self(), ritr->owner, reviewReward, "misblock review reward" );
    //         }

    //         hospitaltable.modify( hitr, get_self(), [&]( HospitalInfo& h ) {
    //             h.reviewVisitors++;
    //             h.setWeight();
    //         }); 
    //     } else {
    //         if ( isCash == true ) {
    //             common::transferToken( hospital, get_self(), fee, "misblock fee" );
    //         } else {
    //             common::transferToken( customer, hospital, cost, "pay medical bills" );
    //             common::transferToken( hospital, get_self(), fee, "misblock fee" );
    //         }
    //     }
    //     customersTable customertable( get_self(), get_self().value );
    //     auto citr = customertable.find( customer.value );
    //     if ( citr == customertable.end() ) {
    //         customertable.emplace( get_self(), [&]( CustomerInfo& c ) {
    //             c.owner = customer;
    //             c.point = 0;
    //             c.hospitals.emplace( hospital );
    //             c.remainLike = 3;
    //         });
    //     } else {
    //         customertable.modify( citr, get_self(), [&]( CustomerInfo& c) {
    //             c.hospitals.emplace( hospital );
    //         });
    //     }
    // }

    void misblock::transferevnt( const uint64_t& sender, const uint64_t& receiver ) {
        misblock::transferEventHandler( sender, receiver, [&]( const types::eventArgs& e ) {
            check( e.action.size(), "Invalid transfer" );

            customersTable customertable( get_self(), get_self().value );
            hospitalsTable hospitaltable( get_self(), get_self().value );

            auto fromCustomer = customertable.find( e.from.value );
            auto fromHospital = hospitaltable.find( e.from.value );
            
            uint32_t hash = common::constHash( e.action.c_str() );
            switch ( hash ) {
            case common::constHash( "paybillmis" ):
                if ( e.action == "paybillmis" ) {
                    // memo => "paywithmis:hospital:reviewId"
                    name customer = e.from;
                    name hospital = name(e.param[0]);
                    asset cost = e.quantity;

                    is_account( hospital );

                    if ( e.param.size() > 1 ) {
                        types::uuidType reviewId = stoull(e.param[1]);
                        paybillmis( customer, hospital, cost, reviewId );
                    } else {
                        paybillmis( customer, hospital, cost );
                    }
                }
                break;
            case common::constHash( "paybillcash" ):
                if ( e.action == "paybillcash" ) {
                    // memo => "paybillcash:customer:reviewId"
                    name hospital = e.from;
                    name customer = name(e.param[0]);
                    asset cost = e.quantity;

                    is_account( customer );
                        
                    if ( e.param.size() > 1 ) {
                        types::uuidType reviewId = stoull(e.param[1]);
                        paybillcash( customer, hospital, cost, reviewId );
                    } else {
                        paybillcash( customer, hospital, cost );
                    }
                }
                break;
            case common::constHash( "payconsmis" ):
                if ( e.action == "payconsmis" ) {
                    // TODO: 추후 원격상담 결제 "payconsmis:hospitalName:???"
                }
                break;
            default:
                check( false, "invalid action" );
                break;
            }
        });
    }

    template<typename T>
    void misblock::transferEventHandler( uint64_t sender, uint64_t receiver, T func ) {
        auto transferData = eosio::unpack_action_data<types::transferArgs>();
        check( transferData.quantity.is_valid(), "Invalid token transfer" );
        check( transferData.quantity.amount > 0, "Quantity must be positive" );
        if ( transferData.quantity.symbol != common::S_MIS ) return;
        
        if ( transferData.from == get_self() ) {
            // misblock이 led.token::transfer로 보냈을때
            // 여기서 assert를 주면 misblock이 누군가에게 돈을 보내는게 막히게 되는가?
            return;
        } else if ( transferData.to == get_self() ) {
            // misblock이 led.token::transfer로 받았을때
            name from = transferData.from;
            string& memo = transferData.memo;

            types::eventArgs res;

            size_t prev = memo.find( ':' );
            res.from = from;
            res.quantity = transferData.quantity;
            res.action = memo.substr( 0, prev );
            size_t pos = prev;
            while ( true ) {
                pos = memo.find( ':', pos + 1 );
                if ( pos == std::string::npos ) {
                    res.param.emplace_back( memo.substr( prev + 1 ) );
                    break;
                } else {
                    res.param.emplace_back( memo.substr( prev + 1, ( pos - ( prev + 1 ) ) ) );
                }
                prev = pos;
            }
            func(res);
        }
    }

    void misblock::paybillmis( const name& customer, const name& hospital, const asset& cost, const uuidType& reviewId ) {
        customersTable customertable( get_self(), get_self().value );
        hospitalsTable hospitaltable( get_self(), get_self().value );
        reviewsTable reviewtable( get_self(), get_self().value );

        auto hitr = hospitaltable.require_find( hospital.value, "hospital does not exist" );

        if ( reviewId == nullID ) {
            const asset payReward( cost.amount * 0.03, cost.symbol );
            
            common::transferToken( get_self(), customer, payReward, "misblock pay reward" );
        } else {
            auto ritr = reviewtable.require_find( reviewId, "review does not exist" );
            check( ritr->hospital == hospital, "invalid reviewId" );

            const asset payReward( cost.amount * 0.05, cost.symbol );
            const asset reviewReward( cost.amount * 0.04, cost.symbol );
            
            common::transferToken( get_self(), customer, payReward, "misblock pay reward" );
            common::transferToken( get_self(), ritr->owner, reviewReward, "misblock review reward" );
        }
        
        hospitaltable.modify( hitr, get_self(), [&]( HospitalInfo& h ) {
            h.reviewVisitors++;
            h.setWeight();
        }); 
        
        auto citr = customertable.find( customer.value );
        if ( citr == customertable.end() ) {
            customertable.emplace( get_self(), [&]( CustomerInfo& c ) {
                c.owner = customer;
                c.point = 0;
                c.hospitals.emplace( hospital );
                c.remainLike = 3;
            });
        } else {
            customertable.modify( citr, get_self(), [&]( CustomerInfo& c ) {
                c.hospitals.emplace( hospital );
            });
        }
    }

    void misblock::paybillcash( const name& customer, const name& hospital, const asset& cost, const uuidType& reviewId ) {
        customersTable customertable( get_self(), get_self().value );
        hospitalsTable hospitaltable( get_self(), get_self().value );
        reviewsTable reviewtable( get_self(), get_self().value );

        auto hitr = hospitaltable.find( hospital.value );
        check(  hitr != hospitaltable.end(), ( hospital.to_string() + " is not hospital" ).c_str() );
        // auto hitr = hospitaltable.require_find( hospital.value, ( hospital.to_string() + " is not hospital" ).c_str() );

        if ( reviewId == nullID ) {
            const asset payReward( cost.amount * 0.3, cost.symbol );

            common::transferToken( get_self(), customer, payReward, "misblock pay reward" );
        } else {
            auto ritr = reviewtable.require_find( reviewId, "review does not exist" );
            check( ritr->hospital == hospital, "invalid reviewId" );

            const asset payReward( cost.amount * 0.5, cost.symbol );
            const asset reviewReward( cost.amount * 0.4, cost.symbol );

            common::transferToken( get_self(), customer, payReward, "misblock pay reward" );
            common::transferToken( get_self(), ritr->owner, reviewReward, "misblock review reward" );
        }

        hospitaltable.modify( hitr, get_self(), [&]( HospitalInfo& h ) {
            h.reviewVisitors++;
            h.setWeight();
        }); 

        auto citr = customertable.find( customer.value );
        if ( citr == customertable.end() ) {
            customertable.emplace( get_self(), [&]( CustomerInfo& c ) {
                c.owner = customer;
                c.point = 0;
                c.hospitals.emplace( hospital );
                c.remainLike = 3;
            });
        } else {
            customertable.modify( citr, get_self(), [&]( CustomerInfo& c ) {
                c.hospitals.emplace( hospital );
            });
        }
    }

    void misblock::addPoint( const name& owner, const types::pointType& point ) {
        customersTable customertable( get_self(), get_self().value );
        auto citr = customertable.find( owner.value );

        // name payer = !has_auth( owner ) ? get_self() : owner;
        if ( citr == customertable.end() ) {
            customertable.emplace( get_self(), [&]( CustomerInfo& c ) {
                c.owner = owner;
                c.point = point;
                c.remainLike = 3;
            });
        } else {
            customertable.modify( citr, get_self(), [&]( CustomerInfo& c ) {
                c.point += point;
            });
        }
        _cstate.totalPointSupply += point;
    }

    void misblock::subPoint( const name& owner, const types::pointType& point ) {
        customersTable customertable( get_self(), get_self().value );
        const auto& customer = customertable.get( owner.value, "customer does not exist" );
        check( customer.point >= point, "overdrawn point" );

        // name payer = !has_auth(owner) ? same_payer : owner;

        customertable.modify( customer, get_self(), [&]( CustomerInfo& c ) {
            c.point -= point;
        });
        _cstate.totalPointSupply -= point;
    }
}
// code: 실행 계정 명, receiver: 수행 대상 계정 명? (내 생각에는 require_recipient를 받는 리시버를 의미하는 것 같다)
extern "C" {
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        auto self = receiver;

        if ( code == self ) switch( action ) {
            EOSIO_DISPATCH_HELPER( misblock::misblock, (setmisratio)(setpubkey)(givepoint)(giverewards)(reghospital)(exchangemis)(postreview)(like)(transferevnt) )
        } else {
            if ( code == name("led.token").value && action == name("transfer").value ) {
                execute_action( name(receiver), name(code), &misblock::misblock::transferevnt );
            }
        }
    }
};