/*
 * dummySubDev.h
 *
 *  Created on: Dec 7, 2018
 *      Author: yukangzhi
 */

#ifndef __VIDEO_DUMMY_SUBDEV_H
#define __VIDEO_DUMMY_SUBDEV_H

#include <sys/ioccom.h>

#define DUMMY_SUBDEV_SPEC_FUNC1      _IO('d',0x77)
#define DUMMY_SUBDEV_SPEC_FUNC2      _IO('d',0x88)

INT  dummyVideoIspDrv(VOID);

INT  dummyVideoIspDevAdd(VOID);

#endif /* __VIDEO_DUMMY_SUBDEV_H */
