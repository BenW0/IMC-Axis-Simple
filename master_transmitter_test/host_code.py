# Dummy Master I2C Interface host code
# ME 498/599 Advanced AM
# Matthew Sorenson & Ben Weiss
# University of Washington
#
# This module implements sending and receiving of structures
#  to/from the Master chip's serial interface. To use it, import
#  this file, open a serial connection to the Master, and call
#  the various functions.

import struct, time

# constants (copy/paste from constants.h)
# message type:
IMC_MSG_INITIALIZE = 1
IMC_MSG_STATUS = 2
IMC_MSG_HOME = 3
IMC_MSG_QUEUEMOVE = 4
IMC_MSG_GETPARAM = 5
IMC_MSG_SETPARAM = 6

# response type:
IMC_RSP_OK = 0
IMC_RSP_UNKNOWN = 1
IMC_RSP_ERROR = 2
IMC_RSP_QUEUEFULL = 3

# imc_axis_parameters
IMC_PARAM_ERROR_INFO1 = 0
IMC_PARAM_ERROR_INFO2 = 1
IMC_PARAM_FLIP_AXIS = 2
IMC_PARAM_ENFORCE_LIMITS = 3
IMC_PARAM_MIN_LIMIT_EN = 4
IMC_PARAM_MIN_LIMIT_INV = 5
IMC_PARAM_MIN_LIMIT_PULLUP = 6
IMC_PARAM_MAX_LIMIT_EN = 7
IMC_PARAM_MAX_LIMIT_INV = 8
IMC_PARAM_MAX_LIMIT_PULLUP = 9
IMC_PARAM_MOTOR_ON = 10
IMC_PARAM_MOTOR_IDLE_TIMEOUT = 11
IMC_PARAM_SLOWDOWN = 12

# imc_axis_error
IMC_ERR_NONE = 0
IMC_ERR_CONTROL = 1
IMC_ERR_ELECTRICAL = 2
IMC_ERR_MECHANICAL = 3
IMC_ERR_TIMEOUT = 4

# sendInit - send the Initialize message
# Params: serobj - serial object to Master
#   fwver - firmware version (2 bytes)
#   reserved - reserved bytes touple (4-byte, 2-byte)
def sendInit(serobj, fwver = 0, reserved = (0,0)) :
    # Pack the Init message
    pack = struct.pack("=BHLH", IMC_MSG_INITIALIZE, fwver, reserved[0], reserved[1])
    resp = sendPacket(serobj, pack, "=BHH")
    print "Response:", resp[0]
    print "Slave hw rev:", resp[1]
    print "Slave fw rev:", resp[2]

# sendPacket - sends a simple packet and waits for response.
# Params: serobj - serial object
#   pack - packet to send (already packed with struct)
#   resp_fmt - format for the response.
# Return: touple of the entries in the response packet, unpacked according to resp_fmt
def sendPacket(serobj, pack, resp_fmt) :
    
    # send the init message
    serobj.write(pack)
    # wait a second...
    time.sleep(0.2)
    # Read back the result
    err = ''
    while serobj.inWaiting() > 0 :
        c = serobj.read();
        if struct.unpack("=B",c)[0] < 10 :  # first byte should always be small (i.e. from response enum)
            # we got a response! Read expected number of bytes
            resp = c + serobj.read(struct.calcsize(resp_fmt) - 1)       # we've already read the header
        else :
            # we got an error message. Accumulate for later printing.
            err = err + c
    # print the errors, return the response
    print err
    return struct.unpack(resp_fmt, resp)
            
            
