#ifndef DRIVERS_KSCAN_IT7259_
#define DRIVERS_KSCAN_IT7259_

#define IT7259_DEVICE_NAME              "ITE7259"

#define IT7259_CMD_BUFFER               0x20
#define IT7259_CMD_RESP_BUFFER          0xA0
#define IT7259_QUERY_BUFFER             0x80
#define IT7259_POINT_INF_BUFFER         0xE0

#define IT7259_QUERY_BUF_STATUS(buf)    ((buf) & 0x03)
#define IT7259_QUERY_BUF_PACKET(buf)    (((buf) >> 6) & 0x03)
#define IT7259_QUERY_BUF_DONE           0x00
#define IT7259_QUERY_BUF_BUSY           0x01
#define IT7259_QUERY_BUF_ERROR          0x02

// IT7259_SUBCMD_SET_INT parameters
#define IT7259_INT_ENABLED              0x01
#define IT7259_INT_DISABLED             0x00
#define IT7259_INT_LOW                  0x00
#define IT7259_INT_HIGH                 0x01
#define IT7259_INT_FALLING              0x02
#define IT7259_INT_RISING               0x03

// IT7259_SUBCMD_SET_PWR_MODE parameter
#define IT7259_PWR_IDLE                 0x01

#define IT7259_CMD_DEVICE_NAME          0x00

#define IT7259_CMD_GET_CAP_INF          0x01
#define IT7259_SUBCMD_GET_INT           0x04

#define IT7259_CMD_SET_CAP_INF          0x02
#define IT7259_SUBCMD_SET_INT           0x04

#define IT7259_CMD_SET_PWR_MODE         0x04
#define IT7259_SUBCMD_SET_PWR_MODE      0x00

#endif // DRIVERS_KSCAN_IT7259_
