#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := kettle_example

EXTRA_COMPONENT_DIRS := ${IDF_PATH}/../../iot-core

include $(IDF_PATH)/make/project.mk
