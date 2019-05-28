//
//  BRRippleSerialize.h
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRipple_serialize_h
#define BRRipple_serialize_h

#include "BRRippleBase.h"
#include "BRRipplePrivateStructs.h"
#include "BRArray.h"

/**
 * Serialize an unsorted array of fields
 *
 * @param fields      pointer to BRArrayOf(BRRippleField). NOTE: the address of the array
 *                    must be passed in due to a possible realloc of the array (and new memory location)
 * @param num_fields  the number of fields to serialize
 *
 * @return number of bytes read
 */
extern uint32_t rippleSerialize(BRRippleField * fields, uint32_t num_fields,
                     uint8_t *buffer, uint32_t bufferSize);

/**
 * Deserialize a byte stream into an array of fields
 *
 * @param buffer      serialized bytes
 * @param bufferSize  number of bytes in stream
 * @param fields      BRArray of fields
 */
extern int rippleDeserialize(uint8_t *buffer, uint32_t bufferSize, BRArrayOf(BRRippleField) *fields);

#endif
