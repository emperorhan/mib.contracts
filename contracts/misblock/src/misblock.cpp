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

namespace misblock {
    void misblock::exchangemis( const name& owner, const pointType point ) {
        // mib.c가 고객에게 misByPoint 만큼의 비율로 point를 차감하고 MIS 토큰을 지급
        is_account( owner );

        customersTable customerstable( get_self(), get_self().value );

        auto existingCustomer = customerstable.find( owner.value );
        check(existingCustomer != customerstable.end(), "customer does not exist");
        const auto& customer = *existingCustomer;

        // check(  );
    }
}


// void misblock::misblock::exchangemis( const name& owner, const pointType point ){

// }