#include "CppUE5Parse/GameTypes/Spectre/SpectreAes.h"

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

//! A transcription of the Spectre Divide decryption round, which the game obfuscates.
void decryptBlock(const Rijndael::RoundKeys& rk, std::span<const std::uint8_t, 16> a3, std::span<std::uint8_t, 16> a4) {
    std::uint32_t v24 = (std::uint32_t)(rk[0] ^ a3[7] ^ ((a3[6] ^ ((a3[5] ^ (a3[4] << 8)) << 8)) << 8));
    std::uint32_t v25 = (std::uint32_t)(rk[3] ^ a3[15] ^ ((a3[14] ^ ((a3[13] ^ (a3[12] << 8)) << 8)) << 8));
    std::uint32_t v26 = (std::uint32_t)(rk[2] ^ a3[3] ^ ((a3[2] ^ ((a3[1] ^ (a3[0] << 8)) << 8)) << 8));
    std::uint32_t v27 = rk[7] ^ td3[(std::uint8_t)(rk[2] ^ a3[3])] ^ td0[(std::uint32_t)(a3[11] ^ ((a3[10] ^ ((a3[9] ^ (a3[8] << 8)) << 8)) << 8)) >> 24] ^ td2[(std::uint8_t)(v25 >> 8)] ^ td1[(std::uint8_t)(v24 >> 16)];
    std::uint32_t v28 = rk[6] ^ td3[a3[11]] ^ td0[v24 >> 24] ^ td2[(std::uint8_t)((rk[2] ^ a3[3] ^ ((a3[2] ^ ((a3[1] ^ (a3[0] << 8)) << 8)) << 8)) >> 8)] ^ td1[(std::uint8_t)(v25 >> 16)];
    std::uint32_t v29 = rk[5] ^ td3[(std::uint8_t)v24] ^ td0[v25 >> 24] ^ td1[(std::uint8_t)(v26 >> 16)] ^ td2[a3[10]];
    std::uint32_t v30 = rk[4] ^ td3[(std::uint8_t)v25] ^ td0[v26 >> 24] ^ td1[(std::uint8_t)((a3[11] ^ ((a3[10] ^ ((a3[9] ^ (a3[8] << 8)) << 8)) << 8)) >> 16)] ^ td2[(std::uint8_t)(v24 >> 8)];
    std::uint32_t v31 = rk[11] ^ td3[(std::uint8_t)v30] ^ td0[v27 >> 24] ^ td1[(std::uint8_t)(v28 >> 16)] ^ td2[(std::uint8_t)(v29 >> 8)];
    std::uint32_t v32 = rk[10] ^ td3[(std::uint8_t)v27] ^ td0[v28 >> 24] ^ td1[(std::uint8_t)(v29 >> 16)] ^ td2[(std::uint8_t)(v30 >> 8)];
    std::uint32_t v33 = rk[9]  ^ td3[(std::uint8_t)v28] ^ td0[v29 >> 24] ^ td2[(std::uint8_t)(v27 >> 8)] ^ td1[(std::uint8_t)(v30 >> 16)];
    std::uint32_t v34 = rk[8]  ^ td3[(std::uint8_t)v29] ^ td0[v30 >> 24] ^ td1[(std::uint8_t)(v27 >> 16)] ^ td2[(std::uint8_t)(v28 >> 8)];
    std::uint32_t v35 = rk[15] ^ td3[(std::uint8_t)v34] ^ td0[v31 >> 24] ^ td1[(std::uint8_t)(v32 >> 16)] ^ td2[(std::uint8_t)(v33 >> 8)];
    std::uint32_t v36 = rk[14] ^ td3[(std::uint8_t)v31] ^ td0[v32 >> 24] ^ td1[(std::uint8_t)(v33 >> 16)] ^ td2[(std::uint8_t)(v34 >> 8)];
    std::uint32_t v37 = rk[13] ^ td3[(std::uint8_t)v32] ^ td0[v33 >> 24] ^ td2[(std::uint8_t)(v31 >> 8)] ^ td1[(std::uint8_t)(v34 >> 16)];
    std::uint32_t v38 = rk[12] ^ td3[(std::uint8_t)v33] ^ td0[v34 >> 24] ^ td1[(std::uint8_t)(v31 >> 16)] ^ td2[(std::uint8_t)(v32 >> 8)];
    std::uint32_t v39 = rk[19] ^ td3[(std::uint8_t)v38] ^ td0[v35 >> 24] ^ td1[(std::uint8_t)(v36 >> 16)] ^ td2[(std::uint8_t)(v37 >> 8)];
    std::uint32_t v40 = rk[18] ^ td3[(std::uint8_t)v35] ^ td0[v36 >> 24] ^ td1[(std::uint8_t)(v37 >> 16)] ^ td2[(std::uint8_t)(v38 >> 8)];
    std::uint32_t v41 = rk[17] ^ td3[(std::uint8_t)v36] ^ td0[v37 >> 24] ^ td2[(std::uint8_t)(v35 >> 8)] ^ td1[(std::uint8_t)(v38 >> 16)];
    std::uint32_t v42 = rk[16] ^ td3[(std::uint8_t)v37] ^ td0[v38 >> 24] ^ td1[(std::uint8_t)(v35 >> 16)] ^ td2[(std::uint8_t)(v36 >> 8)];
    std::uint32_t v43 = rk[23] ^ td3[(std::uint8_t)v42] ^ td0[v39 >> 24] ^ td1[(std::uint8_t)(v40 >> 16)] ^ td2[(std::uint8_t)(v41 >> 8)];
    std::uint32_t v44 = rk[22] ^ td3[(std::uint8_t)v39] ^ td0[v40 >> 24] ^ td1[(std::uint8_t)(v41 >> 16)] ^ td2[(std::uint8_t)(v42 >> 8)];
    std::uint32_t v45 = rk[21] ^ td3[(std::uint8_t)v40] ^ td0[v41 >> 24] ^ td2[(std::uint8_t)(v39 >> 8)] ^ td1[(std::uint8_t)(v42 >> 16)];
    std::uint32_t v46 = rk[20] ^ td3[(std::uint8_t)v41] ^ td0[v42 >> 24] ^ td1[(std::uint8_t)(v39 >> 16)] ^ td2[(std::uint8_t)(v40 >> 8)];
    std::uint32_t v47 = rk[27] ^ td3[(std::uint8_t)v46] ^ td0[v43 >> 24] ^ td1[(std::uint8_t)((rk[22] ^ td3[(std::uint8_t)v39] ^ td0[v40 >> 24] ^ td1[(std::uint8_t)(v41 >> 16)] ^ td2[(std::uint8_t)(v42 >> 8)]) >> 16)] ^ td2[(std::uint8_t)(v45 >> 8)];
    std::uint32_t v48 = rk[26] ^ td3[(std::uint8_t)v43] ^ td0[v44 >> 24] ^ td1[(std::uint8_t)(v45 >> 16)] ^ td2[(std::uint8_t)(v46 >> 8)];
    std::uint32_t v49 = rk[25] ^ td3[(std::uint8_t)v44] ^ td0[v45 >> 24] ^ td2[(std::uint8_t)(v43 >> 8)] ^ td1[(std::uint8_t)(v46 >> 16)];
    std::uint32_t v50 = rk[24] ^ td3[(std::uint8_t)v45] ^ td0[v46 >> 24] ^ td1[(std::uint8_t)(v43 >> 16)] ^ td2[(std::uint8_t)(v44 >> 8)];
    std::uint32_t v51 = rk[31] ^ td3[(std::uint8_t)v50] ^ td0[v47 >> 24] ^ td1[(std::uint8_t)(v48 >> 16)] ^ td2[(std::uint8_t)(v49 >> 8)];
    std::uint32_t v52 = rk[30] ^ td3[(std::uint8_t)v47] ^ td0[v48 >> 24] ^ td1[(std::uint8_t)(v49 >> 16)] ^ td2[(std::uint8_t)(v50 >> 8)];
    std::uint32_t v53 = rk[29] ^ td3[(std::uint8_t)v48] ^ td0[v49 >> 24] ^ td2[(std::uint8_t)(v47 >> 8)] ^ td1[(std::uint8_t)(v50 >> 16)];
    std::uint32_t v54 = rk[28] ^ td3[(std::uint8_t)v49] ^ td0[v50 >> 24] ^ td1[(std::uint8_t)(v47 >> 16)] ^ td2[(std::uint8_t)(v48 >> 8)];
    std::uint32_t v55 = rk[35] ^ td3[(std::uint8_t)v54] ^ td0[v51 >> 24] ^ td1[(std::uint8_t)(v52 >> 16)] ^ td2[(std::uint8_t)(v53 >> 8)];
    std::uint32_t v56 = rk[34] ^ td3[(std::uint8_t)v51] ^ td0[v52 >> 24] ^ td1[(std::uint8_t)(v53 >> 16)] ^ td2[(std::uint8_t)(v54 >> 8)];
    std::uint32_t v57 = rk[33] ^ td3[(std::uint8_t)v52] ^ td0[v53 >> 24] ^ td2[(std::uint8_t)(v51 >> 8)] ^ td1[(std::uint8_t)(v54 >> 16)];
    std::uint32_t v58 = rk[32] ^ td3[(std::uint8_t)v53] ^ td0[v54 >> 24] ^ td1[(std::uint8_t)(v51 >> 16)] ^ td2[(std::uint8_t)(v52 >> 8)];
    std::uint32_t v59 = rk[36] ^ td3[(std::uint8_t)v57] ^ td0[v58 >> 24] ^ td1[(std::uint8_t)(v55 >> 16)] ^ td2[(std::uint8_t)(v56 >> 8)];
    std::uint32_t v60 = rk[37] ^ td3[(std::uint8_t)v56] ^ td0[v57 >> 24] ^ td2[(std::uint8_t)(v55 >> 8)] ^ td1[(std::uint8_t)(v58 >> 16)];
    std::uint32_t v61 = rk[38] ^ td3[(std::uint8_t)v55] ^ td0[v56 >> 24] ^ td1[(std::uint8_t)(v57 >> 16)] ^ td2[(std::uint8_t)(v58 >> 8)];
    std::uint32_t v62 = rk[39] ^ td3[(std::uint8_t)v58] ^ td0[v55 >> 24] ^ td1[(std::uint8_t)(v56 >> 16)] ^ td2[(std::uint8_t)(v57 >> 8)];
    std::uint32_t v63 = rk[43] ^ td3[(std::uint8_t)v59] ^ td0[v62 >> 24] ^ td1[(std::uint8_t)(v61 >> 16)] ^ td2[(std::uint8_t)(v60 >> 8)];
    std::uint32_t v64 = rk[42] ^ td3[(std::uint8_t)v62] ^ td0[v61 >> 24] ^ td1[(std::uint8_t)(v60 >> 16)] ^ td2[(std::uint8_t)(v59 >> 8)];
    std::uint32_t v65 = rk[41] ^ td3[(std::uint8_t)v61] ^ td0[v60 >> 24] ^ td2[(std::uint8_t)(v62 >> 8)] ^ td1[(std::uint8_t)(v59 >> 16)];
    std::uint32_t v66 = rk[40] ^ td3[(std::uint8_t)v60] ^ td0[v59 >> 24] ^ td1[(std::uint8_t)(v62 >> 16)] ^ td2[(std::uint8_t)(v61 >> 8)];
    v59 = rk[44] ^ td3[(std::uint8_t)v65] ^ td0[v66 >> 24] ^ td1[(std::uint8_t)(v63 >> 16)] ^ td2[(std::uint8_t)(v64 >> 8)];
    v60 = rk[45] ^ td3[(std::uint8_t)v64] ^ td0[v65 >> 24] ^ td2[(std::uint8_t)(v63 >> 8)] ^ td1[(std::uint8_t)(v66 >> 16)];
    v61 = rk[46] ^ td3[(std::uint8_t)v63] ^ td0[v64 >> 24] ^ td1[(std::uint8_t)(v65 >> 16)] ^ td2[(std::uint8_t)(v66 >> 8)];
    v62 = rk[47] ^ td3[(std::uint8_t)v66] ^ td0[v63 >> 24] ^ td1[(std::uint8_t)(v64 >> 16)] ^ td2[(std::uint8_t)(v65 >> 8)];
    std::uint32_t v67 = rk[51] ^ td3[(std::uint8_t)v59] ^ td0[v62 >> 24] ^ td1[(std::uint8_t)(v61 >> 16)] ^ td2[(std::uint8_t)(v60 >> 8)];
    std::uint32_t v68 = rk[50] ^ td3[(std::uint8_t)v62] ^ td0[v61 >> 24] ^ td1[(std::uint8_t)(v60 >> 16)] ^ td2[(std::uint8_t)(v59 >> 8)];
    std::uint32_t v69 = rk[49] ^ td3[(std::uint8_t)v61] ^ td0[v60 >> 24] ^ td2[(std::uint8_t)(v62 >> 8)] ^ td1[(std::uint8_t)(v59 >> 16)];
    std::uint32_t v70 = rk[48] ^ td3[(std::uint8_t)v60] ^ td0[v59 >> 24] ^ td1[(std::uint8_t)(v62 >> 16)] ^ td2[(std::uint8_t)(v61 >> 8)];
    v59 = rk[52] ^ td3[(std::uint8_t)v69] ^ td0[v70 >> 24] ^ td1[(std::uint8_t)(v67 >> 16)] ^ td2[(std::uint8_t)(v68 >> 8)];
    v60 = rk[53] ^ td3[(std::uint8_t)v68] ^ td0[v69 >> 24] ^ td2[(std::uint8_t)(v67 >> 8)] ^ td1[(std::uint8_t)(v70 >> 16)];
    v61 = rk[54] ^ td3[(std::uint8_t)v67] ^ td0[v68 >> 24] ^ td1[(std::uint8_t)(v69 >> 16)] ^ td2[(std::uint8_t)(v70 >> 8)];
    v62 = rk[55] ^ td3[(std::uint8_t)v70] ^ td0[v67 >> 24] ^ td1[(std::uint8_t)(v68 >> 16)] ^ td2[(std::uint8_t)(v69 >> 8)];
    std::uint32_t v22 = rk[57];
    std::uint32_t v80 = rk[59];
    std::uint32_t v71 = v61;
    std::uint32_t v72 = v80 ^ (std::uint8_t)td4[(std::uint8_t)v60] ^ td4[v59 >> 24] & 0xFF000000 ^ td4[(std::uint8_t)(v61 >> 8)] & 0xFF00 ^ td4[(std::uint8_t)(v62 >> 16)] & 0xFF0000;
    a4[3] = (std::uint8_t)v72;
    a4[0] = (std::uint8_t)((v72 >> 24) & 0xFF);
    a4[1] = (std::uint8_t)(v72 >> 16);
    a4[2] = (std::uint8_t)(v72 >> 8);
    std::uint32_t v73 = v22 ^ (std::uint8_t)td4[(std::uint8_t)v71] ^ td4[v60 >> 24] & 0xFF000000 ^ td4[(std::uint8_t)(v62 >> 8)] & 0xFF00 ^ td4[(std::uint8_t)(v59 >> 16)] & 0xFF0000;
    a4[7] = (std::uint8_t)((v22 & 0xFF) ^ (td4[(std::uint8_t)v71] & 0xFF));
    a4[4] = (std::uint8_t)((v73 >> 24) & 0xFF);
    a4[5] = (std::uint8_t)(v73 >> 16);
    a4[6] = (std::uint8_t)(v73 >> 8);
    std::uint32_t v74 = (std::uint8_t)td4[(std::uint8_t)v62] ^ td4[v71 >> 24] & 0xFF000000 ^ td4[(std::uint8_t)(v59 >> 8)] & 0xFF00 ^ td4[(std::uint8_t)(v60 >> 16)] & 0xFF0000;
    a4[8] = (std::uint8_t)((v74 >> 24) & 0xFF);
    a4[9] = (std::uint8_t)(v74 >> 16);
    a4[10] = (std::uint8_t)(v74 >> 8);
    a4[11] = (std::uint8_t)v74;
    std::uint32_t v75 = td4[v62 >> 24] & 0xFF000000 ^ td4[(std::uint8_t)(v60 >> 8)] & 0xFF00 ^ td4[(std::uint8_t)(v71 >> 16)] & 0xFF0000;
    std::uint32_t v76 = (std::uint8_t)v59;
    std::uint32_t v77 = v22 ^ (std::uint8_t)td4[v76] ^ v75;
    a4[15] = (std::uint8_t)v77;
    a4[12] = (std::uint8_t)((v77 >> 24) & 0xFF);
    a4[13] = (std::uint8_t)(v77 >> 16);
    a4[14] = (std::uint8_t)(v77 >> 8);
}

}

void decryptSpectre(std::span<std::byte> data, const AesKey& key) {
    Rijndael::RoundKeys roundKeys = Rijndael::setupDecrypt256(key, 0x63636363);
    for (std::size_t offset = 0; offset + 16 <= data.size(); offset += 16) {
        std::array<std::uint8_t, 16> input{};
        std::memcpy(input.data(), data.data() + offset, 16);
        decryptBlock(roundKeys, input, std::span<std::uint8_t, 16>(reinterpret_cast<std::uint8_t*>(data.data() + offset), 16));
    }
}

}
