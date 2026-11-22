#include "CppUE5Parse/GameTypes/Splitgate2/Splitgate2Aes.h"

#include "CppUE5Parse/Crypto/Rijndael.h"

namespace ue5parse {

//! Splitgate 2 and Empulse build the AES-256 decryption round keys without the inverse-mixed key of round 7.
void decryptSplitgate2(std::span<std::byte> data, const AesKey& key) {
    std::array<Rijndael::Block, 15> expanded = Rijndael::expandKey256(key);
    std::array<Rijndael::Block, 14> roundKeys{};
    std::size_t count = 0;
    roundKeys[count++] = expanded[14];
    for (std::size_t round = 13; round >= 1; round--)
        if (round != 7)
            roundKeys[count++] = Rijndael::inverseMixColumns(expanded[round]);
    roundKeys[count] = expanded[0];
    Rijndael::decryptWithRoundKeys(data, roundKeys);
}

}
