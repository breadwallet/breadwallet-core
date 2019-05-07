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
#include "BRArray.h"

/**
 * Serialize an unsorted array of fields
 *
 * @param fields      unsorted array of fields
 * @parma num_fields  the number of fields to serialize
 */
extern int rippleSerialize(BRRippleField * fields, int num_fields,
                     uint8_t *buffer, int bufferSize);

/**
 * Deserialize a byte stream into an array of fields
 *
 * @param buffer      serialized bytes
 * @param bufferSize  number of bytes in stream
 * @param fields      BRArray of fields
 */
extern int rippleDeserialize(uint8_t *buffer, int bufferSize, BRArrayOf(BRRippleField) fields);

#endif
