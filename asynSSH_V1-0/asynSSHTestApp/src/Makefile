TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# build an ioc application
PROD_IOC = asynSSHTest

DBD += asynSSHTest.dbd
# powetPMACTest.dbd will be made up from these files:
asynSSHTest_DBD += base.dbd
asynSSHTest_DBD += asyn.dbd
asynSSHTest_DBD += drvAsynSSHPort.dbd
asynSSHTest_DBD += stream.dbd
asynSSHTest_DBD += asynSSHTestFunctions.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
asynSSHTest_SRCS += asynSSHTest_registerRecordDeviceDriver.cpp
asynSSHTest_SRCS += asynSSHTestFunctions.c
asynSSHTest_SRCS_DEFAULT += asynSSHTestMain.cpp
asynSSHTest_SRCS_vxWorks += -nil-

# Include the following libraries in the build
asynSSHTest_LIBS += asyn
asynSSHTest_LIBS += asynSSH
asynSSHTest_LIBS += stream
asynSSHTest_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


