#include "CppUE5Parse/GameTypes/UnchartedWatersOrigin/UnchartedWatersOriginAes.h"

#include <array>
#include <cstdint>
#include <cstring>

#include "CppUE5Parse/Crypto/Rijndael.h"

namespace ue5parse {

namespace {

using Rijndael::td0;
using Rijndael::td1;
using Rijndael::td2;
using Rijndael::td3;
using Rijndael::td4;

//! A transcription of the Uncharted Waters Origin decryption round, which mixes extra constants into the state.
void decryptBlock(const Rijndael::RoundKeys& key, std::span<const std::uint8_t, 16> a3, std::span<std::uint8_t, 16> a4) {
    std::uint32_t v7 = (std::uint32_t)(a3[15] ^ key[3] ^ ((a3[14] ^ ((a3[13] ^ (a3[12] << 8)) << 8)) << 8) ^ 0x3B91306A);
    std::uint32_t v8 = (std::uint32_t)(key[0] ^ a3[3] ^ ((a3[2] ^ ((a3[1] ^ (a3[0] << 8)) << 8)) << 8) ^ 0x9A236B90);
    std::uint32_t v9 = (std::uint32_t)(a3[11] ^ key[2] ^ ((a3[10] ^ ((a3[9] ^ (a3[8] << 8)) << 8)) << 8) ^ 0x5E2E079C);
    std::uint32_t v10 = (std::uint32_t)(a3[7] ^ key[1] ^ ((a3[6] ^ ((a3[5] ^ (a3[4] << 8)) << 8)) << 8) ^ 0x182FA272);
    std::uint32_t v11 = key[7] ^ td3[(std::uint8_t)(key[0] ^ a3[3]) ^ 0x90] ^ td0[(std::uint8_t)((a3[15] ^ key[3] ^ ((a3[14] ^ ((a3[13] ^ (a3[12] << 8)) << 8)) << 8) ^ 0x3B91306A) >> 24)] ^
               td2[(std::uint8_t)(v10 >> 8)] ^ td1[(std::uint8_t)((a3[11] ^ key[2] ^ ((a3[10] ^ ((a3[9] ^ (a3[8] << 8)) << 8)) << 8) ^ 0x5E2E079C) >> 16)];
    std::uint32_t v12 = key[6] ^ td3[(std::uint8_t) (a3[15] ^ key[3]) ^ 0x6A] ^ td0[v9 >> 24] ^ td1[(std::uint8_t) (v10 >> 16)] ^
               td2[(std::uint8_t) ((std::uint16_t) (key[0] ^ a3[3] ^ (std::uint16_t) ((a3[2] ^ (std::uint16_t) ((a3[1] ^ (std::uint16_t) (a3[0] << 8)) << 8)) << 8) ^ 0x6B90) >> 8)];
    std::uint32_t v13 = key[5] ^ td3[(std::uint8_t)v9] ^ td0[v10 >> 24] ^ td1[(std::uint8_t)(v8 >> 16)] ^ td2[(std::uint8_t)(v7 >> 8)];
    std::uint32_t v14 = key[4] ^ td3[(std::uint8_t)v10] ^ td0[v8 >> 24] ^ td2[(std::uint8_t)(v9 >> 8)] ^ td1[(std::uint8_t)(v7 >> 16)];
    std::uint32_t v15 = key[11] ^ td3[(std::uint8_t)v14] ^ td0[v11 >> 24] ^ td1[(std::uint8_t)(v12 >> 16)] ^ td2[(std::uint8_t)(v13 >> 8)];
    std::uint32_t v16 = key[10] ^ td3[(std::uint8_t)v11] ^ td0[v12 >> 24] ^ td1[(std::uint8_t)(v13 >> 16)] ^ td2[(std::uint8_t)(v14 >> 8)];
    std::uint32_t v17 = key[9] ^ td3[(std::uint8_t)v12] ^ td0[v13 >> 24] ^ td2[(std::uint8_t)(v11 >> 8)] ^ td1[(std::uint8_t)(v14 >> 16)];
    std::uint32_t v18 = key[8] ^ td3[(std::uint8_t)v13] ^ td0[v14 >> 24] ^ td1[(std::uint8_t)(v11 >> 16)] ^ td2[(std::uint8_t)(v12 >> 8)];
    std::uint32_t v19 = key[14] ^ td3[(std::uint8_t)v15] ^ td0[v16 >> 24] ^ td1[(std::uint8_t)(v17 >> 16)] ^ td2[(std::uint8_t)(v18 >> 8)];
    std::uint32_t v20 = key[13] ^ td3[(std::uint8_t)v16] ^ td0[v17 >> 24] ^ td2[(std::uint8_t)(v15 >> 8)] ^ td1[(std::uint8_t)(v18 >> 16)];
    std::uint32_t v21 = key[12] ^ td3[(std::uint8_t)v17] ^ td0[v18 >> 24] ^ td1[(std::uint8_t)(v15 >> 16)] ^ td2[(std::uint8_t)(v16 >> 8)];
    std::uint32_t v22 = key[15] ^ td3[(std::uint8_t)v18] ^ td0[v15 >> 24] ^ td1[(std::uint8_t)(v16 >> 16)] ^ td2[(std::uint8_t)(v17 >> 8)];
    std::uint32_t v23 = key[17] ^ td3[(std::uint8_t)v19] ^ td0[v20 >> 24] ^ td1[(std::uint8_t)(v21 >> 16)] ^ td2[(std::uint8_t)(v22 >> 8)];
    std::uint32_t v24 = key[16] ^ td3[(std::uint8_t)v20] ^ td0[v21 >> 24] ^ td2[(std::uint8_t)(v19 >> 8)] ^ td1[(std::uint8_t)(v22 >> 16)];
    std::uint32_t v25 = key[19] ^ td3[(std::uint8_t)v21] ^ td0[v22 >> 24] ^ td1[(std::uint8_t)(v19 >> 16)] ^ td2[(std::uint8_t)(v20 >> 8)];
    std::uint32_t v26 = key[18] ^ td3[(std::uint8_t)v22] ^ td0[v19 >> 24] ^ td1[(std::uint8_t)(v20 >> 16)] ^ td2[(std::uint8_t)(v21 >> 8)];
    std::uint32_t v27 = key[23] ^ td3[(std::uint8_t)v24] ^ td0[v25 >> 24] ^ td2[(std::uint8_t)(v23 >> 8)] ^ td1[(std::uint8_t)(v26 >> 16)];
    v20 = key[22] ^ td3[(std::uint8_t)v25] ^ td0[v26 >> 24] ^ td1[(std::uint8_t)(v23 >> 16)] ^ td2[(std::uint8_t)(v24 >> 8)];
    std::uint32_t v28 = key[21] ^ td3[(std::uint8_t)v26] ^ td0[v23 >> 24] ^ td1[(std::uint8_t)(v24 >> 16)] ^ td2[(std::uint8_t)(v25 >> 8)];
    std::uint32_t v29 = key[20] ^ td3[(std::uint8_t)v23] ^ td0[v24 >> 24] ^ td1[(std::uint8_t)(v25 >> 16)] ^ td2[(std::uint8_t)(v26 >> 8)];
    std::uint32_t v30 = key[26] ^ td3[(std::uint8_t)v27] ^ td0[(key[22] ^ td3[(std::uint8_t)v25] ^ td0[v26 >> 24] ^ td1[(std::uint8_t)(v23 >> 16)] ^
                td2[(std::uint8_t)(v24 >> 8)]) >> 24] ^ td1[(std::uint8_t)(v28 >> 16)] ^ td2[(std::uint8_t)((std::uint16_t)(key[20] ^
                td3[(std::uint8_t)v23] ^ td0[v24 >> 24] ^ td1[(std::uint8_t)(v25 >> 16)] ^ td2[(std::uint8_t)(v26 >> 8)]) >> 8)];
    std::uint32_t v31 = key[25] ^ td3[(std::uint8_t)v20] ^ td0[v28 >> 24] ^ td2[(std::uint8_t)((std::uint16_t)(key[23] ^ td3[(std::uint8_t)v24] ^ td0[v25 >> 24] ^
                td2[(std::uint8_t)(v23 >> 8)] ^ td1[(std::uint8_t)(v26 >> 16)]) >> 8)] ^ td1[(std::uint8_t)((key[20] ^ td3[(std::uint8_t)v23] ^
                td0[v24 >> 24] ^ td1[(std::uint8_t)(v25 >> 16)] ^ td2[(std::uint8_t)(v26 >> 8)]) >> 16)];
    std::uint32_t v32 = key[24] ^ td3[(std::uint8_t)v28] ^ td0[v29 >> 24] ^ td1[(std::uint8_t)(v27 >> 16)] ^ td2[(std::uint8_t)(v20 >> 8)];
    std::uint32_t v33 = key[27] ^ td3[(std::uint8_t)v29] ^ td0[v27 >> 24] ^ td1[(std::uint8_t)(v20 >> 16)] ^ td2[(std::uint8_t)(v28 >> 8)];
    std::uint32_t v34 = key[31] ^ td3[(std::uint8_t)v32] ^ td0[v33 >> 24] ^ td1[(std::uint8_t)(v30 >> 16)] ^ td2[(std::uint8_t)(v31 >> 8)];
    std::uint32_t v35 = key[30] ^ td3[(std::uint8_t)v33] ^ td0[v30 >> 24] ^ td1[(std::uint8_t)(v31 >> 16)] ^ td2[(std::uint8_t)(v32 >> 8)];
    std::uint32_t v36 = key[29] ^ td3[(std::uint8_t)v30] ^ td0[v31 >> 24] ^ td1[(std::uint8_t)(v32 >> 16)] ^ td2[(std::uint8_t)(v33 >> 8)];
    std::uint32_t v37 = key[28] ^ td3[(std::uint8_t)v31] ^ td0[v32 >> 24] ^ td2[(std::uint8_t)(v30 >> 8)] ^ td1[(std::uint8_t)(v33 >> 16)];
    std::uint32_t v38 = key[34] ^ td3[(std::uint8_t)v34] ^ td0[v35 >> 24] ^ td1[(std::uint8_t)(v36 >> 16)] ^ td2[(std::uint8_t)(v37 >> 8)];
    std::uint32_t v39 = key[33] ^ td3[(std::uint8_t)v35] ^ td0[v36 >> 24] ^ td2[(std::uint8_t)(v34 >> 8)] ^ td1[(std::uint8_t)(v37 >> 16)];
    std::uint32_t v40 = key[32] ^ td3[(std::uint8_t)v36] ^ td0[v37 >> 24] ^ td1[(std::uint8_t)(v34 >> 16)] ^ td2[(std::uint8_t)(v35 >> 8)];
    std::uint32_t v41 = key[35] ^ td3[(std::uint8_t)v37] ^ td0[v34 >> 24] ^ td1[(std::uint8_t)(v35 >> 16)] ^ td2[(std::uint8_t)(v36 >> 8)];
    std::uint32_t v42 = key[39] ^ td3[(std::uint8_t)v40] ^ td0[v41 >> 24] ^ td1[(std::uint8_t)(v38 >> 16)] ^ td2[(std::uint8_t)(v39 >> 8)];
    std::uint32_t v43 = key[37] ^ td3[(std::uint8_t)v38] ^ td0[v39 >> 24] ^ td1[(std::uint8_t)(v40 >> 16)] ^ td2[(std::uint8_t)(v41 >> 8)];
    std::uint32_t v44 = key[38] ^ td3[(std::uint8_t)v41] ^ td0[v38 >> 24] ^ td1[(std::uint8_t)(v39 >> 16)] ^ td2[(std::uint8_t)(v40 >> 8)];
    std::uint32_t v45 = key[36] ^ td3[(std::uint8_t)v39] ^ td0[v40 >> 24] ^ td2[(std::uint8_t)(v38 >> 8)] ^ td1[(std::uint8_t)(v41 >> 16)];
    {
        std::uint32_t v46 = key[39] ^ td3[(std::uint8_t)v40] ^ td0[v41 >> 24] ^ td1[(std::uint8_t)(v38 >> 16)] ^ td2[(std::uint8_t)(v39 >> 8)];
        std::uint32_t v47 = v45 >> 24;
        std::uint32_t v48 = key[43] ^ td3[(std::uint8_t)v45] ^ td0[v42 >> 24] ^ td1[(std::uint8_t)(v44 >> 16)] ^ td2[(std::uint8_t)(v43 >> 8)];
        std::uint32_t v49 = key[42] ^ td3[(std::uint8_t)v42] ^ td0[v44 >> 24] ^ td1[(std::uint8_t)(v43 >> 16)] ^ td2[(std::uint8_t)(v45 >> 8)];
        std::uint32_t v50 = key[41] ^ td3[(std::uint8_t)v44] ^ td0[v43 >> 24] ^ td2[(std::uint8_t)(v46 >> 8)] ^ td1[(std::uint8_t)(v45 >> 16)];
        std::uint32_t v51 = key[40] ^ td3[(std::uint8_t)v43] ^ td0[v47] ^ td1[(std::uint8_t)(v46 >> 16)] ^ td2[(std::uint8_t)(v44 >> 8)];
        v45 = key[44] ^ td3[(std::uint8_t)v50] ^ td0[(key[40] ^ td3[(std::uint8_t)v43] ^ td0[v47] ^ td1[(std::uint8_t)(v46 >> 16)] ^
                td2[(std::uint8_t)(v44 >> 8)]) >> 24] ^ td1[(std::uint8_t)(v48 >> 16)] ^ td2[(std::uint8_t)(v49 >> 8)];
        v43 = key[45] ^ td3[(std::uint8_t)v49] ^ td0[v50 >> 24] ^ td2[(std::uint8_t)(v48 >> 8)] ^ td1[(std::uint8_t)(v51 >> 16)];
        v44 = key[46] ^ td3[(std::uint8_t)v48] ^ td0[v49 >> 24] ^ td1[(std::uint8_t)(v50 >> 16)] ^ td2[(std::uint8_t)(v51 >> 8)];
        v42 = key[47] ^ td3[(std::uint8_t)v51] ^ td0[v48 >> 24] ^ td1[(std::uint8_t)(v49 >> 16)] ^ td2[(std::uint8_t)(v50 >> 8)];
        {
            std::uint32_t v52 = v42;
            std::uint32_t v53 = v45 >> 24;
            std::uint32_t v54 = key[51] ^ td3[(std::uint8_t)v45] ^ td0[v42 >> 24] ^ td1[(std::uint8_t)(v44 >> 16)] ^ td2[(std::uint8_t)(v43 >> 8)];
            std::uint32_t v55 = key[50] ^ td3[(std::uint8_t)v42] ^ td0[v44 >> 24] ^ td1[(std::uint8_t)(v43 >> 16)] ^ td2[(std::uint8_t)(v45 >> 8)];
            std::uint32_t v56 = key[49] ^ td3[(std::uint8_t)v44] ^ td0[v43 >> 24] ^ td2[(std::uint8_t)(v52 >> 8)] ^ td1[(std::uint8_t)(v45 >> 16)];
            std::uint32_t v57 = key[48] ^ td3[(std::uint8_t)v43] ^ td0[v53] ^ td1[(std::uint8_t)(v52 >> 16)] ^ td2[(std::uint8_t)(v44 >> 8)];
            v45 = key[52] ^ td3[(std::uint8_t)v56] ^ td0[(key[48] ^ td3[(std::uint8_t)v43] ^ td0[v53] ^ td1[(std::uint8_t)(v52 >> 16)] ^
                    td2[(std::uint8_t)(v44 >> 8)]) >> 24] ^ td1[(std::uint8_t)(v54 >> 16)] ^ td2[(std::uint8_t)(v55 >> 8)];
            v43 = key[53] ^ td3[(std::uint8_t)v55] ^ td0[v56 >> 24] ^ td2[(std::uint8_t)(v54 >> 8)] ^ td1[(std::uint8_t)(v57 >> 16)];
            v44 = key[54] ^ td3[(std::uint8_t)v54] ^ td0[v55 >> 24] ^ td1[(std::uint8_t)(v56 >> 16)] ^ td2[(std::uint8_t)(v57 >> 8)];
            v42 = key[55] ^ td3[(std::uint8_t)v57] ^ td0[v54 >> 24] ^ td1[(std::uint8_t)(v55 >> 16)] ^ td2[(std::uint8_t)(v56 >> 8)];
        }
    }
    std::uint32_t v58 = v44;
    std::uint32_t v60 = key[56] ^ (std::uint8_t)(td4[(std::uint8_t)v43]) ^ td4[v45 >> 24] & 0xFF000000 ^ td4[(std::uint8_t)(v44 >> 8)] & 0xFF00 ^ td4[(std::uint8_t)(v42 >> 16)] & 0xFF0000;
    a4[3] = (std::uint8_t)v60;
    a4[0] = (std::uint8_t)((v60 >> 24) & 0xFF);
    a4[1] = (std::uint8_t)(v60 >> 16);
    a4[2] = (std::uint8_t)(v60 >> 8);
    std::uint32_t v61 = key[57] ^ (std::uint8_t)(td4[(std::uint8_t)v58]) ^ td4[v43 >> 24] & 0xFF000000 ^ td4[(std::uint8_t)(v45 >> 16)] & 0xFF0000 ^ td4[(std::uint8_t)(v42 >> 8)] & 0xFF00;
    a4[7] = (std::uint8_t)(key[57] ^ (std::uint8_t)(td4[(std::uint8_t)v58]));
    a4[4] = (std::uint8_t)((v61 >> 24) & 0xFF);
    a4[5] = (std::uint8_t)(v61 >> 16);
    a4[6] = (std::uint8_t)(v61 >> 8);
    std::uint32_t v62 = key[58] ^ (std::uint8_t)(td4[(std::uint8_t)v42]) ^ td4[v58 >> 24] & 0xFF000000 ^ td4[(std::uint8_t)(v45 >> 8)] & 0xFF00 ^ td4[(std::uint8_t)(v43 >> 16)] & 0xFF0000;
    a4[11] =  (std::uint8_t)(key[58] ^ (std::uint8_t)(td4[(std::uint8_t)v42]));
    a4[8] = (std::uint8_t)((v62 >> 24) & 0xFF);
    a4[9] = (std::uint8_t)(v62 >> 16);
    a4[10] = (std::uint8_t)(v62 >> 8);
    std::uint32_t v63 = key[59] ^ (std::uint8_t)(td4[(std::uint8_t)v45]) ^ td4[v42 >> 24] & 0xFF000000 ^ td4[(std::uint8_t)(v58 >> 16)] & 0xFF0000 ^ td4[(std::uint8_t)(v43 >> 8)] & 0xFF00;
    a4[15] =  (std::uint8_t)(key[59] ^ (std::uint8_t)(td4[(std::uint8_t)v45]));
    a4[12] = (std::uint8_t)((v63 >> 24) & 0xFF);
    a4[13] = (std::uint8_t)(v63 >> 16);
    a4[14] = (std::uint8_t)(v63 >> 8);
}

}

void decryptUnchartedWatersOrigin(std::span<std::byte> data, const AesKey& key) {
    Rijndael::RoundKeys roundKeys = Rijndael::setupDecrypt256(key, 0);
    for (std::size_t offset = 0; offset + 16 <= data.size(); offset += 16) {
        std::array<std::uint8_t, 16> input{};
        std::memcpy(input.data(), data.data() + offset, 16);
        decryptBlock(roundKeys, input, std::span<std::uint8_t, 16>(reinterpret_cast<std::uint8_t*>(data.data() + offset), 16));
    }
}

}
