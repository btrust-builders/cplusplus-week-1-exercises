#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <optional>
#include <regex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "main.h"

namespace {
constexpr long long kBtcToSats = 100'000'000LL;
constexpr int kGenesisBlockHeight = 0;
constexpr int kFirstHalving = 210'000;
}  // namespace

TEST(BitcoinFunctions, CalculateTotalReward) {
    EXPECT_DOUBLE_EQ(calculate_total_reward(10), 31.25);
}

TEST(BitcoinFunctions, IsValidTxFee) {
    EXPECT_TRUE(is_valid_tx_fee(0.001));
    EXPECT_FALSE(is_valid_tx_fee(0.02));
}

TEST(BitcoinFunctions, IsLargeBalance) {
    EXPECT_TRUE(is_large_balance(100.0));
    EXPECT_FALSE(is_large_balance(10.0));
}

TEST(BitcoinFunctions, TxPriority) {
    EXPECT_EQ(tx_priority(250, 0.02), "high");
    EXPECT_EQ(tx_priority(1000, 0.02), "medium");
    EXPECT_EQ(tx_priority(10000, 0.02), "low");
}

TEST(BitcoinFunctions, IsMainnet) {
    EXPECT_TRUE(is_mainnet("MAINNET"));
    EXPECT_FALSE(is_mainnet("testnet"));
}

TEST(BitcoinFunctions, IsInRange) {
    EXPECT_TRUE(is_in_range(150));
    EXPECT_FALSE(is_in_range(50));
}

TEST(BitcoinFunctions, IsSameWallet) {
    Wallet wallet{"abc"};
    Wallet other{"abc"};
    EXPECT_TRUE(is_same_wallet(wallet, wallet));
    EXPECT_FALSE(is_same_wallet(wallet, other));
}

TEST(BitcoinFunctions, NormalizeAddress) {
    EXPECT_EQ(normalize_address("  1abcDEF  "), "1abcdef");
}

TEST(BitcoinFunctions, AddUtxo) {
    std::vector<int> expected{1, 2, 3};
    EXPECT_EQ(add_utxo({1, 2}, 3), expected);
}

TEST(BitcoinFunctions, FindHighFee) {
    auto found = find_high_fee({0.001, 0.003, 0.006});
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->first, static_cast<std::size_t>(2));
    EXPECT_DOUBLE_EQ(found->second, 0.006);

    EXPECT_FALSE(find_high_fee({0.001, 0.003}).has_value());
}

TEST(BitcoinFunctions, GetWalletDetails) {
    auto [name, balance] = get_wallet_details();
    EXPECT_EQ(name, "satoshi_wallet");
    EXPECT_DOUBLE_EQ(balance, 50.0);
}

TEST(BitcoinFunctions, GetTxStatus) {
    std::map<std::string, std::string> tx_pool{{"abc123", "confirmed"}};
    EXPECT_EQ(get_tx_status(tx_pool, "abc123"), "confirmed");
    EXPECT_EQ(get_tx_status(tx_pool, "def456"), "not found");
}

TEST(BitcoinFunctions, UnpackWalletInfo) {
    EXPECT_EQ(unpack_wallet_info({"alice", 75.0}),
              "Wallet alice has balance: 75.0 BTC");
}

TEST(BitcoinFunctions, CalculateSats) {
    EXPECT_EQ(calculate_sats(1.0), kBtcToSats);
    EXPECT_EQ(calculate_sats(0.5), 50'000'000LL);
    EXPECT_EQ(calculate_sats(0.00000001), 1LL);
    EXPECT_EQ(calculate_sats(0.0001), 10'000LL);
    EXPECT_EQ(calculate_sats(0.0), 0LL);
    EXPECT_EQ(calculate_sats(-1.0), -kBtcToSats);
}

TEST(BitcoinFunctions, GenerateAddress) {
    std::string address = generate_address();
    ASSERT_EQ(address.size(), static_cast<std::size_t>(32));
    EXPECT_EQ(address.substr(0, 4), "bc1q");

    std::string testnet_address = generate_address("tb1q");
    EXPECT_EQ(testnet_address.substr(0, 4), "tb1q");

    std::regex suffix_pattern("^[a-z0-9]+$");
    EXPECT_TRUE(std::regex_match(address.substr(4), suffix_pattern));
}

TEST(BitcoinFunctions, ValidateBlockHeight) {
    EXPECT_EQ(validate_block_height(0),
              (std::pair<bool, std::string>{true, "Valid block height"}));
    EXPECT_EQ(validate_block_height(700'000),
              (std::pair<bool, std::string>{true, "Valid block height"}));

    EXPECT_EQ(validate_block_height(-1),
              (std::pair<bool, std::string>{false, "Block height cannot be negative"}));
    EXPECT_EQ(validate_block_height(1'000'000),
              (std::pair<bool, std::string>{false, "Block height seems unrealistic"}));
}

TEST(BitcoinFunctions, HalvingSchedule) {
    std::map<int, long long> genesis_expected{{0, 50LL * kBtcToSats}};
    EXPECT_EQ(halving_schedule({kGenesisBlockHeight}), genesis_expected);

    std::map<int, long long> first_halving_expected{
        {209'999, 50LL * kBtcToSats},
        {210'000, 25LL * kBtcToSats},
        {210'001, 25LL * kBtcToSats},
    };
    EXPECT_EQ(halving_schedule({kFirstHalving - 1, kFirstHalving, kFirstHalving + 1}),
              first_halving_expected);

    std::map<int, long long> multi_halving_expected{
        {420'000, static_cast<long long>(12.5 * kBtcToSats)},
        {630'000, static_cast<long long>(6.25 * kBtcToSats)},
    };
    EXPECT_EQ(halving_schedule({420'000, 630'000}), multi_halving_expected);
}

TEST(BitcoinFunctions, FindUtxoWithMinValue) {
    std::vector<Utxo> utxos{
        {"abc", 10'000},
        {"def", 25'000},
        {"ghi", 5'000},
        {"jkl", 50'000},
    };

    {
        auto match = find_utxo_with_min_value(utxos, 25'000);
        ASSERT_TRUE(match.has_value());
        EXPECT_EQ(*match, (Utxo{"def", 25'000}));
    }

    {
        auto match = find_utxo_with_min_value(utxos, 15'000);
        ASSERT_TRUE(match.has_value());
        EXPECT_EQ(*match, (Utxo{"def", 25'000}));
    }

    EXPECT_FALSE(find_utxo_with_min_value(utxos, 100'000).has_value());
    EXPECT_FALSE(find_utxo_with_min_value({}, 10'000).has_value());
}
