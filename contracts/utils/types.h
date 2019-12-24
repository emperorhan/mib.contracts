#pragma once

namespace types {
enum transferEventActions : uint8_t {
    PAY_WITH_MIS    = 0,
    PAY_WITH_CASH   = 1
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