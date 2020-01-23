/*
 Copyright (c) 2015 Orson Peters <orsonpeters@gmail.com>
 
 This software is provided 'as-is', without any express or implied warranty. In no event will the
 authors be held liable for any damages arising from the use of this software.
 
 Permission is granted to anyone to use this software for any purpose, including commercial
 applications, and to alter it and redistribute it freely, subject to the following restrictions:
 
 1. The origin of this software must not be misrepresented; you must not claim that you wrote the
 original software. If you use this software in a product, an acknowledgment in the product
 documentation would be appreciated but is not required.
 
 2. Altered source versions must be plainly marked as such, and must not be misrepresented as
 being the original software.
 
 3. This notice may not be removed or altered from any source distribution.
 */

#include "ed25519.h"
#include "sha512.h"
#include "ge.h"
#include "sc.h"


void ed25519_sign(unsigned char *signature, const unsigned char *message, size_t message_len, const unsigned char *public_key, const unsigned char *private_key) {
    sha512_context hash;
    unsigned char hram[64];
    unsigned char r[64];
    ge_p3 R;


    sha512_init(&hash);
    sha512_update(&hash, private_key + 32, 32);
    sha512_update(&hash, message, message_len);
    sha512_final(&hash, r);

    sc_reduce(r);
    ge_scalarmult_base(&R, r);
    ge_p3_tobytes(signature, &R);

    sha512_init(&hash);
    sha512_update(&hash, signature, 32);
    sha512_update(&hash, public_key, 32);
    sha512_update(&hash, message, message_len);
    sha512_final(&hash, hram);

    sc_reduce(hram);
    sc_muladd(signature + 32, hram, private_key, r);
}
