TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += asynSSH

DBD += drvAsynSSHPort.dbd

# Link locally-provided code into the support library,
asynSSH_SRCS += sshDriver.cpp
asynSSH_SRCS += drvAsynSSHPort.cpp

asynSSH_LIBS += asyn
asynSSH_LIBS += ssh2
asynSSH_LIBS += $(EPICS_BASE_IOC_LIBS)
ssh2_DIR = /usr/local/lib

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

