#pragma once

#define MSG_IMU0_A       0x01
#define MSG_IMU0_G       0x02
#define MSG_IMU0_M       0x03
#define MSG_IMU0_STATUS  0x04

#define MSG_IMU1_A       0x09
#define MSG_IMU1_G       0x0a
#define MSG_IMU1_M       0x0b
#define MSG_IMU1_STATUS  0x0c

#define MSG_IMU2_A       0x11
#define MSG_IMU2_G       0x12
#define MSG_IMU2_M       0x13
#define MSG_IMU2_STATUS  0x14

#define MSG_IMU3_A       0x19
#define MSG_IMU3_G       0x1a
#define MSG_IMU3_M       0x1b
#define MSG_IMU3_STATUS  0x1c

#define MSG_IMU4_A       0x21
#define MSG_IMU4_G       0x22
#define MSG_IMU4_M       0x23
#define MSG_IMU4_STATUS  0x24

#define MSG_IMU5_A       0x29
#define MSG_IMU5_G       0x2a
#define MSG_IMU5_M       0x2b
#define MSG_IMU5_STATUS  0x2c

#define MSG_EKF_POSE        0x30
#define MSG_EKF_POS         0x31
#define MSG_EKF_VEL         0x32
#define MSG_EKF_INNOVATION  0x34

#define MSG_SYNC_STATS1     0x35
#define MSG_SYNC_STATS2     0x36
#define MSG_SYNC_STATS3     0x37
#define MSG_SYNC_STATS4     0x38

#define MSG_ERROR           0x3f


extern int boxid;
#define CANID_OWN(x) (((x)<<4) | (boxid&0x0f))
#define CANID_THEIR(x, b) (((x)<<4) | ((b)&0x0f))

#define BOXID(canid) (canid&0x0f)
#define MSGID(canid) (canid>>4)

#define SYNCID 0x000
