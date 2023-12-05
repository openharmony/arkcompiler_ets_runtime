/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * We generate muid-hashcode according to the MD5 Message-Digest Algorithm here.
 */
#include "muid.h"
#include <cstring>
#include "securec.h"

/*
 * Basic MUID functions.
 */
#define F(x, y, z) (((x) & (y)) | ((~(x)) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~(z))))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~(z))))

/*
 * Muid Transformation function.
 */
#define TRANS(f, a, b, c, d, x, t, s)                        \
    (a) += f((b), (c), (d)) + (x) + (t);                     \
    (a) = (((a) << (s)) | (((a)&0xffffffff) >> (32 - (s)))); \
    (a) += (b);

/*
 * Divide the whole input data into sevral groups with kGroupSizebit each and decode them.
 */
#if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
#define DECODE(n, input, output) \
    ((output)[(n)] = (*reinterpret_cast<unsigned int *>(const_cast<unsigned char *>(&(input)[(n)*4]))))
#else
#define DECODE(n, input, output)                                                          \
    ((output)[(n)] = (unsigned int)(input)[(n)*4] | ((unsigned int)(input)[(n)*4 + 1] << 8) | \
                     ((unsigned int)(input)[(n)*4 + 2] << 16) | ((unsigned int)(input)[(n)*4 + 3] << 24))
#endif

/*
 * Encode function.
 */
#define ENCODE(dst, src)                     \
    (dst)[0] = (unsigned char)(src);         \
    (dst)[1] = (unsigned char)((src) >> 8);  \
    (dst)[2] = (unsigned char)((src) >> 16); \
    (dst)[3] = (unsigned char)((src) >> 24);

/*
 * Body of transformation.
 */
static const unsigned char *MuidTransform(MuidContext &status, const unsigned char &data, uint64_t count)
{
    unsigned int a, b, c, d;
    auto *result = &data;

    while (count--) {
        for (unsigned int i = 0; i < kBlockLength; i++) {
            DECODE(i, result, status.block);
        }

        a = status.a;
        b = status.b;
        c = status.c;
        d = status.d;

        /* Round 1 */
        TRANS(F, a, b, c, d, status.block[0], 0xd76aa478, 7)
        TRANS(F, d, a, b, c, status.block[1], 0xe8c7b756, 12)
        TRANS(F, c, d, a, b, status.block[2], 0x242070db, 17)
        TRANS(F, b, c, d, a, status.block[3], 0xc1bdceee, 22)
        TRANS(F, a, b, c, d, status.block[4], 0xf57c0faf, 7)
        TRANS(F, d, a, b, c, status.block[5], 0x4787c62a, 12)
        TRANS(F, c, d, a, b, status.block[6], 0xa8304613, 17)
        TRANS(F, b, c, d, a, status.block[7], 0xfd469501, 22)
        TRANS(F, a, b, c, d, status.block[8], 0x698098d8, 7)
        TRANS(F, d, a, b, c, status.block[9], 0x8b44f7af, 12)
        TRANS(F, c, d, a, b, status.block[10], 0xffff5bb1, 17)
        TRANS(F, b, c, d, a, status.block[11], 0x895cd7be, 22)
        TRANS(F, a, b, c, d, status.block[12], 0x6b901122, 7)
        TRANS(F, d, a, b, c, status.block[13], 0xfd987193, 12)
        TRANS(F, c, d, a, b, status.block[14], 0xa679438e, 17)
        TRANS(F, b, c, d, a, status.block[15], 0x49b40821, 22)

        /* Round 2 */
        TRANS(G, a, b, c, d, status.block[1], 0xf61e2562, 5)
        TRANS(G, d, a, b, c, status.block[6], 0xc040b340, 9)
        TRANS(G, c, d, a, b, status.block[11], 0x265e5a51, 14)
        TRANS(G, b, c, d, a, status.block[0], 0xe9b6c7aa, 20)
        TRANS(G, a, b, c, d, status.block[5], 0xd62f105d, 5)
        TRANS(G, d, a, b, c, status.block[10], 0x02441453, 9)
        TRANS(G, c, d, a, b, status.block[15], 0xd8a1e681, 14)
        TRANS(G, b, c, d, a, status.block[4], 0xe7d3fbc8, 20)
        TRANS(G, a, b, c, d, status.block[9], 0x21e1cde6, 5)
        TRANS(G, d, a, b, c, status.block[14], 0xc33707d6, 9)
        TRANS(G, c, d, a, b, status.block[3], 0xf4d50d87, 14)
        TRANS(G, b, c, d, a, status.block[8], 0x455a14ed, 20)
        TRANS(G, a, b, c, d, status.block[13], 0xa9e3e905, 5)
        TRANS(G, d, a, b, c, status.block[2], 0xfcefa3f8, 9)
        TRANS(G, c, d, a, b, status.block[7], 0x676f02d9, 14)
        TRANS(G, b, c, d, a, status.block[12], 0x8d2a4c8a, 20)

        /* Round 3 */
        TRANS(H, a, b, c, d, status.block[5], 0xfffa3942, 4)
        TRANS(H, d, a, b, c, status.block[8], 0x8771f681, 11)
        TRANS(H, c, d, a, b, status.block[11], 0x6d9d6122, 16)
        TRANS(H, b, c, d, a, status.block[14], 0xfde5380c, 23)
        TRANS(H, a, b, c, d, status.block[1], 0xa4beea44, 4)
        TRANS(H, d, a, b, c, status.block[4], 0x4bdecfa9, 11)
        TRANS(H, c, d, a, b, status.block[7], 0xf6bb4b60, 16)
        TRANS(H, b, c, d, a, status.block[10], 0xbebfbc70, 23)
        TRANS(H, a, b, c, d, status.block[13], 0x289b7ec6, 4)
        TRANS(H, d, a, b, c, status.block[0], 0xeaa127fa, 11)
        TRANS(H, c, d, a, b, status.block[3], 0xd4ef3085, 16)
        TRANS(H, b, c, d, a, status.block[6], 0x04881d05, 23)
        TRANS(H, a, b, c, d, status.block[9], 0xd9d4d039, 4)
        TRANS(H, d, a, b, c, status.block[12], 0xe6db99e5, 11)
        TRANS(H, c, d, a, b, status.block[15], 0x1fa27cf8, 16)
        TRANS(H, b, c, d, a, status.block[2], 0xc4ac5665, 23)

        /* Round 4 */
        TRANS(I, a, b, c, d, status.block[0], 0xf4292244, 6)
        TRANS(I, d, a, b, c, status.block[7], 0x432aff97, 10)
        TRANS(I, c, d, a, b, status.block[14], 0xab9423a7, 15)
        TRANS(I, b, c, d, a, status.block[5], 0xfc93a039, 21)
        TRANS(I, a, b, c, d, status.block[12], 0x655b59c3, 6)
        TRANS(I, d, a, b, c, status.block[3], 0x8f0ccc92, 10)
        TRANS(I, c, d, a, b, status.block[10], 0xffeff47d, 15)
        TRANS(I, b, c, d, a, status.block[1], 0x85845dd1, 21)
        TRANS(I, a, b, c, d, status.block[8], 0x6fa87e4f, 6)
        TRANS(I, d, a, b, c, status.block[15], 0xfe2ce6e0, 10)
        TRANS(I, c, d, a, b, status.block[6], 0xa3014314, 15)
        TRANS(I, b, c, d, a, status.block[13], 0x4e0811a1, 21)
        TRANS(I, a, b, c, d, status.block[4], 0xf7537e82, 6)
        TRANS(I, d, a, b, c, status.block[11], 0xbd3af235, 10)
        TRANS(I, c, d, a, b, status.block[2], 0x2ad7d2bb, 15)
        TRANS(I, b, c, d, a, status.block[9], 0xeb86d391, 21)

        status.a += a;
        status.b += b;
        status.c += c;
        status.d += d;

        result += kGroupSize;
    }

    return result;
}

/*
 * Initialize constants here.
 */
void MuidInit(MuidContext &status)
{
    status.a = 0x67452301;
    status.b = 0xefcdab89;
    status.c = 0x98badcfe;
    status.d = 0x10325476;

    status.count[0] = 0;
    status.count[1] = 0;
}

/*
 * Decoding part(byte to unsigned int).
 */
void MuidDecode(MuidContext &status, const unsigned char &data, size_t size)
{
    unsigned int tmp = status.count[0];
    status.count[0] = (tmp + size) & 0x1fffffff;
    if (status.count[0] < tmp) {
        status.count[1]++;
    }
    uint32_t higherBits = static_cast<uint64_t>(size) >> kShiftAmount;
    status.count[1] += higherBits * kByteLength;

    size_t idx = tmp & kBitMask;
    size_t remain = kGroupSize - idx;
    auto *position = &data;

    if (idx != 0) {
        if (size < remain) {
            if (memcpy_s(&status.buffer[idx], kGroupSize, &data, size) != EOK) {
                return;
            }
            return;
        }

        if (memcpy_s(&status.buffer[idx], kGroupSize, &data, remain) != EOK) {
            return;
        }
        (void)MuidTransform(status, *status.buffer, 1);

        size -= remain;
        position += remain;
    }

    if (size >= kGroupSize) {
        position = MuidTransform(status, *position, size / kGroupSize);
        size &= kBitMask;
    }

    if (memcpy_s(status.buffer, kGroupSize, position, size) != EOK) {
        return;
    }
}

/*
 * Encoding part(unsigned int to byte).
 */
template <typename T>
void FullEncode(T &result, MuidContext &status)
{
    size_t idx = status.count[0] & kBitMask;
    status.buffer[idx++] = 0x80;

    size_t remain = kGroupSize - idx;

    if (remain < kByteLength) {
        if (memset_s(&status.buffer[idx], kGroupSize, 0, remain) != EOK) {
            return;
        }
        (void)MuidTransform(status, *status.buffer, 1);
        idx = 0;
        remain = kGroupSize;
    }

    if (memset_s(&status.buffer[idx], kGroupSize, 0, remain - kByteLength) != EOK) {
        return;
    }
    status.count[0] *= kByteLength;
    const unsigned int indexOfLastEight = 56;
    const unsigned int indexOfLastFour = 60;
    ENCODE(&status.buffer[indexOfLastEight], status.count[0])
    ENCODE(&status.buffer[indexOfLastFour], status.count[1])

    (void)MuidTransform(status, *status.buffer, 1);
    ENCODE(&result[0], status.a)
    ENCODE(&result[4], status.b)
}

void MuidEncode(unsigned char (&result)[kDigestShortHashLength], MuidContext &status)
{
    FullEncode(result, status);
    if (memset_s(&status, sizeof(status), 0, sizeof(status)) != EOK) {
        return;
    }
}

void MuidEncode(unsigned char (&result)[kDigestHashLength], MuidContext &status, bool use64Bit)
{
    FullEncode(result, status);
    if (!use64Bit) {
        ENCODE(&result[8], status.c)
        ENCODE(&result[12], status.d)
    }
    if (memset_s(&status, sizeof(status), 0, sizeof(status)) != EOK) {
        return;
    }
}

/*
 * The entrance functions.
 */
void GetMUIDHash(const unsigned char &data, size_t size, MUID &muid)
{
    MuidContext status;
    MuidInit(status);
    MuidDecode(status, data, size);
    MuidEncode(muid.data.bytes, status);
}

DigestHash GetDigestHash(const unsigned char &bytes, uint32_t len)
{
    DigestHash digestHash;
    MuidContext digestContext;

    digestHash.data.first = 0;
    digestHash.data.second = 0;

    MuidInit(digestContext);
    MuidDecode(digestContext, bytes, len);
    MuidEncode(digestHash.bytes, digestContext);

    return digestHash;
}

MUID GetMUID(const std::string &symbolName, bool forSystem)
{
    MUID muid;
    auto *data = reinterpret_cast<const unsigned char *>(symbolName.c_str());
    GetMUIDHash(*data, symbolName.length(), muid);
    if (forSystem) {
        muid.SetSystemNameSpace();
    } else {
        muid.SetApkNameSpace();
    }
    return muid;
}