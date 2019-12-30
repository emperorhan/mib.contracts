#pragma once

namespace types {
enum transferEventActions : uint8_t {
    PAY_WITH_MIS    = 0,
    PAY_WITH_CASH   = 1
};

enum customerTiers : uint8_t {
    BABY        = 0,
    BRONZE      = 5,
    SILVER      = 10,
    GOLD        = 15,
    PLATINUM    = 20,
    DIAMOND     = 25
};

typedef uint64_t uuidType;
typedef uint64_t pointType;

struct transferArgs {
    eosio::name     from;
    eosio::name     to;
    eosio::asset    quantity;
    std::string     memo;
};

struct eventArgs {
    eosio::name                 from;
    eosio::asset                quantity;
    std::string                 action;
    std::vector<std::string>    param;
};
}  // namespace types