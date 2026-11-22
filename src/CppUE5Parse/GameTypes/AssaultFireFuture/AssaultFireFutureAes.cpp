#include "CppUE5Parse/GameTypes/AssaultFireFuture/AssaultFireFutureAes.h"

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

constexpr std::array<std::uint8_t, 16> xorKey = {0xB3, 0x9C, 0x26, 0xA1, 0x0E, 0x5A, 0x3A, 0xD3, 0x19, 0xD1, 0x62, 0x28, 0x50, 0x9C, 0xB5, 0x2F};

//! The game decrypts with these fixed round keys and does not use the AES key that it is given.
constexpr std::array<std::uint32_t, 60> rk = {
    0xA1E3936Du, 0x91294977u, 0xCF8CD4BFu, 0xA8DE1C1Eu,
    0x334AE9ADu, 0x46879115u, 0x00DA2A31u, 0xF1C1210Eu,
    0xE5A29A3Cu, 0xAB7635D2u, 0x3BD3BFF9u, 0x0D161A5Du,
    0x99D890B3u, 0x75CD78B8u, 0x465DBB24u, 0xF11B0B3Fu,
    0xEBFFFC58u, 0x4ED4AFEEu, 0x90A58A2Bu, 0x36C5A5A4u,
    0x405B595Fu, 0xEC15E80Bu, 0x3390C39Cu, 0xB746B01Bu,
    0x3E28F779u, 0xA52B53B6u, 0xDE7125C5u, 0xA6602F8Fu,
    0x8A4B7D9Fu, 0xAC4EB154u, 0xDF852B97u, 0x84D67387u,
    0x9DE56208u, 0x9B03A4CFu, 0x7B5A7673u, 0x78110A4Au,
    0xF9BEE30Fu, 0x2605CCCBu, 0x73CB9AC3u, 0x5B535810u,
    0x71DF3DB5u, 0x06E6C6C7u, 0xE059D2BCu, 0x034B7C39u,
    0x76E21A1Cu, 0xDFBB2FC4u, 0x55CE5608u, 0x2898C2D3u,
    0x3956F631u, 0x7739FB72u, 0xE6BF147Bu, 0xE312AE85u,
    0x80F4F081u, 0xA95935D8u, 0x8A7579CCu, 0x7D5694DBu,
    0xDB3A500Du, 0x63C4FE36u, 0x4EA5C9D3u, 0xA26FDEFFu,
};

void decryptBlock(std::span<const std::uint8_t, 16> a3, std::span<std::uint8_t, 16> a4) {
    std::uint32_t ecx7 = (std::uint32_t)((((((a3[0] << 8) ^ a3[1]) << 8) ^ a3[2]) << 8) ^ a3[3]) ^ rk[0];
    std::uint32_t ecx15 = (std::uint32_t) ((((((a3[4] << 8) ^ a3[5]) << 8) ^ a3[6]) << 8) ^ a3[7]) ^ rk[1];
    std::uint32_t ecx16 = rk[2];
    std::uint32_t edx = (std::uint16_t) ecx16;
    std::uint32_t ebx4 = (std::uint32_t)(((a3[8] << 8) ^ a3[9]) << 16) ^ ecx16;
    std::uint32_t ecx24 = (std::uint32_t)((((((a3[0x0C] << 8) ^ a3[0x0D]) << 8) ^ a3[0x0E]) << 8) ^ a3[0x0F]) ^ rk[3];
    std::uint32_t ecx32 = td2[(a3[0x0A] ^ (edx >> 8)) & 0xFF] ^ td1[(std::uint8_t) (ecx24 >> 16)] ^ td0[(std::uint8_t) (ecx7 >> 24)] ^ td3[(std::uint8_t) ecx15] ^ rk[4];
    std::uint32_t eax30 = td3[(a3[0x0B] ^ (std::uint8_t) edx) & 0xFF] ^ td2[(std::uint8_t) (ecx24 >> 8)] ^ td1[(std::uint8_t) (ecx7 >> 16)] ^ td0[(std::uint8_t) (ecx15 >> 24)] ^ rk[5];
    std::uint32_t ecx43 = td1[(std::uint8_t) (ecx15 >> 16)] ^ td2[(std::uint8_t) (ecx7 >> 8)] ^ td0[(std::uint8_t) (ebx4 >> 24)] ^ td3[(std::uint8_t) ecx24] ^ rk[6];
    std::uint32_t edx6 = td1[(std::uint8_t) (ebx4 >> 16)] ^ td2[(std::uint8_t) (ecx15 >> 8)] ^ td0[(std::uint8_t) (ecx24 >> 24)] ^ td3[(std::uint8_t) ecx7] ^ rk[7];
    std::uint32_t ecx50 = td1[(std::uint8_t) (edx6 >> 16)] ^ td2[(std::uint8_t) (ecx43 >> 8)] ^ td0[(std::uint8_t) (ecx32 >> 24)] ^ td3[(std::uint8_t) eax30] ^ rk[8];
    std::uint32_t ecx56 = td2[(std::uint8_t) (edx6 >> 8)] ^ td1[(std::uint8_t) (ecx32 >> 16)] ^ td0[(std::uint8_t) (eax30 >> 24)] ^ td3[(std::uint8_t) ecx43] ^ rk[9];
    std::uint32_t ecx62 = td1[(std::uint8_t) (eax30 >> 16)] ^ td2[(std::uint8_t) (ecx32 >> 8)] ^ td0[(std::uint8_t) (ecx43 >> 24)] ^ td3[(std::uint8_t) edx6] ^ rk[10];
    std::uint32_t ebx12 = td1[(std::uint8_t) (ecx43 >> 16)] ^ td2[(std::uint8_t) (eax30 >> 8)] ^ td0[(std::uint8_t) (edx6 >> 24)] ^ td3[(std::uint8_t) ecx32] ^ rk[11];
    std::uint32_t ecx69 = td1[(std::uint8_t) (ebx12 >> 16)] ^ td2[(std::uint8_t) (ecx62 >> 8)] ^ td0[(std::uint8_t) (ecx50 >> 24)] ^ td3[(std::uint8_t) ecx56] ^ rk[12];
    std::uint32_t eax95 = td2[(std::uint8_t) (ebx12 >> 8)] ^ td1[(std::uint8_t) (ecx50 >> 16)] ^ td0[(std::uint8_t) (ecx56 >> 24)] ^ td3[(std::uint8_t) ecx62] ^ rk[13];
    std::uint32_t ecx78 = td1[(std::uint8_t) (ecx56 >> 16)] ^ td2[(std::uint8_t) (ecx50 >> 8)] ^ td0[(std::uint8_t) (ecx62 >> 24)] ^ td3[(std::uint8_t) ebx12] ^ rk[14];
    std::uint32_t edx14 = td1[(std::uint8_t) (ecx62 >> 16)] ^ td2[(std::uint8_t) (ecx56 >> 8)] ^ td0[(std::uint8_t) (ebx12 >> 24)] ^ td3[(std::uint8_t) ecx50] ^ rk[15];
    std::uint32_t ecx85 = td1[(std::uint8_t) (edx14 >> 16)] ^ td2[(std::uint8_t) (ecx78 >> 8)] ^ td0[(std::uint8_t) (ecx69 >> 24)] ^ td3[(std::uint8_t) eax95] ^ rk[16];
    std::uint32_t eax127 = td2[(std::uint8_t) (edx14 >> 8)] ^ td1[(std::uint8_t) (ecx69 >> 16)] ^ td0[(std::uint8_t) (eax95 >> 24)] ^ td3[(std::uint8_t) ecx78] ^ rk[17];
    std::uint32_t ecx94 = td1[(std::uint8_t) (eax95 >> 16)] ^ td2[(std::uint8_t) (ecx69 >> 8)] ^ td0[(std::uint8_t) (ecx78 >> 24)] ^ td3[(std::uint8_t) edx14] ^ rk[18];
    std::uint32_t ebx20 = td1[(std::uint8_t) (ecx78 >> 16)] ^ td2[(std::uint8_t) (eax95 >> 8)] ^ td0[(std::uint8_t) (edx14 >> 24)] ^ td3[(std::uint8_t) ecx69] ^ rk[19];
    std::uint32_t ecx101 = td1[(std::uint8_t) (ebx20 >> 16)] ^ td2[(std::uint8_t) (ecx94 >> 8)] ^ td0[(std::uint8_t) (ecx85 >> 24)] ^ td3[(std::uint8_t) eax127] ^ rk[20];
    std::uint32_t eax159 = td2[(std::uint8_t) (ebx20 >> 8)] ^ td1[(std::uint8_t) (ecx85 >> 16)] ^ td0[(std::uint8_t) (eax127 >> 24)] ^ td3[(std::uint8_t) ecx94] ^ rk[21];
    std::uint32_t ecx110 = td1[(std::uint8_t) (eax127 >> 16)] ^ td2[(std::uint8_t) (ecx85 >> 8)] ^ td0[(std::uint8_t) (ecx94 >> 24)] ^ td3[(std::uint8_t) ebx20] ^ rk[22];
    std::uint32_t edx22 = td1[(std::uint8_t) (ecx94 >> 16)] ^ td2[(std::uint8_t) (eax127 >> 8)] ^ td0[(std::uint8_t) (ebx20 >> 24)] ^ td3[(std::uint8_t) ecx85] ^ rk[23];
    std::uint32_t ecx117 = td1[(std::uint8_t) (edx22 >> 16)] ^ td2[(std::uint8_t) (ecx110 >> 8)] ^ td0[(std::uint8_t) (ecx101 >> 24)] ^ td3[(std::uint8_t) eax159] ^ rk[24];
    std::uint32_t eax191 = td2[(std::uint8_t) (edx22 >> 8)] ^ td1[(std::uint8_t) (ecx101 >> 16)] ^ td0[(std::uint8_t) (eax159 >> 24)] ^ td3[(std::uint8_t) ecx110] ^ rk[25];
    std::uint32_t ecx126 = td1[(std::uint8_t) (eax159 >> 16)] ^ td2[(std::uint8_t) (ecx101 >> 8)] ^ td0[(std::uint8_t) (ecx110 >> 24)] ^ td3[(std::uint8_t) edx22] ^ rk[26];
    std::uint32_t ebx28 = td1[(std::uint8_t) (ecx110 >> 16)] ^ td2[(std::uint8_t) (eax159 >> 8)] ^ td0[(std::uint8_t) (edx22 >> 24)] ^ td3[(std::uint8_t) ecx101] ^ rk[27];
    std::uint32_t ecx133 = td1[(std::uint8_t) (ebx28 >> 16)] ^ td2[(std::uint8_t) (ecx126 >> 8)] ^ td0[(std::uint8_t) (ecx117 >> 24)] ^ td3[(std::uint8_t) eax191] ^ rk[28];
    std::uint32_t eax223 = td2[(std::uint8_t) (ebx28 >> 8)] ^ td1[(std::uint8_t) (ecx117 >> 16)] ^ td0[(std::uint8_t) (eax191 >> 24)] ^ td3[(std::uint8_t) ecx126] ^ rk[29];
    std::uint32_t ecx142 = td1[(std::uint8_t) (eax191 >> 16)] ^ td2[(std::uint8_t) (ecx117 >> 8)] ^ td0[(std::uint8_t) (ecx126 >> 24)] ^ td3[(std::uint8_t) ebx28] ^ rk[30];
    std::uint32_t edx30 = td1[(std::uint8_t) (ecx126 >> 16)] ^ td2[(std::uint8_t) (eax191 >> 8)] ^ td0[(std::uint8_t) (ebx28 >> 24)] ^ td3[(std::uint8_t) ecx117] ^ rk[31];
    std::uint32_t eax246 = td2[(std::uint8_t) (edx30 >> 8)] ^ td1[(std::uint8_t) (ecx133 >> 16)] ^ td0[(std::uint8_t) (eax223 >> 24)] ^ td3[(std::uint8_t) ecx142] ^ rk[33];
    std::uint32_t ecx150 = td1[(std::uint8_t) (eax223 >> 16)] ^ td2[(std::uint8_t) (ecx133 >> 8)] ^ td0[(std::uint8_t) (ecx142 >> 24)] ^ td3[(std::uint8_t) edx30] ^ rk[34];
    std::uint32_t ecx156 = td1[(std::uint8_t) (edx30 >> 16)] ^ td2[(std::uint8_t) (ecx142 >> 8)] ^ td0[(std::uint8_t) (ecx133 >> 24)] ^ td3[(std::uint8_t) eax223] ^ rk[32];
    std::uint32_t ebx39 = td1[(std::uint8_t) (ecx142 >> 16)] ^ td2[(std::uint8_t) (eax223 >> 8)] ^ td0[(std::uint8_t) (edx30 >> 24)] ^ td3[(std::uint8_t) ecx133] ^ rk[35];
    std::uint32_t ecx163 = td1[(std::uint8_t) (ebx39 >> 16)] ^ td2[(std::uint8_t) (ecx150 >> 8)] ^ td0[(std::uint8_t) (ecx156 >> 24)] ^ td3[(std::uint8_t) eax246] ^ rk[36];
    std::uint32_t ecx169 = td2[(std::uint8_t) (ebx39 >> 8)] ^ td1[(std::uint8_t) (ecx156 >> 16)] ^ td0[(std::uint8_t) (eax246 >> 24)] ^ td3[(std::uint8_t) ecx150] ^ rk[37];
    std::uint32_t eax296 = td2[(std::uint8_t) (ecx156 >> 8)] ^ td1[(std::uint8_t) (eax246 >> 16)] ^ td0[(std::uint8_t) (ecx150 >> 24)] ^ td3[(std::uint8_t) ebx39] ^ rk[38];
    std::uint32_t eax306 = td2[(std::uint8_t) (eax246 >> 8)] ^ td1[(std::uint8_t) (ecx150 >> 16)] ^ td0[(std::uint8_t) (ebx39 >> 24)] ^ td3[(std::uint8_t) ecx156] ^ rk[39];
    std::uint32_t eax316 = td2[(std::uint8_t) (eax306 >> 8)] ^ td1[(std::uint8_t) (ecx163 >> 16)] ^ td0[(std::uint8_t) (ecx169 >> 24)] ^ td3[(std::uint8_t) eax296] ^ rk[41];
    std::uint32_t eax331 = td1[(std::uint8_t) (ecx169 >> 16)] ^ td2[(std::uint8_t) (ecx163 >> 8)] ^ td0[(std::uint8_t) (eax296 >> 24)] ^ td3[(std::uint8_t) (eax306)] ^ rk[42];
    std::uint32_t ecx192 = td1[(std::uint8_t) (eax306 >> 16)] ^  td2[(std::uint8_t) (eax296 >> 8)] ^ td0[(std::uint8_t) (ecx163 >> 24)] ^ td3[(std::uint8_t) ecx169] ^ rk[40];
    std::uint32_t ebx52 = td1[(std::uint8_t) (eax296 >> 16)] ^ td2[(std::uint8_t) (ecx169 >> 8)] ^ td0[(std::uint8_t) (eax306 >> 24)] ^ td3[(std::uint8_t) ecx163] ^ rk[43];
    std::uint32_t eax358 = td2[(std::uint8_t) (eax331 >> 8)] ^ td1[(std::uint8_t) (ebx52 >> 16)] ^ td0[(std::uint8_t) (ecx192 >> 24)] ^ td3[(std::uint8_t) eax316] ^ rk[44];
    std::uint32_t ecx200 = td2[(std::uint8_t) (ebx52 >> 8)] ^ td1[(std::uint8_t) (ecx192 >> 16)] ^ td0[(std::uint8_t) (eax316 >> 24)] ^ td3[(std::uint8_t) eax331] ^ rk[45];
    std::uint32_t ebx63 = td2[(std::uint8_t) (ecx192 >> 8)] ^ td1[(std::uint8_t) (eax316 >> 16)] ^ td0[(std::uint8_t) (eax331 >> 24)] ^ td3[(std::uint8_t) ebx52] ^ rk[46];
    std::uint32_t ebx68 = td2[(std::uint8_t) (eax316 >> 8)] ^ td3[(std::uint8_t) ecx192] ^ td1[(std::uint8_t) (eax331 >> 16)] ^ td0[(std::uint8_t) (ebx52 >> 24)] ^ rk[47];
    std::uint32_t ecx215 = td1[(std::uint8_t) (ebx68 >> 16)] ^ td2[(std::uint8_t) (ebx63 >> 8)] ^ td0[(std::uint8_t) (eax358 >> 24)] ^ td3[(std::uint8_t) ecx200] ^ rk[48];
    std::uint32_t ecx221 = td2[(std::uint8_t) (ebx68 >> 8)] ^ td1[(std::uint8_t) (eax358 >> 16)] ^ td0[(std::uint8_t) (ecx200 >> 24)] ^ td3[(std::uint8_t) ebx63] ^ rk[49];
    std::uint32_t eax413 = td1[(std::uint8_t) (ecx200 >> 16)] ^ td2[(std::uint8_t) (eax358 >> 8)] ^ td0[(std::uint8_t) (ebx63 >> 24)] ^ td3[(std::uint8_t) ebx68] ^ rk[50];
    std::uint32_t ecx228 = td2[(std::uint8_t) (ecx200 >> 8)] ^ td1[(std::uint8_t) (ebx63 >> 16)] ^ td0[(std::uint8_t) (ebx68 >> 24)] ^ td3[(std::uint8_t) eax358] ^ rk[51];
    std::uint32_t eax432 = td2[(std::uint8_t)(eax413 >> 8)] ^ td1[(std::uint8_t) (ecx228 >> 16)] ^ td0[(std::uint8_t) (ecx215 >> 24)] ^ td3[(std::uint8_t) ecx221] ^ rk[52];
    std::uint32_t ecx234 = td2[(std::uint8_t) (ecx228 >> 8)] ^ td1[(std::uint8_t) (ecx215 >> 16)] ^ td0[(std::uint8_t) (ecx221 >> 24)] ^ td3[(std::uint8_t) eax413] ^ rk[53];
    std::uint32_t eax451 = td1[(std::uint8_t) (ecx221 >> 16)] ^ td2[(std::uint8_t) (ecx215 >> 8)] ^ td0[(std::uint8_t) (eax413 >> 24)] ^ td3[(std::uint8_t) ecx228] ^ rk[54];
    std::uint32_t edi33 = td2[(std::uint8_t) (ecx221 >> 8)] ^ td3[(std::uint8_t) ecx215] ^ td1[(std::uint8_t) (eax413 >> 16)] ^ td0[(std::uint8_t) (ecx228 >> 24)] ^ rk[55];
    std::uint32_t ecx243 = (((std::uint8_t) td4[(std::uint8_t) (edi33 >> 16)] * 0x0001'0101u) & 0x00FF'0000u) ^ (((std::uint8_t) td4[(std::uint8_t) (eax451 >> 8)] * 0x0000'0101u) & 0x0000'FF00u);
    std::uint32_t ecx246 = ecx243 ^ (((std::uint8_t) td4[(std::uint8_t) (eax432 >> 24)] * 0x0101'0101u) & 0xFF00'0000u) ^ (std::uint8_t) td4[(std::uint8_t) ecx234] ^ rk[56];
    a4[0] = (std::uint8_t) ((ecx246 >> 24) ^ xorKey[0]);
    a4[1] = (std::uint8_t) ((ecx246 >> 16) ^ xorKey[1]);
    a4[2] = (std::uint8_t) ((ecx246 >> 8) ^ xorKey[2]);
    a4[3] = (std::uint8_t) (ecx246 ^ xorKey[3]);
    std::uint32_t edx55 =
        (((std::uint8_t) td4[(std::uint8_t) (eax432 >> 16)] * 0x0001'0101u) & 0x00FF'0000u) ^
        (((std::uint8_t) td4[(std::uint8_t) (edi33 >> 8)] * 0x0000'0101u) & 0x0000'FF00u) ^
        (((std::uint8_t) td4[(std::uint8_t) (ecx234 >> 24)] * 0x0101'0101u) & 0xFF00'0000u) ^ rk[57];
    a4[4] = (std::uint8_t) ((edx55 >> 24) ^ xorKey[4]);
    a4[5] = (std::uint8_t) ((edx55 >> 16) ^ xorKey[5]);
    a4[6] = (std::uint8_t) ((edx55 >> 8) ^ xorKey[6]);
    a4[7] = (std::uint8_t) ((td4[(std::uint8_t) eax451] ^ rk[57]) ^ xorKey[7]);
    std::uint32_t edx61 =
        (((std::uint8_t) td4[(std::uint8_t) (ecx234 >> 16)] * 0x0001'0101u) & 0x00FF'0000u) ^
        (((std::uint8_t) td4[(std::uint8_t) (eax432 >> 8)] * 0x0000'0101u) & 0x0000'FF00u) ^
        (((std::uint8_t) td4[(std::uint8_t) (eax451 >> 24)] * 0x0101'0101u) & 0xFF00'0000u) ^ rk[58];
    a4[8] = (std::uint8_t) ((edx61 >> 24) ^ xorKey[8]);
    a4[9] = (std::uint8_t) ((edx61 >> 16) ^ xorKey[9]);
    a4[10] = (std::uint8_t) ((edx61 >> 8) ^ xorKey[10]);
    a4[11] = (std::uint8_t) ((td4[(std::uint8_t) edi33] ^ rk[58]) ^ xorKey[11]);
    std::uint32_t edx67 =
        (((std::uint8_t) td4[(std::uint8_t) (eax451 >> 16)] * 0x0001'0101u) & 0x00FF'0000u) ^
        (((std::uint8_t) td4[(std::uint8_t) (ecx234 >> 8)] * 0x0000'0101u) & 0x0000'FF00u) ^
        (((std::uint8_t) td4[(std::uint8_t) (edi33 >> 24)] * 0x0101'0101u) & 0xFF00'0000u) ^ rk[59];
    a4[12] = (std::uint8_t) ((edx67 >> 24) ^ xorKey[12]);
    a4[13] = (std::uint8_t) ((edx67 >> 16) ^ xorKey[13]);
    a4[14] = (std::uint8_t) ((edx67 >> 8) ^ xorKey[14]);
    a4[15] = (std::uint8_t) ((td4[(std::uint8_t) eax432] ^ rk[59]) ^ xorKey[15]);
}

}

void decryptAssaultFireFuture(std::span<std::byte> data) {
    for (std::size_t offset = 0; offset + 16 <= data.size(); offset += 16) {
        std::array<std::uint8_t, 16> input{};
        std::memcpy(input.data(), data.data() + offset, 16);
        decryptBlock(input, std::span<std::uint8_t, 16>(reinterpret_cast<std::uint8_t*>(data.data() + offset), 16));
    }
}

}
