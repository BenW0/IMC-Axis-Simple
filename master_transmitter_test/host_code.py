# Dummy Master I2C Interface host code
# ME 498/599 Advanced AM
# Matthew Sorenson & Ben Weiss
# University of Washington
#
# This module implements sending and receiving of structures
#  to/from the Master chip's serial interface when using the
#  master_transmitter_test firmware. To use it, import
#  this file, open a serial connection to the Master, and call
#  the various functions.

import struct, time
import serial

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
IMC_PARAM_FLIP_AXIS   = 2
IMC_PARAM_HOME_DIR    = 3
IMC_PARAM_MIN_SOFTWARE_ENDSTOPS = 4
IMC_PARAM_MAX_SOFTWARE_ENDSTOPS = 5
IMC_PARAM_MIN_LIMIT_EN = 6
IMC_PARAM_MIN_LIMIT_INV = 7
IMC_PARAM_MIN_LIMIT_PULLUP = 8
IMC_PARAM_MAX_LIMIT_EN = 9
IMC_PARAM_MAX_LIMIT_INV = 10 
IMC_PARAM_MAX_LIMIT_PULLUP = 11
IMC_PARAM_MIN_POS = 12
IMC_PARAM_MAX_POS = 13
IMC_PARAM_HOME_POS = 14
IMC_PARAM_HOMING_FEEDRATE = 15
IMC_PARAM_MOTOR_ON = 16 
IMC_PARAM_MOTOR_IDLE_TIMEOUT = 17
IMC_PARAM_SLOWDOWN = 18
IMC_PARAM_LOCATION = 19
IMC_PARAM_SYNC_ERROR = 20

# imc_axis_error
IMC_ERR_NONE = 0
IMC_ERR_CONTROL = 1
IMC_ERR_ELECTRICAL = 2
IMC_ERR_MECHANICAL = 3
IMC_ERR_TIMEOUT = 4

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
    resp = ''
    try :
        while serobj.inWaiting() > 0 :
            c = serobj.read(1);
            #print "Byte:",c,"-", struct.unpack("=B",c)[0]
            if struct.unpack("=B",c)[0] < 10 and '' == resp:  # first byte should always be small (i.e. from response enum)
                # we got a response! Read expected number of bytes
                print "Got a response byte", c, "want", struct.calcsize(resp_fmt) - 1, "payload bytes", serobj.inWaiting(), "available."
                if struct.unpack("=B",c)[0] > 0 :
                    print "Response was an error!"
                    
                resp = c + serobj.read(struct.calcsize(resp_fmt) - 1)       # we've already read the header
            else :
                # we got an error message. Accumulate for later printing.
                err = err + c
    finally :
        # print the errors, return the response
        print err
        return struct.unpack(resp_fmt, resp)




# sendInit - send the Initialize message
# Params: serobj - serial object to Master
#   fwver - firmware version (2 bytes)
#   reserved - reserved bytes touple (4-byte, 2-byte)
def sendInit(serobj, fwver = 0, reserved = (0,0)) :
    # Pack the Init message
    pack = struct.pack("=BHLH", IMC_MSG_INITIALIZE, fwver, reserved[0], reserved[1])
    resp = sendPacket(serobj, pack, "=BHHH")
    print "Response:", resp[0]
    print "Slave hw rev:", resp[1]
    print "Slave fw rev:", resp[2]
    print "Slave queue depth:", resp[3]
            


# sendStatus - send the Status message
# Params: serobj - serial object to Master
def sendStatus(serobj) :
    # Pack the Status message
    pack = struct.pack("=B", IMC_MSG_STATUS)
    resp = sendPacket(serobj, pack, "=BiIBB")
    print "Response:", resp[0]
    print "Location:", resp[1]
    print "Sync Error:", resp[2]
    print "Status:", resp[3]
    print "Queued Moves:", resp[4]


# sendHome - send the Home message
# Params: serobj - serial object to Master
def sendHome(serobj) :
    # Pack the Home message
    pack = struct.pack("=B", IMC_MSG_HOME)
    resp = sendPacket(serobj, pack, "=Bi")
    print "Response:", resp[0]
    print "Old Position:", resp[1]

# sendQueueMove - send the Queue Move message
# Params: serobj - serial object to Master
#   length - move length on this axis
#   total_length - move length of biggest axis
#   init_rate - initial rate
#   fin_rate - final rate
#   accel - acceleration
#   stop_accel - time to stop accelerating
#   start_decel - time to start decelerating
#  * see specification document for more details.
def sendQueueMove(serobj, length, total_length, init_rate, fin_rate, accel, stop_accel, start_decel) :
    # Pack the QueueMove message
    pack = struct.pack("=BiIIIIII", IMC_MSG_QUEUEMOVE, length, total_length, init_rate, fin_rate, accel, stop_accel, start_decel)
    resp = sendPacket(serobj, pack, "=B")
    print "Response:", resp[0]


# sendGetParam - send the Get Parameter message
# Params: serobj - serial object to Master
#   param_id - parameter to get
def sendGetParam(serobj, param_id) :
    # Pack the ParamID message
    pack = struct.pack("=BB", IMC_MSG_GETPARAM, param_id)
    resp = sendPacket(serobj, pack, "=BI")
    print "Response:", resp[0]
    print "Param Value:", resp[1]


# sendSetParam - send the Set Parameter message
# Params: serobj - serial object to Master
#   param_id - parameter id
#   value - value to assign to parameter (32-bit unsigned int)
def sendSetParam(serobj, param_id, value) :
    # Pack the Home message
    pack = struct.pack("=BIB", IMC_MSG_SETPARAM, value, param_id)
    resp = sendPacket(serobj, pack, "=B")
    print "Response:", resp[0]
