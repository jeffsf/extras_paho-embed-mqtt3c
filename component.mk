#-
# Copyright (c) 2017, Jeff Kletsky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
#     * Neither the name of the copyright holder nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# Inspired by extras/mbedtls/component.mk
# commited by Angus Gratton <gus@projectgus.com>

# Component makefile for paho-embed-mqtt3c

# Adapted by Jeff Kletsky Copyright (c) 2017 ALL RIGHTS RESERVED
# using git tag v.1.1.0 of https://github.com/eclipse/paho.mqtt.embedded-c

# Builds the esp-open-rtos equivalent of paho-embed-mqtt3c

##############################
# MQTTCLIENT_PLATFORM_HEADER #
# MQTTCLIENT_PLATFORM_DIR    #
##############################

#
# Defined by paho-embed-mqtt3c source to identify which
# "platform-specific" header file to use.
# Code uses a #define macro to have effect of
# #include "MQTTCLIENT_PLATFORM_HEADER"
#
# Keep that convention for this code and its future developement
#
# Implemenation here assumes of the form *.h
#
# Get the "right" MQTTCLIENT_PLATFORM_HEADER
# Order of preference is
#    make variable
#    paho-embed-mqtt3c_CFLAGS
#    paho-embed-mqtt3c_CPPFLAGS
#    PROGRAM_CFLAGS
#    PROGRAM_CPPFLAGS
#    CFLAGS
#    CPPFLAGS
#    (set to default)
#

mqtt3c_mqttclient_platform_default = MQTTSimple.h

mqtt3c_mph_prefix = -DMQTTCLIENT_PLATFORM_HEADER

# utility macros -- need to be declared before in-flow use

# Don't show mqtt3c_info/warning if only target is 'clean'

ifeq ($(MAKECMDGOALS),clean)
mqtt3c_enable_info :=
mqtt3c_enable_warning :=
else
mqtt3c_enable_info := YES
mqtt3c_enable_warning := YES
endif

# Build system uses V=1 for internal verbose
# Enable showing mqtt3c_info/warning if V=1

ifeq ($(V),1)
mqtt3c_enable_info := YES
mqtt3c_enable_warning := YES
endif

#
# $(call mqtt3c_warning,Some warning text)
#
define mqtt3c_warning
$(if $(mqtt3c_enable_warning),$(warning *** WARNING: $(1)))
endef

#
# $(call mqtt3c_info,Some info text)
#
define mqtt3c_info
$(if $(mqtt3c_enable_info),$(info paho-embed-mqtt3c INFO: $(1)))
endef



#
# Select the "proper" MQTTCLIENT_PLATFORM_HEADER 
#

mqtt3c_platform_from_make = $(MQTTCLIENT_PLATFORM_HEADER)

#
# Check flags in priority of lowest to highest priority
#

mqtt3c_platform_flag_checks = \
	CPPFLAGS \
	CFLAGS \
	PROGRAM_CPPFLAGS \
	PROGRAM_CFLAGS \
	paho-embed-mqtt3c_CPPFLAGS \
	paho-embed-mqtt3c_CFLAGS
#
# flag sets that must have consistent -DMQTTCLIENT_PLATFORM_HEADER=
#

mqtt3c_platform_flag_required = \
	paho-embed-mqtt3c_CFLAGS \
	PROGRAM_CFLAGS

#
# mqtt3c_extract_mph_flags
#
# returns a list of entire flags that match $(mqtt3c_mph_prefix)=%
#
# $(1): name of flags list; e.g. CFLAGS
#
define mqtt3c_extract_mph_flags
$(filter $(mqtt3c_mph_prefix)=%,$($(1)))
endef

#
# mqtt3c_extract_mph_values
#
# returns the value portion of flags that match  $(mqtt3c_mph_prefix)=%
# does *not* strip the suffix (.h)
#
# $(1): name of flags list; e.g. CFLAGS
#
define mqtt3c_extract_mph_values
$(patsubst $(mqtt3c_mph_prefix)=%,%,$(call mqtt3c_extract_mph_flags,$(1)))
endef

# Initialize notion of "best" flag and its source

mqtt3c_platform_from :=
mqtt3c_platform_header_found :=

#
# check_for_flags
#
# Find any -DMQTTCLIENT_PLATFORM_HEADER= flags in the flag set
# Update the notion of "best" flag and its source
#
# $(1): Name of flag set to check
#
# There may be multiple matching flags, take the last one
define check_for_flags
mqtt3c_mph_found = \
	$$(lastword $$(call mqtt3c_extract_mph_values,$(1)))
ifneq ($$(mqtt3c_mph_found),)
    mqtt3c_platform_from := $(1)
    mqtt3c_platform_header_found := $$(mqtt3c_mph_found)
endif
endef

$(foreach f,$(mqtt3c_platform_flag_checks),$(eval $(call check_for_flags,$(f))))

#
# Pick the "best" -DMQTTCLIENT_PLATFORM_HEADER= flag
#

ifneq ($(MQTTCLIENT_PLATFORM_HEADER),)
#   MQTTCLIENT_PLATFORM_HEADER := $(MQTTCLIENT_PLATFORM_HEADER)
    mqtt3c_platform_from = MQTTCLIENT_PLATFORM_HEADER make variable

else ifneq ($(mqtt3c_platform_header_found),)
    MQTTCLIENT_PLATFORM_HEADER = $(mqtt3c_platform_header_found)
#   mqtt3c_platform_from := $(mqtt3c_platform_from)

else
    MQTTCLIENT_PLATFORM_HEADER = $(mqtt3c_mqttclient_platform_default)
    mqtt3c_platform_from = component.mk: mqtt3c_mqttclient_platform_default

endif

mqtt3c_mph_flag = $(mqtt3c_mph_prefix)=$(MQTTCLIENT_PLATFORM_HEADER)

$(call mqtt3c_info,Transport: $(MQTTCLIENT_PLATFORM_HEADER) from $(mqtt3c_platform_from))


#
# Don't adjust the CFLAGS before calling component_compile_rules
# or the default CFLAGS aren't included
#

#
# mqtt3c_warn_conflicting_mph_flags
#
# Warn about conflicting MQTTCLIENT_PLATFORM_HEADER flags
#
# $(1): name of flags list to examime; e.g. CFLAGS
#
define mqtt3c_warn_conflicting_mph_flags
mqtt3c_mph_found_mismatch = $$(filter-out $$(mqtt3c_mph_flag),$$(call mqtt3c_extract_mph_flags,$(1)))
#$$(info $$(mqtt3c_mph_found_mismatch))
ifneq ($$(mqtt3c_mph_found_mismatch),)
    $$(call mqtt3c_warning,Inconsistent flags from $(1): $$(mqtt3c_mph_found_mismatch))
# Surprisingly, writing \# here results in "\#" in the output
    $$(call mqtt3c_info,Using $$(mqtt3c_mph_flag) # from $(mqtt3c_platform_from))
    $$(call mqtt3c_info,make variable MQTTCLIENT_PLATFORM_HEADER has highest precedence)
endif
endef

#
# mqtt3c_add_mph_flag
#
# Add the selected -DMQTTCLIENT_PLATFORM_HEADER= flag, if needed
#
# $(1): Name of flag set to ensure has the flag
#
define mqtt3c_add_mph_flag
ifeq ($$(filter $$(mqtt3c_mph_flag),$(1)),)
    $$(call mqtt3c_info,Adding transport flag to $(1) for $$(MQTTCLIENT_PLATFORM_HEADER))
    override $(1) += $$(mqtt3c_mph_flag)
endif
endef

#
# Create name of the directory containing th platform-specific header and code
# so that it can be added to INC_DIRS and paho-embed-mqtt3c_SRC_DIR
#

MQTTCLIENT_PLATFORM_DIR ?= \
	$(paho-embed-mqtt3c_ROOT)mqttclient_platforms/$(MQTTCLIENT_PLATFORM_HEADER:%.h=%)

#
# Build up INC_DIRS (for use by call component_compile_rules) incrementally
# This is intentionally +=, not =, to retain some pre-populated directories
#
INC_DIRS += \
	$(MQTTCLIENT_PLATFORM_DIR) \
	$(MQTTCLIENT_PLATFORM_DIR)/include


########################################################
# Selection of upstream sources that are used verbaitm #
########################################################

MQTT3C_GIT_DIR = $(paho-embed-mqtt3c_ROOT)paho.mqtt.embedded-c

#
# The object files needed were determined by cmake, make paho-embed-mqtt3c
# and by inspection on a POSIX platform using git tag v1.1.0
#
# MQTTPacketClient is  apparently a subset of paho-embed-mqtt3c
# Provide the broader set under assumption that the linker is efficient
#

mqtt3c_git_mqttpacket_dir = $(MQTT3C_GIT_DIR)/MQTTPacket/src
mqtt3c_objs_mqttpacket = \
	MQTTConnectClient.o \
	MQTTConnectServer.o \
	MQTTDeserializePublish.o \
	MQTTFormat.o \
	MQTTPacket.o \
	MQTTSerializePublish.o \
	MQTTSubscribeClient.o \
	MQTTSubscribeServer.o \
	MQTTUnsubscribeClient.o \
	MQTTUnsubscribeServer.o

INC_DIRS += $(mqtt3c_git_mqttpacket_dir)


mqtt3c_git_mqttclient_c_dir  = $(MQTT3C_GIT_DIR)/MQTTClient-C/src
mqtt3c_objs_mqttclient_c =
#	MQTTClient.c
#	MQTTClient.h

# INC_DIRS += $(mqtt3c_git_mqttclient_c_dir)


mqtt3c_git_freertos_dir = $(mqtt3c_git_mqttclient_c_dir)/FreeRTOS
mqtt3c_objs_freertos =
# none at this time -- see also $(MQTTCLIENT_PLATFORM_DIR)
# INC_DIRS += $(mqtt3c_git_freertos_dir)


############################################
# Upstream sources to be patched or copied #
############################################

#
# WARNING:
# Copy the associated .c files even if they are unmodified
# Otherwise compiler may pick up the original version of the .h file
# 

# No trailing slashes on MQTT3C_*_DIR

MQTT3C_GEN_DIR     := $(BUILD_DIR)paho-embed-mqtt3c_gen
mqtt3c_gen_work_dir = $(MQTT3C_GEN_DIR)/work
mqtt3c_gen_src_dir  = $(MQTT3C_GEN_DIR)/src
mqtt3c_gen_obj_dir  = $(MQTT3C_GEN_DIR)/obj

#
# Note that the basenames in mqtt3c_gen_originals must be unique
# Subdirectories are not created for them at this time
#
mqtt3c_gen_originals = \
	$(mqtt3c_git_mqttclient_c_dir)/MQTTClient.c \
	$(mqtt3c_git_mqttclient_c_dir)/MQTTClient.h

ifeq ($(MQTTCLIENT_PLATFORM_HEADER),MQTTFreeRTOS.h)
mqtt3c_gen_originals += \
	$(mqtt3c_git_freertos_dir)/MQTTFreeRTOS.c \
	$(mqtt3c_git_freertos_dir)/MQTTFreeRTOS.h
endif

mqtt3c_gen_filenames = $(notdir $(mqtt3c_gen_originals))

mqtt3c_gen_headers = \
	$(addprefix $(mqtt3c_gen_src_dir)/,$(filter %.h,$(mqtt3c_gen_filenames)))
INC_DIRS += $(mqtt3c_gen_src_dir)


mqtt3c_gen_objects = \
	$(addprefix $(mqtt3c_gen_obj_dir)/,$(patsubst %.c,%.o,$(filter %.c,$(mqtt3c_gen_filenames))))


# args for passing into compile rule generation

# The component_compile_rules uses $(realpath file)
# which returns nothing if the file does not exist
# so it can't be used for yet-to-be-generated files

paho-embed-mqtt3c_INC_DIR =
paho-embed-mqtt3c_SRC_DIR = $(MQTTCLIENT_PLATFORM_DIR)
paho-embed-mqtt3c_EXTRA_SRC_FILES = \
	$(patsubst %.o,$(mqtt3c_git_mqttpacket_dir)/%.c,$(mqtt3c_objs_mqttpacket)) \
	$(patsubst %.o,$(mqtt3c_git_mqttclient_c_dir)/%.c,$(mqtt3c_objs_mqttclient_c)) \
	$(patsubst %.o,$(mqtt3c_git_freertos_dir)/%.c,$(mqtt3c_objs_freertos))


################################
# call component_compile_rules #
################################

$(eval $(call component_compile_rules,paho-embed-mqtt3c))


####################################################
# Add to CFLAGS to bring in right transport header #
####################################################

#
# Check the flag sets and add, where needed
#

$(foreach f,$(mqtt3c_platform_flag_checks),$(eval $(call mqtt3c_warn_conflicting_mph_flags,$(f))))

$(foreach f,$(mqtt3c_platform_flag_required),$(eval $(call mqtt3c_add_mph_flag,$(f))))


# Now can modify the archive content variables

MQTT3C_PATCH_DIR = $(paho-embed-mqtt3c_ROOT)patches
mqtt3c_patches = $(wildcard $(MQTT3C_PATCH_DIR)/*)

paho-embed-mqtt3c_AR_IN_FILES += $(mqtt3c_gen_objects)

$(paho-embed-mqtt3c_AR): $(mqtt3c_gen_objects)

# Generate headers before they might be used

$(PROGRAM_OBJ_FILES): $(mqtt3c_gen_headers)

$(paho-embed-mqtt3c_OBJ_FILES): $(mqtt3c_gen_headers)

$(mqtt3c_gen_objects): $(mqtt3c_gen_headers)



####################
# Generate sources #
####################

#
# As patch can output to another directory, tempting to not do the
# copy of originals at all However, having them as a build artifact,
# at least for now, will likely ease debugging
#

#
# Copy originals to work dir
#

define copy_originals_to_work_dir
$(mqtt3c_gen_work_dir)/$(notdir $(orig)): $(orig)
	@echo "===> GEN: $$@"
	@mkdir -p $(mqtt3c_gen_work_dir)
	@cp -vp $$< $$@
endef

$(foreach orig,$(mqtt3c_gen_originals),$(eval $(copy_originals_to_work_dir)))


#
# Patch from copied originals
#

# As slick as patch -o seemed to be at first
# it seems to create a zero-size output if the patch is empty

# Do *not* use cp -p here as the new timestamp indicates "patch done"

$(mqtt3c_gen_src_dir)/%: $(mqtt3c_gen_work_dir)/% \
				$(MQTT3C_PATCH_DIR)/%.patch
	@echo "===> GEN: $@"
	@mkdir -p $(mqtt3c_gen_src_dir)
	@if [ -s $(MQTT3C_PATCH_DIR)/$(@F).patch ] ; then  \
	    patch -i $(MQTT3C_PATCH_DIR)/$(@F).patch -o $@ $< ; \
	    echo using $(MQTT3C_PATCH_DIR)/$(@F).patch ; \
	else \
	    echo patch file nonexistent or of zero length, using cp -p ; \
	    cp -v $(mqtt3c_gen_work_dir)/$(@F) $@ ; \
	fi
	@if [ ! -s "$@" ] ; then \
	    echo "**** GEN: Not generated;" $@ \
	         "does not exist, or is of zero length" ; \
	    false ; \
	fi
#
# Build .o and .d from patched sources
#
# Replicates build rule in component.mk
#

$(mqtt3c_gen_obj_dir)/%.o: $(mqtt3c_gen_src_dir)/%.c
	@echo "CC: $<"
	@mkdir -p $(mqtt3c_gen_obj_dir)
	$(paho-embed-mqtt3c_CC_BASE) $(paho-embed-mqtt3c_CFLAGS) -c $< -o $@
	$(paho-embed-mqtt3c_CC_BASE) $(paho-embed-mqtt3c_CFLAGS) -MM -MT $@ -MF $(@:.o=.d) $<

#
# End generate sources
#

# This check has to be made *after* call component_make_rules
ifeq ("$(wildcard $(MQTT3C_GIT_DIR)/.git)","")
    $(warning "paho.mqtt.embedded-c git checkout missing.  Perhaps run 'git submodule update --init'")
    $(info $(MQTT3C_GIT_DIR)/.git)
endif


# Debug targets

print-%:
	@echo "$* = $($*)"

return-%:
	@echo "$($*)"
