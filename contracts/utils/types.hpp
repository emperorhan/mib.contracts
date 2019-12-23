#pragma ones

namespace types {
enum transferEventActions : uint8_t {
    PAY_WITH_MIS    = 0,
    PAY_WITH_CASH   = 1
};

typedef uint64_t uuidType;
typedef uint64_t pointType;

struct transferArgs {
    name from;
    name to;
    asset quantity;
    string memo;
};

struct eventArgs {
    name            from;
    asset           quantity;
    string          action;
    vector<string>  param;
};
}  // namespace types