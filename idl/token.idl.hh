namespace dht {
class token {
    enum class kind : int {
        before_all_keys,
        key,
        after_all_keys,
    };
    dht::token::kind _kind;
    dht::token_data _data;
};
}
