/*
 * common.h
 *
 *  Created on: 4.05.2016 ã.
 *      Author: Anton Angelov
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

typedef int32_t RETCODE;

#define RC_OK			0x00
#define RC_FALSE		0x01
#define RC_FAIL			0x02
#define	RC_OUTOFMEM		0x03
#define RC_INVALIDARG	0x04
#define RC_INVALIDDATA	0x05
#define RC_NOTIMPL		0x06

#define failed(rc) rc > RC_FALSE
#define succeeded(rc) rc <= RC_FALSE

#endif /* COMMON_H_ */
