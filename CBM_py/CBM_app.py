#!/usr/bin/python

# *********************************************************************************
# Copyright(c) 2018-2020 Analog Devices, Inc. All Rights Reserved.

# This software is proprietary to Analog Devices, Inc. and its licensors.
# By using this software you agree to the terms of the associated Analog Devices
# License Agreement.
# *********************************************************************************/

"""
CBM 3-axis: Module to connect to a Dust Networks SmartMesh manager, create connections
to SmartMesh motes, and plot time and fft data to a Tkinter GUI. Written using the
dust networks SmartMesh API.

Terminal Mode (Non-GUI) and handshaking with radio added in version 1.05

@authors: Christian Aaen, Dara O'Sullivan, Tom Sharkey, Jack McCarthy
@last-modified: 23/7/2021
@version: v1.05
"""

# ============================ adjust path =====================================

import os
import sys

if __name__ == "__main__":
    here = sys.path[0]
    sys.path.insert(0, os.path.join(here, 'libs'))
    sys.path.insert(0, os.path.join(here, 'external_libs'))

    ## Testing master branch change

# ============================ imports =========================================

import time
import threading
import traceback

from SmartMeshSDK.IpMgrConnectorSerial import IpMgrConnectorSerial
from SmartMeshSDK.IpMgrConnectorMux import IpMgrSubscribe
from SmartMeshSDK.ApiException import APIError

from Tkinter import *
import Tkinter as tk
import ttk
import tkMessageBox

import matplotlib

matplotlib.use("agg")  # To be tested - Solves closing crash issued caused by TkAgg, the default
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib import style

from math import sqrt, cos, sin, fabs
from math import radians
from scipy.stats import kurtosis, skew
import pandas as pd
import sqlite3

global offset_removal_method
global T_frame_count
global moteStartAckd
T_frame_count = 0
moteStartAckd = False

# ============================ data ============================================
Vcc = 3.3  # Supply Voltage
V_perg_ref = 0.04  # V per g at reference voltage
Vref = 1.8  # 1.8V reference voltage
ADC_res = 16  # No of bits resolution
ADC_offset = 2 ** (ADC_res - 1)
V_per_LSB = Vcc / 2 ** ADC_res  # V per LSB resolution
V_perg_Vcc = V_perg_ref * Vcc / Vref  # Volts per g at Vcc
ADC_gain = V_per_LSB / V_perg_Vcc  # Gain of ADC
f_adc = 6.5e6  # ADC clock frequency
n_acq = 185  # Acq. time in clock periods


def getAdcCodeFromGravityValue(G):
    """Input the desired gravity value toecalculate the corresponding ADC code"""
    adc_code = G/ADC_gain+ADC_offset
    return round(adc_code)


LARGE_FONT = ("Verdana", 12)

VERSION = 1.05
py_version = int(VERSION * 100 - int(VERSION) * 100)  # version after decimal
##bin_version     = bin(version).replace("0b","") # 1.02 float -> 10 binary string


# ============================ mote function variables ============================================

save_frames = 0  # Keeps track of frames left to be saved to SQL database

FLASH_PAGE_SAMPLES = 1024  # How many samples can be fit on a page of the external flash

# ==============================================================================
# Class for handling SmartMesh manager information
# ==============================================================================

class Manager:

    def __init__(self):
        self.ID = 0
        self.connector = IpMgrConnectorSerial.IpMgrConnectorSerial()
        self.mac = []  # Is this var returned in the connector call? Probably some API to get it
        self.motes = {}  # Dictionary connecting mac address to mote objects and their plots
        self.num_motes = 0
        self.connected = False
        self.net_config = None
        self.desired_base_bw = 500  # 500ms inter-packet-delay
        self.operationalMacs = []
        self.macList = []
        self.subscriber = None
        self.lights = []

        # Network Config Information
        self.netID = None
        self.max_motes = None
        self.base_bw = None
        self.UDP_PORT_NUMBER = 61624

        # Default variables for sampling parameters
        self.adc_num_samples = 512
        self.adc_param_len = 2 
        #self.adc_param_len = 1  # NOTE: This has been changed from 2 to 1. It is counted like a sample which is 16bits not 2*16
        self.sampling_frequency = 512
        self.sleep_dur = 0  # Continual streaming by default

        # Default mote parameters in firmware readable format
        self.sampling_frequency_msg = "512xxxxx"
        self.alarm                  = "0xxxxxxx"
        self.axis_sel_msg           = "111xxxxx"
        self.sleep_dur_msg          = "0xxxxxxx"
        self.adc_num_samples_msg    = "512xxxxx"

    def __repr__(self):
        return repr(tuple(self.mac))

    #################### Connection and Data Handling Functions ################################
    def connect(self, *args):
        if self.connected:
            return
        
        if not TerminalMode:
           str_in = comportEntry.get()  # Put user input in string variable
           try:
               self.connector.connect({'port': str_in})
           except:
               tkMessageBox.showinfo("Error", "Unsuccessful Connection Attempt: Check COM port is the correct one")
               return
        else:
           try:
               print("Connecting to: {}".format(T_com_port))
               self.connector.connect({'port': T_com_port})
           except:
               print("Error: Unsuccessful Connection Attempt: Check COM port is the correct one. Shutting Down")
               quit()
               return
   

        self.net_config = self.connector.dn_getNetworkConfig()  # Update network variables
        # Change the default base_bw for all motes in a network here instead of having them 
        # take time to request it
        if self.net_config.baseBandwidth != self.desired_base_bw:
            self.change_base_bw(self.desired_base_bw)

        print("Connected and subscribed to data notifications ")
        self.connected = True
        self._get_motes()
        self._sub_to_mote_data()
        if not TerminalMode:
            self._set_indicators()
            controller.mote_plot_init()
        else:
            for i in range(mgr.num_motes):
                self.motes[tuple(mgr.operationalMacs[i])] = (Mote(mgr.adc_num_samples), (i))


    def change_base_bw(self, base_bw):
        print("Base BW prior to change = ", self.net_config.baseBandwidth)

        rc = self.connector.dn_setNetworkConfig( 
                                                networkId        = self.net_config.networkId        ,
                                                apTxPower        = self.net_config.apTxPower        ,
                                                frameProfile     = self.net_config.frameProfile     ,
                                                maxMotes         = self.net_config.maxMotes         ,
                                                baseBandwidth    = base_bw                          ,
                                                downFrameMultVal = self.net_config.downFrameMultVal ,
                                                numParents       = self.net_config.numParents       ,
                                                ccaMode          = self.net_config.ccaMode          ,
                                                channelList      = self.net_config.channelList      ,
                                                autoStartNetwork = self.net_config.autoStartNetwork ,
                                                locMode          = self.net_config.locMode          ,
                                                bbMode           = self.net_config.bbMode           ,
                                                bbSize           = self.net_config.bbSize           ,
                                                isRadioTest      = self.net_config.isRadioTest      ,
                                                bwMult           = self.net_config.bwMult           ,
                                                oneChannel       = self.net_config.oneChannel
                                               )
        if rc[0] == 0:
            print("Base BW changed successfully")
        else:
            print("Base BW NOT changed Successfully")

        self.net_config = self.connector.dn_getNetworkConfig()  # Update network variables
        print("Base BW after change = ", self.net_config.baseBandwidth)


    def disconnect(self, *args):
        global cv
        global moteStartAckd
        moteStartAckd = False
        if not self.connected:
            return
        self.connector.disconnect()
        self.operationalMacs = []
        self.connected = False
        for light in self.lights:
            cv.itemconfig(light, fill='red')
        Mote.ID = 0
        self.num_motes = 0
        if not TerminalMode:
            controller.disconnect()  # Called to disable plot animation

        print("Successfully Disconnected\n")

    def handle_notifdata(self, notifName, notifParams):
        '''Parse mote packet for timestamp and data and pass
        to handle_packet function

        @description: handle_notifdata parses Mote packets received.
        Checks the list of attached motes to verify the packet has come from verified mac address
        Parses packet into timestamp and data and passes info to handle_packet method of mote
        Packets are parsed by: handle_notifdata -> mote.handle_packet -> mote._handle_bytes

        @params
        notifName: ID data from header of smartmesh packet, specifying type of data: Notification,
        notifParams: Structure holding smartmesh message data and metadata, time sent, data

        @returns None '''

        mac = tuple(notifParams.macAddress)
        if mac not in self.motes:
            print "MAC not recognised"
            return
        timestamp = notifParams.utcSecs + 1e-6 * notifParams.utcUsecs
        self.motes[mac][0].handle_packet((timestamp, notifParams.data))

    def _sub_to_mote_data(self):
        self.subscriber = IpMgrSubscribe.IpMgrSubscribe(self.connector)
        self.subscriber.start()
        self.subscriber.subscribe(
            notifTypes=[
                IpMgrSubscribe.IpMgrSubscribe.NOTIFDATA,  # Subscribe to notification data only
            ],
            fun=self.handle_notifdata,
            isRlbl=False,
        )
        for entry in self.motes.values():
            print("Updating adc_num_samples to ", self.adc_num_samples)
            mote = entry[0]
            mote.update(self.adc_num_samples)

    def _get_motes(self):
        print("- retrieve the list of all connected motes")
        continueAsking = True
        currentMac = (0, 0, 0, 0, 0, 0, 0, 0)
        while continueAsking:  # While mote remains, add mote mac to operationalMacs
            try:
                res = self.connector.dn_getMoteConfig(currentMac, True)
            except APIError:
                continueAsking = False
            else:
                if ((not res.isAP) and (res.state in [4, ])):  # Not access point and in operational state
                    self.operationalMacs += [res.macAddress]
                currentMac = res.macAddress
        self.num_motes = len(self.operationalMacs)
        self.macList = []

        print("Found the following operational motes:")
        for mac in self.operationalMacs:
            self.macList.append(self.pretty_mac(mac))
            print(self.pretty_mac(mac))

        if len(self.macList) == 0:
            print "    No operational motes found"
            if not TerminalMode:
                print "Please press 'disconnect', followed by 'connect'"

    def calc_sampling(self, *args):
        # This function calculates the values of the x-axes for time series and FFT

        n_acq = (float(f_adc) / (float(self.sampling_frequency)) - 14)
        fs = f_adc / (14 + n_acq)
        f = [(fs / self.adc_num_samples) * x for x in range(0, self.adc_num_samples / 2)]
        t = [x / fs for x in range(0, self.adc_num_samples - self.adc_param_len)]  # Sample per frequency
        #print("------ CALC SAMPLING------")
        #print("f_max = {}".format(f[0]))
        return (f, t)

    ######################## Functions to send mote commands and messages #######################
    def send_to_mote(self, mac, message, prnt=True):
        '''Sends message to be received by mote
        Message can be read in mote module SmartMeshRFcog.c in RCV section'''
        try:
            rc = self.connector.dn_sendData(macAddress=mac,
                                            priority=0,
                                            srcPort=self.UDP_PORT_NUMBER,
                                            dstPort=self.UDP_PORT_NUMBER,
                                            options=0,
                                            data=[ord(i) for i in message])
            if (rc[0] == 0 and prnt):
                print("Sent command to {} successfully".format(self.pretty_mac(mac)))
            elif rc[0] != 0:
                print("Unsuccessful command to {}".format(self.pretty_mac(mac)))
        except:
            print("ERROR on send. Mote not connected")
            traceback.print_exc()

    def send_to_all_motes(self, message, prnt=True):
        if (self.num_motes > 0):
            for mac in self.operationalMacs:
                self.send_to_mote(mac, message, prnt)

    def send_sampling_parameters(self, alarm="0xxxxxxx"):
        '''Sends msg to motes to update parameters, by default sets alarm trigger to off'''
        
        cmd_descriptor = "22xxxxxx"  # This tells MCU that the packet contains sampling parameters

        sampling_message = self.sampling_frequency_msg + self.alarm         + self.axis_sel_msg \
                         + self.adc_num_samples_msg    + self.sleep_dur_msg + cmd_descriptor
        
        self.send_to_all_motes(sampling_message)
        print("Sending sampling parameters with msg {}".format(sampling_message))

    def send_axis_info(self, *args):
        '''Sends smartmesh packet to all motes with updated axis select message
        axis_sel_msg: 111 -> Send x,y,z
        axis_sel_msg: 101 -> Send x, ,z'''
        
        if not TerminalMode:
            self.axis_sel_msg = str(v1.get()) + str(v2.get()) + str(v3.get()) +"xxxxx"
        else:
            self.axis_sel_msg = T_axis_en

        cmd_descriptor = '33xxxxxx'
        msg = 'xxxxxxxx' + '0xxxxxxx' + self.axis_sel_msg + 'xxxxxxxx' + 'xxxxxxxx' + cmd_descriptor
        self.send_to_all_motes(msg)

    def get_user_sampling_parameters(self, *args):
        '''Get user selection from sampling comboboxes
        Format msg to send sampling parameters to all motes'''

        try:
            if not self.connected:
                return

            if not TerminalMode:
                # Get parameters from combobox
                self.sampling_frequency_str = frequencyCombo.get()
                self.adc_num_samples_str    = adcNumSamplesCombo.get()
                self.sleep_dur_str          = sleepDurCombo.get()
            else:
                self.sampling_frequency_str = T_samp_freq
                self.adc_num_samples_str    = T_num_samples
                self.sleep_dur_str          = T_sleep_s

            # Wait until all sampling parameters have been entered before sending, otherwise strings will be null
            # User must press "OK" button"
            self.sampling_frequency = int(self.sampling_frequency_str)
            self.adc_num_samples    = int(self.adc_num_samples_str)
            self.sleep_dur          = int(self.sleep_dur_str)

            # Update the expected number of samples for each mote
            for entry in self.motes.values():
                print "Updating adc_num_samples to ", self.adc_num_samples
                mote = entry[0]
                mote.update(self.adc_num_samples)

            # Always fills arrays to 8-bit width
            # Example:
            # sampFrequency = 5000xxxx
            self.sampling_frequency_msg = self.sampling_frequency_str + ('x' * (8 - (len(self.sampling_frequency_str))))
            self.adc_num_samples_msg    = self.adc_num_samples_str + ('x' * (8 - (len(self.adc_num_samples_str))))
            self.sleep_dur_msg          = self.sleep_dur_str + ('x' * (8 - (len(self.sleep_dur_str))))

            self.send_sampling_parameters(self.alarm)
            if not TerminalMode:
                controller.f, controller.t = self.calc_sampling()

        except ValueError:
            traceback.print_exc()
            print("Invalid sampling settings input. Please enter a valid integer.")

    def alarm_triggered(self, *args):
        '''Send a message with no changes to sampling parameters, but alarm set to "1"'''
        cmd_descriptor = '44xxxxxx'
        msg = 'xxxxxxxx' + '1xxxxxxx' + 'xxxxxxxx' + 'xxxxxxxx' + 'xxxxxxxx' + cmd_descriptor
        self.send_to_mote(mgr.operationalMacs[self.ID], msg, False)

    ## IN mgr class
    def set_alarm(self, *args):
        for entry in self.motes.values():
            mote = entry[0]
            if var == 1: mote.peak_alarm = float(peak_entry.get())
            if var == 2: mote.p2p_alarm = float(p2p_entry.get())
            if var == 3: mote.rms_alarm = float(rms_entry.get())
            if var == 4: mote.std_alarm = float(std_entry.get())
            if var == 5: mote.kurt_alarm = float(kurts_entry.get())
            if var == 6: mote.skew_alarm = float(skew_entry.get())
            if var == 7: mote.crest_alarm = float(crest_entry.get())
            mote.alarm_flag = 1
        print("Alarm set")

    def reset_alarm(self, *args):
        '''Send default msg with alarm value set to 0 to reset mote alarm LED'''
        cmd_descriptor = '55xxxxxx'
        msg = 'xxxxxxxx' + '0xxxxxxx' + 'xxxxxxxx' + 'xxxxxxxx' + 'xxxxxxxx' + cmd_descriptor
        self.send_to_all_motes(msg, prnt=False)
        for entry in self.motes.values():
            mote = entry[0]
            mote.alarm_flag = 0
        print("Alarm off, motes reset")

    ##################################### Helper functions ######################################
    def _set_indicators(self):

        def get_light_pos(n):
            '''Method to calculate position of mote indicators,
            based on number of connected motes'''
            ang = 360 // n
            r_ang = radians(ang)
            light_pos = list()
            for i in range(0, n):
                x = round(30 * -(sin(i * r_ang)))
                y = round(30 * -(cos(i * r_ang)))
                light_pos.append((x, y))
            return light_pos

        def center_to_coords(center):
            '''Converts center of circle values for mote indicators to 4
            coordinates needed for plotting
            '''
            indent_x = 125
            indent_y = 50

            width = 10
            height = 10

            x0 = indent_x + center[0] - width // 2
            x1 = indent_x + center[0] + width // 2
            y0 = indent_y + center[1] - width // 2
            y1 = indent_y + center[1] + width // 2

            return (x0, y0, x1, y1)  # Remember to unpack tuple in create oval option

        if self.num_motes == 0:
            return
        positions = get_light_pos(self.num_motes)
        for i, pos in enumerate(positions):
            co_ords = center_to_coords(pos)
            self.lights.append(cv.create_oval(*co_ords, fill='green'))

    def get_network_info(self):
        '''Retrieves network info from manager and shows to user as tkMessageBox'''

        try:
            netConList = list(self.net_config)

            # == Variables
            self.netID = netConList[1]
            self.max_motes = netConList[4]
            self.base_bw = netConList[5]

            network_str = "NetworkID = {0}\nMax Motes = {1}\nBase Bandwidth = {2}".format(self.netID, self.max_motes,
                                                                                          self.base_bw)
            info_str = ""
            for mac in self.macList:
                info_str += "\nMote {}".format(mac)
            return tkMessageBox.showinfo("Network info: ",
                                         "Manager Info\n{} \n\nMac Addresses: {}".format(network_str, info_str))
        except:
            traceback.print_exc()
            print("Not connected. You must be connected to display network information.")

    def show_help(self):
        return tkMessageBox.showinfo(
            "Help", '''------------------------------Info-------------------------------             \
            \n\nMode: Used to select output of graph on right-hand-side. Default is DFT.             \
            \n\nReset: Clears data for all graphs used in the mode section.                          \
            \n\nAction: Provides functionality to save data. User designates number of frames saved. \
            \n\nHelp: Provides further details on network information and application function.      \
            \n\n---------------------------Disclaimer-------------------------------                 \
            \nPlease note that the application is designed only as a proof of concept.               \
            \nIt is by no means fully completed. Use at your own risk.
                                ''')

    def pretty_mac(self, mac):
        pMac = '-'.join(['%02x' % b for b in mac])
        return pMac


# ==============================================================================
# Class for handling mote data
# ==============================================================================
class Mote:
    _packet_queue_max = 0
    colors = {0: 'salmon', 1: 'orchid', 2: 'lightskyblue'}  # Colors for plotting x,y,z
    ID = 0  # Incremented for each mote that is created

    def __init__(self, num_samples):
        self.mac = ()
        self._data_valid = False
        self._rx_cnt = 0  # Count of received packets
        self._x_offset = 0
        self._y_offset = 0
        self._z_offset = 0
        self._offset_removed = {'x': False, 'y': False, 'z': False}
        self._packet_queue = []
        self._lock = threading.Lock()

        # data buffer for num_samples for raw samples and num_samples/2 for fft magnitude
        self._data1 = ((3 * num_samples / 2) + mgr.adc_param_len) * [0]
        self._data2 = ((3 * num_samples / 2) + mgr.adc_param_len) * [0]

        # Ping-pong buffer, _data_order[0] = currently filling
        self._data_order = (self._data1, self._data2)
        self._num_samples = num_samples

        # Mote variables used for plotting
        self.ID = Mote.ID
        Mote.ID += 1
        self.color = self.colors[self.ID % 3]
        self.scale = [0.75, 0.75, 0.75]

        # Alarm setting
        self.alarm_flag = 0
        self.peak_alarm = 0
        self.p2p_alarm = 0
        self.rms_alarm = 0
        self.std_alarm = 0
        self.kurt_alarm = 0
        self.crest_alarm = 0
        self.skew_alarm = 0

        # Initialise data plotting lists
        #            [x, y, z]
        self._peak = [0, 0, 0]
        self._p2p = [0, 0, 0]

        # Stat list for each axis: x, y, z;
        self._peaks = [[0], [0], [0]]
        self._p2ps = [[0], [0], [0]]
        self._means = [[0], [0], [0]]
        self._sds = [[0], [0], [0]]
        self._kurts = [[0], [0], [0]]
        self._skews = [[0], [0], [0]]
        self._crests = [[0], [0], [0]]

        self._x_reset = False
        self._y_reset = False
        self._z_reset = False

        self.motor_speed = []
        self.motor_drawn = False
        self.motor_clear = False
        self.motor_list = []
        self.motor_text = []

        self.bpfi = 0
        self.bpfo = 0
        self.bpf_drawn = False
        self.bpf_clear = False
        self.bpf_list = []
        self.bpf_text = []

        self.peak_drawn = [False, False, False]
        self.peak_text = [None, None, None]
        self.p2p_drawn = [False, False, False]
        self.p2p_text = [None, None, None]

        self.remove_offsets = False
        self.offset = 0  # Default offset for all axis

    def __repr__(self):
        return repr(self.ID)

    ################  Core data handling functions for individual mote packets  #################
    def _handle_bytes(self, bytes):
        '''Aligns incoming SmartMesh bytes to form frames for analysis and visualisation
           Raw and fft variables are filled when method completes

           @description Does not return anything but updates data_order[0] with current data.
           Fills math lists (peaks, means) with raw data while we have the current frame to
           avoid timing issues

           @param bytes: 90 byte packet passed down from handle_packet
           @returns: None
        '''
        global T_frame_count
        global moteStartAckd

        try:

            # 0xAA, 0xBB, 0xCC, 0xDD
            startAck = bytes[0] == 170 and \
                       bytes[1] == 187 and \
                       bytes[2] == 204 and \
                       bytes[3] == 221      

            # Below lines are used to debug mote when not connected to debugger
            if False:
                if bytes[0] == 10 and bytes[1] == 11:
                    print "NEW PARAM MGR READY"

                if bytes[0] == 11 and bytes[1] == 12:
                    print "ACQUIRING"

                if bytes[0] == 12 and bytes[1] == 13:
                    print "GETTING DATA"

                if bytes[0] == 13 and bytes[1] == 13:
                    print "STARTING TX"

                if bytes[0] == 13 and bytes[1] == 14:
                    print "FINISHED TX"

                if bytes[0] == 14 and bytes[1] == 15:
                    print "AWAKE"

                if bytes[0] == 10 and bytes[1] == 10:
                    print "CALCULATING FFT"

                if bytes[0] == 11 and bytes[1] == 11:
                    print "SAMPLES ACQUIRED"

            if startAck:
                # The mote has acknowledge the start signal from the GUI    
                print "Mote has advertised it is ready"
                if TerminalMode:
                    mgr.get_user_sampling_parameters()  # This will also tell Mote that Mgr is ready
                else:
                    #sendReadyMsg()
                    mgr.send_sampling_parameters()

                moteStartAckd = True
            else:
                cur_data = self._data_order[0]
                data_len = len(bytes) / 2
                msb_set = bytes[1] == 255  # MSB of first word is alignment bit

                if self._rx_cnt + data_len > len(cur_data):  # Restart if we get more data in a frame than expected
                    print("[{} {}] Restarting after data overflow. Expected {}. Rxd {}".format(id(self), self._rx_cnt, len(cur_data), self._rx_cnt + data_len))
                    self._rx_cnt = 0

                if self._rx_cnt > 0 and msb_set:  # Restart if we get an alignment bit in the middle of a frame
                    print("[{}] Restarting after partial frame".format(id(self)))
                    print("Rx_cnt {}, cur_data {}".format(self._rx_cnt, len(cur_data)))
                    self._rx_cnt = 0

                if self._rx_cnt == 0 and not msb_set:  # Align to the start of a new frame
                    print("[{}] Waiting for start of new frame".format(id(self)))
                    return  # Continue count until full frame is reached

                if msb_set and self._rx_cnt == 0:
                    self.frame_start_time = time.clock()
                    # Send ready message to mote. If this stops being sent then sampling will stop
                    #sendReadyMsg()
                    mgr.send_sampling_parameters()
                    print("[{}] Frame beginning @ {}".format(id(self), self.frame_start_time))

                if (msb_set):
                    # Check python and firmware versions match
                    c_version = int(bytes[0] & 0x0F)
                    version_match = int(c_version) == int(py_version)
                    if not version_match:
                        print("Version mismatch - GUI version: {}, Firmware version:{}".format(py_version, c_version))

                self._lock.acquire()

                for i in range(0, len(bytes), 2):
                    cur_data[self._rx_cnt] = (bytes[i + 1] << 8) | bytes[i]  # Convert from bytes to words
                    self._rx_cnt = self._rx_cnt + 1

                #print("Rx Count: ", self._rx_cnt)

                frame_got = False  # True if we have entered the next if statement, avoids double release of lock
                # Are we done with frame?
                if self._rx_cnt == len(cur_data):

                    T_frame_count += 1

                    self.frame_end_time = time.clock()
                    self._data_order = (self._data_order[1], self._data_order[0])
                    self._data_valid = True
                    self._rx_cnt = 0
                    self._lock.release()
                    frame_got = True

                    # This where we have a complete frame, update all our stats here
                    raw, fft, ax = self.latest_valid_data()

                    print("[{}] {} axis frame complete @ {}".format(id(self), ax, self.frame_end_time))
                    print("[{}] Time between first and last packets = {}s\n".format(id(self), self.frame_end_time - self.frame_start_time))

                    # Choose which stat axis list to store data: x,y,z
                    axis_dict = {'x': 0,
                                 'y': 1,
                                 'z': 2}
                    if ax is not None:
                        axis_index = axis_dict[ax]
                        self._peaks[axis_index].append(peak(raw))
                        self._p2ps[axis_index].append(peak_to_peak(raw))
                        self._means[axis_index].append(rms(raw))
                        self._sds[axis_index].append(standard_deviation(raw))
                        self._kurts[axis_index].append(kurtosis(raw))
                        self._skews[axis_index].append(skew(raw))
                        self._crests[axis_index].append(crest(raw))

                    # Setting alarm stuffs
                    if not TerminalMode:
                        if (v and self.alarm_flag):
                            self.check_alarm_flags(raw)

                    # Setting peak, p2p and scale values
                    for i, p in enumerate(self._peak):
                        local_peak = max(self._peaks[i])
                        if local_peak > p:
                            self._peak[i] = local_peak
                        if local_peak > self.scale[i]: 
                            self.scale[i] = local_peak * 1.2

                    for i, p2p in enumerate(self._p2p):
                        local_p2p = max(self._p2ps[i])
                        if local_p2p > p2p:
                            self._p2p[i] = local_p2p

                    if all(self._peaks):  # Ensure we have values for all axis'
                        save_to_file(self, self.ID, ax, raw, fft)

                    if self.remove_offsets:
                        self.remove_offset(method=offset_removal_method)
                        if self._offset_removed.values() == [True, True, True]:
                            self.remove_offsets = False


                if not frame_got:  # Ensure threading lock not released twice
                    self._lock.release()
        except:
            print(traceback.print_exc())

    def handle_packet(self, packet):
        '''Orders incoming mote packets by timestamp for processing

        @description
        Adds smartmesh packet to queue, sorts by latest timestamp
        and passes earliest timestamp packet to _handle_bytes. Packets must
        be sorted by timestamp and not processed in the order they arrive, as
        the nature of the "mesh" type network could result in older packets
        being processed first

        @param
        Packet - Smartmesh packet from mote with time and adc_data

        @returns None
        '''

        self._packet_queue.append(packet)  # packet -> (time, data)
        self._packet_queue.sort(key=lambda entry: entry[0], reverse=True)  # Sort by latest timestamp

        #print "Packet"
        if moteStartAckd:
            if len(self._packet_queue) > self._packet_queue_max:
                self._handle_bytes(self._packet_queue.pop()[1])  # Process and remove most recent data byte
        else:
            self._handle_bytes(self._packet_queue.pop()[1])  # Process and remove most recent data byte
    
    def latest_valid_data(self, withOffset=False):
        '''Uses threading lock to return current frame of raw, fft and axis data

        @description
        Returns most recent data frame raw,fft,axis (x, y or z)
        The inital offset relates to how adc readings are translated from voltage to binary value
        The adc values translate 0-3.3V -> 0-65535 with 1.65V or 32768 corresponding to 0g acceleration
        For plotting, this mean value must be removed

        @params
        withOffset->bool: if True, returns the raw and fft data without the offset removed

        @returns
        raw,fft,ax tuple: Complete "frame" of SmartMesh data
        '''

        if not self._data_valid:  # _handle_bytes has not yet been called
            return (None, None, None)

        with self._lock:
            params = self._data_order[1][0]  # Info on axis - x,y,z
            raw = self._data_order[1][mgr.adc_param_len:mgr.adc_num_samples]

            if mgr.adc_num_samples <= FLASH_PAGE_SAMPLES:
                fft = self._data_order[1][mgr.adc_num_samples + mgr.adc_param_len:]
            else:
                fft = [0 for x in range(mgr.adc_num_samples//2)]

        x_set = (int(params & 0x00F0)) == 144
        y_set = (int(params & 0x00F0)) == 160
        z_set = (int(params & 0x00F0)) == 192

        if x_set:
            ax = 'x'
            self.offset = self._x_offset
        elif y_set:
            ax = 'y'
            self.offset = self._y_offset
        elif z_set:
            ax = 'z'
            self.offset = self._z_offset

        raw_avg = sum(raw)/len(raw)
        #print("{} axis: Average Raw Value = {}".format(ax, raw_avg))
        #print("{} axis: Average Raw Value - offset = {} (O={})".format(ax, raw_avg - self.offset, self.offset))

        if withOffset:
            # Just return raw codes
            pass
        else:
            raw = [(ADC_gain * (x - ADC_offset - self.offset)) for x in raw]

        raw_avg = sum(raw)/len(raw)
        #print("{} axis: Average G Value = {}".format(ax, raw_avg))

        fft = [2 * ADC_gain * x for x in fft]

        return (raw, fft, ax)


    def remove_offset(self, method=1):
        '''Updates mote offset variables for each axis, x,y,z by calculating mean for each
           If method==1 then it is assumed the mote is in the perfectly upright position i.e. X,Y=0, Z=1
           If method==2 then the average values of 1 frame is taken off all subsequent frames'''
        if not self._data_valid:
            return

        raw_w_offset, fft, ax = self.latest_valid_data(withOffset=True)

        if method == 1:
            # Assumes G in X,Y=0 and G in Z=1
            # Offset is difference between average reading and ideal code for desired G value
            if ax == 'x' and self._offset_removed[ax]==False:
                ideal_code = getAdcCodeFromGravityValue(0)
                self._x_avg0G  = sum(raw_w_offset) / len(raw_w_offset)
                self._x_offset = self._x_avg0G - ideal_code
                print("Updating X axis offset to {}. M1".format(self._x_offset))

            if ax == 'y' and self._offset_removed[ax]==False:
                ideal_code = getAdcCodeFromGravityValue(0)
                self._y_avg0G  = sum(raw_w_offset) / len(raw_w_offset)
                self._y_offset = self._y_avg0G - ideal_code
                print("Updating Y axis offset to {}. M1".format(self._y_offset))

            if ax == 'z' and self._offset_removed[ax]==False:
                ideal_code = getAdcCodeFromGravityValue(1)
                self._z_avg0G  = sum(raw_w_offset) / len(raw_w_offset)
                self._z_offset = self._z_avg0G - ideal_code
                print("Updating Z axis offset to {}. M1".format(self._z_offset))
            
            self._offset_removed[ax] = True

        elif method == 2:
            # Calculate average values of 1 frame and take them off all subsequent frames
            if ax == 'x' and self._offset_removed[ax]==False:
                self._x_offset = sum(raw_w_offset) / len(raw_w_offset) - ADC_offset
                print("Updating X axis offset to {}. M1".format(self._x_offset))
            if ax == 'y' and self._offset_removed[ax]==False:
                self._y_offset = sum(raw_w_offset) / len(raw_w_offset) - ADC_offset
                print("Updating Y axis offset to {}. M1".format(self._y_offset))
            if ax == 'z' and self._offset_removed[ax]==False:
                self._z_offset = sum(raw_w_offset) / len(raw_w_offset) - ADC_offset
                print("Updating Z axis offset to {}. M1".format(self._z_offset))

            self._offset_removed[ax] = True
        else:
            print("Incorrect argument passed to remove_offset(). Offsets not removed")




    ##############################  Functions for mote updates  #################################
    def update(self, num_samples):
        '''Takes in new num_samples and updates mote values'''
        try:
            self._data_valid = False
            if num_samples <= FLASH_PAGE_SAMPLES:
                self._data1 = (mgr.adc_param_len + 3 * (num_samples / 2)) * [0]
                self._data2 = (mgr.adc_param_len + 3 * (num_samples / 2)) * [0]
            else:
                # No FFT data sent if flash is being used
                self._data1 = (mgr.adc_param_len + 2 * (num_samples / 2)) * [0]
                self._data2 = (mgr.adc_param_len + 2 * (num_samples / 2)) * [0]

            self._data_order = (self._data1, self._data2)
            self._packet_queue = []
            self._rx_cnt = 0
            self.alarm_flag = 0
        except:
            print(traceback.print_exc())

    def reset(self):
        '''Resets mote statistics to default'''

        self._peak = [0, 0, 0]
        self._p2p = [0, 0, 0]
        self.scale = [0.75, 0.75, 0.75]

        self._peaks = [[0], [0], [0]]
        self._p2ps = [[0], [0], [0]]
        self._means = [[0], [0], [0]]
        self._sds = [[0], [0], [0]]
        self._kurts = [[0], [0], [0]]
        self._skews = [[0], [0], [0]]
        self._crests = [[0], [0], [0]]


    def check_alarm_flags(self, raw):
        '''Calls alarm_triggered function if alarm value is exceeded

        @params
        raw - Raw data values used to calculate mote statistics

        @returns
        None
        '''
        flag = 0

        # Use dict to connect var to associated name, value and limit
        alarms_dict = {
            1: ("Peak", peak(raw), self.peak_alarm),
            2: ("Peak-to-Peak", peak_to_peak(raw), self.p2p_alarm),
            3: ("RMS", rms(raw), self.rms_alarm),
            4: ("Standard Deviation", standard_deviation(raw), self.std_alarm),
            5: ("Kurtosis", kurtosis(raw), self.kurt_alarm),
            6: ("Skew Factor", skew(raw), self.skew_alarm),
            7: ("Crest Factor", crest(raw), self.crest_alarm)
        }

        name, value, limit = alarms_dict[var]
        if value > limit:
            print("Alarm triggered on {}".format(name))
            mgr.alarm_triggered(self)  # Sends command to mote to turn LED red
            self.alarm_flag = 0  # Reset alarm to not trigger repeatedly


# ==============================================================================
# Class for handling animation of plots
# ==============================================================================
class AnimationController:
    NO_AXIS = 3

    def __init__(self, *args, **kwargs):
        self.mode = "DFT"
        self.t = []
        self.f = []
        self.axisSet = False

        # Animation variables
        self.interval = 3000  # Animation interval in ms
        self.animated = False

    ############################### Core plot init and animation functions ###################################
    def animate(self, i, line, ax, ID):
        '''
        Updates mote values and titles being plotted. Called by funcAnimation every "i" ms.

        @params
        i - interval between updates in ms
        x - xaxis data list
        y - yaxis data list
        lines - data array being plotted (x,y)
        ax - ONE raw + fft axis of ONE Mote

        @returns
        None
        '''

        try:
            entry = mgr.motes.values()[ID]
            mote = entry[0]
            raw, fft, axe = mote.latest_valid_data()
            ax = self.axs[ID]
            if raw is None:
                return

            fft[0] = 0.0  # Removing DC offset (required because offset not removed on firmware side, where fft is calculated

            if axe == 'x':
                line[0].set_data(self.t, raw)
                line[1].set_data(self.f, fft)
            elif axe == 'y':
                line[2].set_data(self.t, raw)
                line[3].set_data(self.f, fft)
            elif axe == 'z':
                line[4].set_data(self.t, raw)
                line[5].set_data(self.f, fft)

            self._set_right_graph(raw, self.mode, mote, line, fft, ax)  # Updates rightmost plots, for mote statistics
            self.animated = True
            if ax.any():  # If plotting has occured, reset axis limits
                try:
                    if not self.axisSet:  # Check user has not selected manual axis
                        for r in range(3):
                            for c in range(2):
                                ax[r][c].relim()
                                ax[r][c].autoscale_view()
                    else:
                        for r in range(3):
                            ax[r][0].relim()
                            ax[r][0].autoscale_view()
                except:
                    print("Could not rescale plots")
                    traceback.print_exc()

        except:
            print("Error in animate callback")
            traceback.print_exc()

    def _set_right_graph(self, raw, mode, mote, line, fft, ax):
        '''Choose plot to display on right hand side
        based on mode selected by user, DFT default

        @description

        @params
        raw - raw mote data
        mode - user selected plot to display - peak, mean, skew
        mote - mote object being plotted
        line -
        fft - fft mote data
        ax - x, y, or z axis

        @returns None
        '''

        if self.mode == "DFT":
            if len(self.f) == len(fft):
                ax[0, 1].set_title('DFT X, Y, Z')
                ax[2, 1].set_xlabel('Frequency [Hz]')

                if mote.motor_speed and not mote.motor_drawn:  # If user has set a "healthy" motor to be plotted
                    for i in range(self.NO_AXIS):
                        plt.sca(ax[i, 1])
                        for freq in mote.motor_speed:
                            texty = "f" + str(mote.motor_speed.index(freq) + 1)
                            mote.motor_list.append(
                                ax[i, 1].axvline(freq, color='rosybrown', linestyle='dashed', zorder=2))
                            mote.motor_text.append(plt.text(freq + 0.1, 0, texty, rotation=45))
                    mote.motor_drawn = True

                if mote.motor_clear:
                    for line in mote.motor_list:
                        line.remove()
                    mote.motor_list = []
                    for text in mote.motor_text:
                        text.remove()
                    mote.motor_text = []

                    mote.motor_clear = False
                    mote.motor_drawn = False

                if mote.bpfi != 0 and not mote.bpf_drawn:
                    for i in range(self.NO_AXIS):
                        plt.sca(ax[i, 1])
                        mote.bpf_list.append(
                            plt.axvline(mote.bpfo, color='steelblue', linestyle='dashed', label='BP', zorder=2))
                        mote.bpf_text.append(plt.text(mote.bpfo + 0.1, 0, "bpfo", rotation=45))
                        mote.bpf_list.append(
                            plt.axvline(mote.bpfi, color='steelblue', linestyle='dashed', label='BP', zorder=2))
                        mote.bpf_text.append(plt.text(mote.bpfi + 0.1, 0, "bpfi", rotation=45))

                    mote.bpf_drawn = True

                if mote.bpf_clear:
                    for line in mote.bpf_list:
                        line.remove()
                    mote.bpf_list = []
                    for text in mote.bpf_text:
                        text.remove()
                    mote.bpf_text = []
                    mote.bpf_drawn = False
                    mote.bpf_clear = False
                    mote.bpfi = 0
                    mote.bpfo = 0

        elif self.mode == "Peak":
            ax[0, 1].set_title('Peak X Y Z')
            ax[2, 1].set_xlabel('Time (s)')

            # Check for new max peak to redraw peak box
            for i in range(self.NO_AXIS):
                old_peak = 0
                new_peak = mote._peak[i]
                if new_peak > old_peak:
                    if mote.peak_text[i] is not None:
                        mote.peak_text[i].remove()
                    mote.peak_text[i] = None
                    mote.peak_drawn[i] = False

            for i in range(self.NO_AXIS):
                x = [d for d in range(len(mote._peaks[i]))]
                line[1 + (2 * i)].set_data(x, mote._peaks[i])
                if (mote.peak_drawn[i] == False):  # If current peak box is not drawn
                    plt.sca(ax[i, 1])
                    ax[i, 1].set_ylim([-mote.scale[i], mote.scale[i]])
                    mote.peak_text[i] = plt.text(0, mote.scale[i] * 0.9, "Peak= {:.4f}".format(mote._peak[i]),
                                                 wrap=True,
                                                 verticalalignment='top',
                                                 horizontalalignment='left',
                                                 bbox=dict(boxstyle='round', ec=mote.color, fc=(1, 1, 1)))
                    mote.peak_drawn[i] = True

        elif self.mode == "Peak-To-Peak":
            ax[0, 1].set_title('Peak-to-Peak X Y Z')
            ax[2, 1].set_xlabel('Time (s)')

            for i in range(self.NO_AXIS):
                old_p2p = 0
                new_p2p = mote._p2p[i]
                if new_p2p > old_p2p:
                    if mote.p2p_text[i] is not None:
                        mote.p2p_text[i].remove()
                    mote.p2p_text[i] = None
                    mote.p2p_drawn[i] = False

            for i in range(self.NO_AXIS):
                x = [d for d in range(len(mote._p2ps[i]))]  # Create simple x-axis
                line[1 + (2 * i)].set_data(x, mote._p2ps[i])
                if mote.p2p_drawn[i] == False:  # If current peak box is not drawn
                    plt.sca(ax[i, 1])
                    ax[i, 1].set_ylim([0, 2 * mote.scale[i]])
                    mote.p2p_text[i] = plt.text(0, mote.scale[i] * 1.8, "Peak-to-Peak= {:.4f}".format(mote._p2p[i]),
                                                verticalalignment='top',
                                                horizontalalignment='left',
                                                bbox=dict(boxstyle='round', ec=mote.color, fc=(1, 1, 1)))
                    mote.p2p_drawn[i] = True

        elif self.mode == "Mean":
            ax[0, 1].set_title('Means X Y Z')
            ax[2, 1].set_xlabel('Time (s)')
            for i in range(self.NO_AXIS):
                x = [d for d in range(len(mote._means[i]))]
                line[1 + (2 * i)].set_data(x, mote._means[i])

        elif self.mode == "Standard Deviation":
            ax[0, 1].set_title('Standard Deviation X Y Z')
            ax[2, 1].set_xlabel('Time (s)')
            for i in range(self.NO_AXIS):
                x = [d for d in range(len(mote._sds[i]))]
                line[1 + (2 * i)].set_data(x, mote._sds[i])

        elif self.mode == "Kurtosis":
            ax[0, 1].set_title('Kurtosis X Y Z')
            ax[2, 1].set_xlabel('Time (s)')
            for i in range(self.NO_AXIS):
                x = [d for d in range(len(mote._kurts[i]))]
                line[1 + (2 * i)].set_data(x, mote._kurts[i])

        elif self.mode == "Skew Factor":
            ax[0, 1].set_title('Skew Factor X Y Z')
            ax[2, 1].set_xlabel('Time (s)')
            for i in range(self.NO_AXIS):
                x = [d for d in range(len(mote._skews[i]))]
                line[1 + (2 * i)].set_data(x, mote._skews[i])

        elif self.mode == "Crest Factor":
            ax[0, 1].set_title('Crest Factor X Y Z')
            ax[2, 1].set_xlabel('Time (s)')
            for i in range(self.NO_AXIS):
                x = [d for d in range(len(mote._crests[i]))]
                line[1 + (2 * i)].set_data(x, mote._crests[i])

        if self.mode != "DFT":
            for line in mote.motor_list:
                line.remove()
                mote.motor_list = []
            for text in mote.motor_text:
                text.remove()
                mote.motor_text = []

            for line in mote.bpf_list:
                line.remove()
                mote.bpf_list = []
            for text in mote.bpf_text:
                text.remove()
                mote.bpf_text = []

            mote.motor_drawn = False
            mote.bpf_drawn = False

        if self.mode != "Peak":
            for text in mote.peak_text:
                if text is not None:
                    text.remove()
            mote.peak_text = [None, None, None]
            mote.peak_drawn = [False, False, False]

        if self.mode != "Peak-To-Peak":
            for text in mote.p2p_text:
                if text is not None:
                    text.remove()
            mote.p2p_text = [None, None, None]
            mote.p2p_drawn = [False, False, False]

        if self.mode != "Peak" and self.mode != "Peak-To-Peak":
            # Resetting scale to auto, manual not needed for non peak modes
            for i in range(self.NO_AXIS):
                plt.sca(ax[i, 1])
                ax[i, 1].set_ylim(auto=True)

        # Implement manual setting of axis limits
        try:
            for i in range(self.NO_AXIS):
                if (self.axisSet):
                    ax[i][1].set_xlim([self.xStart, self.xEnd])
                    ax[i][1].set_ylim([self.yStart, self.yEnd])
                elif self.mode == "Peak" or self.mode == "Peak-To-Peak":
                    ax[i][1].set_xlim(auto=True)
                else:
                    ax[i][1].autoscale(enable=True)
        except:
            traceback.print_exc()
            print("Please enter float axis parameters.")

    def mote_plot_init(self):

        '''Initialise plots based
        on number of connected motes.

        @description
        Creates tkinter notebook object with number of notebook "tabs" equal
        to number of connected motes. Creates tkinter frame for each tab,
        on which a canvas figure can be placed.
        Canvas is split into 6 subplots, using matplotlib subplots to return
        (axis, figure) tuple for later reference.
        Plots are initialised to plot no data, but with plot color assigned.
        "Lines" object keeps track of "Line2D" objects plotted.
        Lines object is updated to update values plotted.
        Motes dictionary is created associating mote with "axes" (3,2) object to plot to.
        funcAnimation called to update graphs with current data every "interval" ms
        Reference kept to animations object in anis to avoid garbage collection.

        @params None

        @returns None '''

        global nb

        AXES = 3
        ROWS = 3
        COLS = 2

        self.f, self.t = mgr.calc_sampling()  # Get lists to plot fft, raw against

        # Keeping reference of figures and animation callbacks for each mote
        self.frames = []
        self.figs = []
        self.anis = []
        self.axs = [[] for _ in range(mgr.num_motes)]
        self.lines = [[] for _ in range(5)]  # Last two rows for 4. Motor speed, 5. Bearing info

        for i in range(mgr.num_motes):
            frame = ttk.Frame(nb)
            self.frames.append(frame)
            nb.pack(side="bottom", fill="both", expand=True)

            fig, ax = plt.subplots(3, 2, figsize=(12, 6), constrained_layout=True)
            self.figs.append(fig)  # Reference needed to keep figure active
            self.axs[i] = ax  # Axes for each plot - used as reference to update plots

            c = FigureCanvasTkAgg(fig, frame)
            c.get_tk_widget().pack(side='bottom', padx=10, pady=10)

            for r in range(ROWS):
                for c in range(COLS):
                    self.lines[i].append(ax[r, c].plot([], [], Mote.colors[r])[0])

                    if c == 0:  # Left plots
                        ax[r, c].set_ylabel('Acceleration (g)')
                        if r == 2:
                            ax[r, c].set_xlabel('Time (s)')

                    else:  # Right plots
                        if r == 2:
                            ax[r, c].set_xlabel('Frequency (Hz)')

            ax[0, 0].set_title("Time series X,Y,Z")
            ax[0, 1].set_title("DFT X,Y,Z")

            nb.add(frame, text="Mote #" + str(i + 1))

            mgr.motes[tuple(mgr.operationalMacs[i])] = (Mote(mgr.adc_num_samples), (ax))
            # Need to loop through specific subplots in ax and animate each one seperately
            self.anis.append(animation.FuncAnimation(fig, self.animate, interval=self.interval,
                                                     fargs=(self.lines[i], ax, i)))

    def disconnect(self):
        global nb
        for ani in self.anis:
            ani.event_source.stop()
        self.anis = []
        for frame in self.frames:
            nb.forget(frame)

    ################################# Motor and bearing plotting for DFT #####################################
    def calc_motor_speed(self, *args):
        """Takes a number of parameters from the user and calculates a number
        of useful 'benchmarks' for plotting motor fault data.

        Returns nothing but updates mote variable for motor benchmark
        """
        speed = float(motor1_entry.get()) / 60
        harm = int(motor2_entry.get())
        motor_flag = True
        motor_speed = [speed * i for i in range(1, harm + 1)]
        for entry in mgr.motes.values():
            mote = entry[0]
            mote.motor_speed = motor_speed

    def clear_motor_plot(self, *args):
        print("Cleared")
        for entry in mgr.motes.values():
            mote = entry[0]
            mote.motor_speed = []
            mote.motor_clear = True

    def calc_bearing_faults(self, *args):
        """Takes a number of parameters from the user and calculates a number
        of useful 'benchmarks' for plotting bearing fault data. These benchmarks are overlaid
        with real data to show in fft plots what frequencies bearing faults are likely to be seen at.

        @params None: Activated by button press reading bearing entry boxes
        @returns None: Updates bearing benchmark variables of mote - later plotted
        """
        n = int(bear1_entry.get())
        fr = int(bear2_entry.get())
        d = int(bear3_entry.get())
        D = int(bear4_entry.get())
        fi = int(bear5_entry.get())

        bpfo = ((n * fr / 2) * (1 - (cos(fi) * d / D)))
        bpfi = ((n * fr / 2) * (1 + (cos(fi) * d / D)))
        ftf = ((fr / 2) * (1 - (cos(fi) * d / D)))
        bsf = ((D * fr) / (2 * d)) * (1 - (cos(fi)) * (d / float(D)) ** 2)

        for entry in mgr.motes.values():
            mote = entry[0]
            mote.bpfi = bpfi
            mote.bpfo = bpfo

    def clear_bearing_faults(self, *args):
        print("Cleared")
        for entry in mgr.motes.values():
            mote = entry[0]
            mote.bpf_clear = True

    def set_mode(self, select):
        '''Change mode based on menu selection'''
        self.mode = select

    def set_axis_auto(self, *args):
        self.axisSet = False

    def get_user_axis_parameters(self, *args):
        try:
            self.axisSet = True
            self.xStart = float(xStartEntry.get())
            self.yStart = float(yStartEntry.get())
            self.xEnd = float(xEndEntry.get())
            self.yEnd = float(yEndEntry.get())
        except ValueError:
            traceback.print_exc()
            print("Invalid axis length input. Please enter a float.")


# ====================================== math functions ====================================
def peak(dataList):
    '''Takes in list and returns float furthest
    deviation from 0'''
    max = 0
    for data in dataList:
        if fabs(data) > max:  # Check absolute value against current max
            max = data
    return max


def peak_to_peak(dataList):
    '''Takes in list and return float max -min value'''
    max = 0
    min = 0
    for data in dataList:
        if data > max:
            max = data
        elif data < min:
            min = data
    return max - min


def mean(dataList):
    '''Takes in list and return float average value'''
    mean = 0
    for data in dataList:
        mean += data
    return mean / float(len(dataList))


def rms(dataList):
    '''Takes in list and returns float Root mean square value'''
    ms = 0
    for data in dataList:
        ms += (data ** 2)
    ms /= float(len(dataList))
    return (float(sqrt(ms)))


def standard_deviation(dataList):
    '''Takes in a list and returns float standard deviation'''
    n = len(dataList)
    xBar = mean(dataList)
    total = 0
    for x in dataList:
        total += (x - xBar) ** 2
    return sqrt((total / float(n - 1)))


def crest(dataList):
    '''Takes in list and returns float crest factor'''
    try:
        absPeak = float(peak(dataList))
        xRms = float(rms(dataList))
        return absPeak / xRms
    except:
        traceback.print_exc()


def set_alarm(self):
    for entry in mgr.motes.values():
        mote = entry[0]
        if var == 1: mote.peak_alarm = float(peak_entry.get())
        if var == 2: mote.p2p_alarm = float(p2p_entry.get())
        if var == 3: mote.rms_alarm = float(rms_entry.get())
        if var == 4: mote.std_alarm = float(std_entry.get())
        if var == 5: mote.kurt_alarm = float(kurts_entry.get())
        if var == 6: mote.skew_alarm = float(skew_entry.get())
        if var == 7: mote.crest_alarm = float(crest_entry.get())
        mote.alarm_flag = 1
    print("Alarm set")


def reset_graphs():
    '''Clears data for each math function of each mote'''
    for entry in mgr.motes.values():
        mote = entry[0]
        mote.reset()


def get_num_data(self):
    '''Getter method for save_frames textbox variable'''
    global save_frames
    global excelButton
    try:
        save_frames = int(dataPointsEntry.get())
        excelButton.config(state="enabled")
    except:
        print("Please enter a valid integer")


# ================================= Database Functions ========================================#
def save_to_file(self, ID, ax, raw, fft):
    global save_frames
    '''Save number of data sets to SQL database  file

    @description
    Saves number of frames of data specified by user to SQL file "motedata.db" in project directory
    Called repeatedly but only saves if save_frames has been set

    @params
    ID - Mote ID
    ax - Axis value (x, y, z)
    raw - Raw mote data
    fft - fft mote data

    @returns None
    '''
    if not save_frames:
        return

    index = 0

    if ax == 'x':
        index = 0
    elif ax == 'y':
        index = 1
    elif ax == 'z':
        index = 2

    # Get list of latest entry in each mode for given axis (x, y or z)
    latest_stats = [
        self._peaks[index][-1],
        self._p2ps[index][-1],
        self._means[index][-1],
        self._sds[index][-1],
        self._skews[index][-1],
        self._kurts[index][-1],
        self._crests[index][-1]
    ]

    def create_new_table():
        '''Creates an SQL table in the database with a given name

        Mote    Time    Raw Data    FFT data
        #####################################
        1       unix    429314      853910
        '''

        c.execute(
            'CREATE TABLE IF NOT EXISTS mote(ID TEXT, axis TEXT, time REAL, raw REAL, fft REAL)')  # SQL command to create table called "mote" if none exists
        c.execute(
            'CREATE TABLE IF NOT EXISTS stats(ID TEXT, axis TEXT, time REAL, peaks REAL, p2ps REAL, rms REAL, sds REAL, skewness REAL, kurtosis REAL, crest REAL)')  # Create table "stats"

    def dynamic_data_entry(ID, ax, raw, fft, time=time.clock()):
        '''Function to automatically format Python variables into SQL syntax
        and insert into table mote'''
        c.execute("INSERT INTO mote (ID, axis, time, raw, fft) VALUES (?, ?, ?, ?, ?)",
                  (ID, ax, time, raw, fft))

    def extra_data_entry(stats):
        '''Automatically format Python variables into SQL syntax
        and insert into table mote'''
        c.execute(
            "INSERT INTO stats (ID, axis, time, peaks, p2ps, rms, sds, skewness, kurtosis, crest) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            (ID, ax, time.clock(), stats[0], stats[1], stats[2], stats[3], stats[4], stats[5], stats[6]))

    def commit_frame(ID, ax, raw, fft, stats):
        '''Enter data for raw, fft, and stats into an SQL database
        under Tables "Mote" and "Stats"'''
        extra_data_entry(stats)
        dynamic_data_entry(None, None, None, None, None)  # First byte contains no information
        for i in range(len(fft)):
            dynamic_data_entry(ID, ax, raw[i], fft[i])
        for i in range((len(fft)), len(raw)):
            dynamic_data_entry(ID, ax, raw[i], 0)
        conn.commit()

    if save_frames:
        save_frames -= 1
        try:
            conn = sqlite3.connect('motedata.db')
            c = conn.cursor()  # SQL syntax creates cursor to "write" to SQL database
            create_new_table()  # Creates table if it does not exist already
            commit_frame(ID, ax, raw, fft,
                         latest_stats)  # Save one frame of data (raw, fft) to the database, indexed by mote ID
            print(" SAVING DATA - MOTE: {} ".format(ID).center(60, "~"))
        except:
            print(traceback.print_exc())
            print("Failed to create SQL table.")
        finally:
            c.close()  # Cursor must be closed on program close
            conn.close()


def convert_sql2excel(ignore=1):
    '''Takes existing SQL database and coverts two tables:
    mote and stats to Excel sheets. Saves current directory. '''

    # ignore input is there as a hack because tkinter tries to pass an argument
    # to this function after a button press
    try:
        df1 = pd.read_sql("SELECT * from mote",
                          sqlite3.connect("motedata.db"))  # Create pandas dataframe to read from SQL
        df2 = pd.read_sql("SELECT * from stats", sqlite3.connect("motedata.db"))

        with pd.ExcelWriter("motedata.xlsx") as writer:  # Use pandas to write to excel
            df1.to_excel(writer, 'sheet1')
            df2.to_excel(writer, 'sheet2')
            writer.save()
        print("Successfully saved database to motedata.xlsx")
    except:
        print("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\
Failed to read SQL file. The problem is one of the following:\n\
1: Data must be saved to SQL before Excel conversion\n\
2: motedata.xlsx is already open - close to continue\n\
3: The process was cancelled by the user\n\
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n")


# ============================ helper functions ================================ #

def update_offsetsM1():
    global offset_removal_method
    offset_removal_method=1
    update_offsets()

def update_offsetsM2():
    global offset_removal_method
    offset_removal_method=2
    update_offsets()

def update_offsets():
    for entry in mgr.motes.values():
        mote = entry[0]
        mote.remove_offsets = True


def rem_offset():
    print('Calibrating offset')
    for entry in mgr.motes.values():
        mote = entry[0]
        mote.remove_offset()


## =========================== Frame and menu initilisation  ===========================##

def connection_frame_init(frame):
    '''Holds user input fields for connecting, disconnecting from manager
    and removing mote raw data offset. Indicator lights set to grey'''
    global moteInd1, moteInd2, moteInd3
    global comportEntry
    global cv

    frame.pack(side="top", fill="both", expand=True)
    frame.grid_rowconfigure(0, weight=1)
    frame.grid_columnconfigure(0, weight=1)

    TitleLabel = ttk.Label(frame,
                           text="Analog Devices Condition Based Monitoring Demonstrator",
                           font=LARGE_FONT)
    TitleLabel.pack(side='top')
    ttk.Label(frame, text="Enter Serial Port e.g. COM7").pack(side='left', padx=10, pady=10)
    comportEntry = ttk.Entry(frame)
    comportEntry.pack(side='left', padx=10, pady=10)
    comButton = ttk.Button(frame, text="Connect")
    comButton.pack(side='left', padx=10, pady=10)
    comButton.bind("<Button-1>", mgr.connect)
    disButton = ttk.Button(frame, text="Disconnect")
    disButton.pack(side='left', padx=10, pady=10)
    disButton.bind("<Button-1>", mgr.disconnect)

    offsetCheck2 = ttk.Checkbutton(frame,
                                  text="Remove ADC offset (only perform when mote is stationary,\ncalculates average values of 1 frame and removes from future frames)",
                                  command=update_offsetsM2)
    offsetCheck2.pack(side='left', padx=10, pady=10)

    offsetCheck1 = ttk.Checkbutton(frame,
                                  text="Remove ADC offset (only perform when mote is\nstationary and upright, assumes G in Z-axis=1)",
                                  command=update_offsetsM1)
    offsetCheck1.pack(side='left', padx=10, pady=10)


    cv = tk.Canvas(frame, width=300, height=100)  # Indicator lights canvas setup
    cv.pack(side='left')


def save_data_window(root):
    '''Creates window for user to set number of frames to save to SQL database
    SQL data saved can also be converted to excel file, both saved to current directory'''
    global dataPointsEntry
    global excelButton
    frame3 = tk.Toplevel(root)
    tk.Toplevel.wm_title(frame3, "Save Data")
    dataPointsEntry = ttk.Entry(frame3)
    dataPointsEntry.pack(side='left', padx=10, pady=10)

    dataPointsButton = ttk.Button(frame3, text="Enter")
    dataPointsButton.bind("<Button-1>", get_num_data)
    dataPointsButton.pack(side='right', padx=10, pady=10)

    excelButton = ttk.Button(frame3, text="Convert to Excel")
    excelButton.bind("<Button-1>", convert_sql2excel)
    excelButton.pack(side='right', padx=10, pady=10)
    excelButton.config(state="disabled")


def sampling_window(root):
    '''Create and display user sampling frequency selection window'''
    global updatingLabel
    global frequencyCombo
    global adcNumSamplesCombo
    global sleepDurCombo
    global bitsCombo

    frame4 = tk.Toplevel(root)
    tk.Toplevel.wm_title(frame4, "Sampling parameters")

    frequencyString   = tk.StringVar()
    adcNumSampsString = tk.StringVar()
    sleepDurString    = tk.StringVar()
    bitsString        = tk.StringVar()

    ttk.Label(frame4, text="Sampling Frequency (Hz) ").grid(row=0, column=0, sticky=W, padx=10, pady=5)
    ttk.Label(frame4, text="Number of ADC Samples ").grid(row=1, column=0, sticky=W, padx=10, pady=5)
    ttk.Label(frame4, text="Sleep Duration (s) ").grid(row=2, column=0, sticky=W, padx=10, pady=5)

    frequencyCombo = ttk.Combobox(frame4,
                                  textvariable=frequencyString,
                                  values=[256, 512, 1024, 2048, 4096, 8192, 16384, 32768],
                                  width=7)
                                  #values=[250, 500, 1000, 2000, 5000, 10000, 20000],

    # Number of samples needs to be a power of 2... will not be run if greater than 1024 samples collected
    adcNumSamplesCombo = ttk.Combobox(frame4,
                                  textvariable=adcNumSampsString,
                                  values=[256, 512, 1024, 2048, 4096, 8192, 16384, 32768],
                                  width=7)

    sleepDurCombo = ttk.Combobox(frame4,
                                  textvariable=sleepDurString,
                                  values=[0, 15, 30, 60, 120, 240, 480, 960, 1920],
                                  width=7)

    frequencyCombo.grid(row=0, column=1, sticky=W, padx=(0, 10), pady=5)
    adcNumSamplesCombo.grid(row=1, column=1, sticky=W, padx=(0, 10), pady=5)
    sleepDurCombo.grid(row=2, column=1, sticky=W, padx=(0, 10), pady=5)

    # OK Button calls mgr.get_user_sampling_parameters
    sampParamsOkBtn = tk.Button(frame4, text="OK", pady=5, command=mgr.get_user_sampling_parameters)
    sampParamsOkBtn.grid(row=3, column=1, sticky=W, padx=(0, 10), pady=5)



def set_axis_window(root):
    '''Creates window to manually set right-graph axis limits '''
    global xStartEntry, yStartEntry
    global xEndEntry, yEndEntry

    frame5 = tk.Toplevel(root)
    tk.Toplevel.wm_title(frame5, "Axis parameters")

    xStartLabel = ttk.Label(frame5, text='X-Axis (start) ').grid(row=0, column=0, sticky=W, padx=10, pady=5)
    yStartLabel = ttk.Label(frame5, text='Y-Axis (start) ').grid(row=1, column=0, sticky=W, padx=10, pady=5)
    xEndLabel = ttk.Label(frame5, text='X-Axis (end) ').grid(row=0, column=2, sticky=W, padx=10, pady=5)
    yEndLabel = ttk.Label(frame5, text='Y-Axis (end) ').grid(row=1, column=2, sticky=W, padx=10, pady=5)

    xStartEntry = ttk.Entry(frame5, width=7)
    yStartEntry = ttk.Entry(frame5, width=7)
    xEndEntry = ttk.Entry(frame5, width=7)
    yEndEntry = ttk.Entry(frame5, width=7)

    xStartEntry.grid(row=0, column=1, sticky=W, pady=5)
    yStartEntry.grid(row=1, column=1, sticky=W, pady=5)
    xEndEntry.grid(row=0, column=3, sticky=W, pady=2)
    yEndEntry.grid(row=1, column=3, sticky=W, pady=5)

    autoButton = ttk.Button(frame5, text='Auto', width=5)
    enterXY = ttk.Button(frame5, text='Set', width=5)

    autoButton.bind("<Button-1>", controller.set_axis_auto)
    enterXY.bind("<Button-1>", controller.get_user_axis_parameters)

    autoButton.grid(row=0, column=4, sticky=W, padx=25, pady=5)
    enterXY.grid(row=1, column=4, sticky=W, padx=25, pady=5)


def motor_window(root):
    global motor1_entry
    global motor2_entry
    frame6 = tk.Toplevel(root)
    tk.Toplevel.wm_title(frame6, "Motor Speed")

    ttk.Label(frame6, text="Enter motor speed").grid(pady=10)
    motor1_entry = ttk.Entry(frame6, width=7)
    motor1_entry.grid(row=0, column=1, padx=7)
    motor1_entry.insert(END, '5000')  # FOR TESTING ONLY

    ttk.Label(frame6, text="Enter number of harmonics").grid(pady=10)
    motor2_entry = ttk.Entry(frame6, width=7)
    motor2_entry.grid(row=1, column=1, padx=7)
    motor2_entry.insert(END, '5')  # FOR TESTING ONLY

    motorButton = ttk.Button(frame6, width=5, text="Enter")
    motorButton.grid(row=1, column=2, padx=3)
    motorButton.bind("<Button-1>", controller.calc_motor_speed)

    clear = ttk.Button(frame6, width=5, text="Clear")
    clear.grid(row=0, column=2, padx=3)
    clear.bind("<Button-1>", controller.clear_motor_plot)


def bearing_window(root):
    global bear1_entry
    global bear2_entry
    global bear3_entry
    global bear4_entry
    global bear5_entry

    frame7 = tk.Toplevel(root)
    tk.Toplevel.wm_title(frame7, "Bearing Characteristics")

    ttk.Label(frame7, text="Enter number of rolling elements").grid(pady=10)
    bear1_entry = ttk.Entry(frame7, width=7)
    bear1_entry.grid(row=0, column=1, padx=7)
    bear1_entry.insert(END, '2')  # TESTING ONLY

    ttk.Label(frame7, text="Enter shaft speed (RPM)").grid(pady=10)
    bear2_entry = ttk.Entry(frame7, width=7)
    bear2_entry.grid(row=1, column=1, padx=7)
    bear2_entry.insert(END, '2000')  # TESTING ONLY

    ttk.Label(frame7, text="Enter rolling element diameter (mm)").grid(pady=10)
    bear3_entry = ttk.Entry(frame7, width=7)
    bear3_entry.grid(row=2, column=1, padx=7)
    bear3_entry.insert(END, '2')  # TESTING ONLY

    ttk.Label(frame7, text="Enter bearing pitch diameter (mm)").grid(pady=10)
    bear4_entry = ttk.Entry(frame7, width=7)
    bear4_entry.grid(row=3, column=1, padx=7)
    bear4_entry.insert(END, '20')  # TESTING ONLY

    ttk.Label(frame7, text="Enter angle of load from radial plane (degrees)").grid(pady=10)
    bear5_entry = ttk.Entry(frame7, width=7)
    bear5_entry.grid(row=4, column=1, padx=7)
    bear5_entry.insert(END, '0')  # TESTING ONLY

    enter = ttk.Button(frame7, width=5, text="Enter")
    enter.grid(row=3, column=2, rowspan=2, padx=3, sticky=tk.N + tk.S)
    enter.bind("<Button-1>", controller.calc_bearing_faults)

    clear = ttk.Button(frame7, width=5, text="Clear")
    clear.grid(row=2, column=2, padx=3)
    clear.bind("<Button-1>", controller.clear_bearing_faults)


def axis_select_window(self):
    '''Creates window allowing user to select x,y,z axes to receive from mote
    Fewer axes leads to faster updating of the remaining axes'''
    frame9 = tk.Toplevel(root)
    tk.Toplevel.wm_title(frame9, "Select Axis to Receive")

    ttk.Label(frame9, text="Select Axis to receive").pack()
    ttk.Checkbutton(frame9, text="X", variable=v1).pack()
    ttk.Checkbutton(frame9, text="Y", variable=v2).pack()
    ttk.Checkbutton(frame9, text="Z", variable=v3).pack()
    ttk.Button(frame9, text="Enter", command=mgr.send_axis_info).pack(side="right")


def create_alarm_window(self):
    '''Creates window where user can set alarm limits on statistics: peak, rms...'''
    global peak_entry, p2p_entry, rms_entry, std_entry
    global kurts_entry, skew_entry, crest_entry
    frame8 = tk.Toplevel(root)
    tk.Toplevel.wm_title(frame8, "Alarm settings")

    # name var_num entry_box
    choices = [
        ("Peak", 1),
        ("P2P", 2),
        ("RMS", 3),
        ("Standard Deviation", 4),
        ("Kurtosis", 5),
        ("Skew Factor", 6),
        ("Crest Factor", 7)
    ]

    def enable_entry_box():
        '''Enables only one alarm entry box based on user button press'''
        global v
        global var
        var = v.get()

        peak_entry.config(state="disabled")
        p2p_entry.config(state="disabled")
        rms_entry.config(state="disabled")
        std_entry.config(state="disabled")
        kurts_entry.config(state="disabled")
        skew_entry.config(state="disabled")
        crest_entry.config(state="disabled")

        if var == 1:
            peak_entry.config(state="enabled")
        elif var == 2:
            p2p_entry.config(state="enabled")
        elif var == 3:
            rms_entry.config(state="enabled")
        elif var == 4:
            std_entry.config(state="enabled")
        elif var == 5:
            kurts_entry.config(state="enabled")
        elif var == 6:
            skew_entry.config(state="enabled")
        elif var == 7:
            crest_entry.config(state="enabled")

    for mode, val in choices:
        tk.Radiobutton(frame8, text=mode, width=16,
                       variable=v, command=lambda: enable_entry_box(),
                       value=val, indicatoron=0).grid(column=0, padx=5)

    peak_entry = ttk.Entry(frame8, width=7, state="disabled")
    peak_entry.grid(row=0, column=1, padx=7)

    p2p_entry = ttk.Entry(frame8, width=7, state="disabled")
    p2p_entry.grid(row=1, column=1, padx=7)

    rms_entry = ttk.Entry(frame8, width=7, state="disabled")
    rms_entry.grid(row=2, column=1, padx=7)

    std_entry = ttk.Entry(frame8, width=7, state="disabled")
    std_entry.grid(row=3, column=1, padx=7)

    kurts_entry = ttk.Entry(frame8, width=7, state="disabled")
    kurts_entry.grid(row=4, column=1, padx=7)

    skew_entry = ttk.Entry(frame8, width=7, state="disabled")
    skew_entry.grid(row=5, column=1, padx=7)

    crest_entry = ttk.Entry(frame8, width=7, state="disabled")
    crest_entry.grid(row=6, column=1, padx=7)

    reset = ttk.Button(frame8, width=5, text="Reset")
    reset.grid(row=4, column=2, padx=3)
    reset.bind("<Button-1>", mgr.reset_alarm)

    enter = ttk.Button(frame8, width=5, text="Enter")
    enter.grid(row=5, column=2, padx=3, rowspan=2, sticky=tk.N + tk.S)
    enter.bind("<Button-1>", mgr.set_alarm)


def menu_init(menu):
    '''Creates and attaches command bindings
    to dropdown menus'''
    root.config(menu=menu)

    selMode = tk.Menu(menu)
    menu.add_cascade(label='Mode', menu=selMode)
    selMode.add_command(label='1. DFT', command=lambda: controller.set_mode("DFT"))
    selMode.add_command(label='2. Peak', command=lambda: controller.set_mode("Peak"))
    selMode.add_command(label='3. Peak-to-Peak', command=lambda: controller.set_mode("Peak-To-Peak"))
    selMode.add_command(label='4. Mean', command=lambda: controller.set_mode("Mean"))
    selMode.add_command(label='5. Standard Deviation', command=lambda: controller.set_mode("Standard Deviation"))
    selMode.add_command(label='6. Kurtosis', command=lambda: controller.set_mode("Kurtosis"))
    selMode.add_command(label='7. Skew Factor', command=lambda: controller.set_mode("Skew Factor"))
    selMode.add_command(label='8. Crest Factor', command=lambda: controller.set_mode("Crest Factor"))

    action = tk.Menu(menu)
    menu.add_cascade(label='Action', menu=action)
    action.add_command(label='Save Data', command=lambda: save_data_window(root))
    action.add_command(label='Set Alarm', command=lambda: create_alarm_window(root))

    reset = tk.Menu(menu)
    menu.add_cascade(label='Reset', menu=reset)
    reset.add_command(label='Reset', command=reset_graphs)

    # Options menu, each option opens its own window, tk.Toplevel
    Options = tk.Menu(menu)
    menu.add_cascade(label='Settings', menu=Options)
    Options.add_command(label='1. Sampling Parameters', command=lambda: sampling_window(root))
    Options.add_command(label='2. Right Graph Limits', command=lambda: set_axis_window(root))
    Options.add_command(label='3. Motor Speed', command=lambda: motor_window(root))
    Options.add_command(label='4. Bearing Characteristics', command=lambda: bearing_window(root))
    Options.add_command(label='5. Axes Received', command=lambda: axis_select_window(root))

    Help = tk.Menu(menu)
    menu.add_cascade(label='Help', menu=Help)
    Help.add_command(label='Network Info', command=mgr.get_network_info)
    Help.add_command(label='CBM Help', command=mgr.show_help)


def sendReadyMsg():
    cmd_descriptor = '11xxxxxx'  # Manager Ready Signal
    msg = 'xxxxxxxx' + '0xxxxxxx' + 'xxxxxxxx' + 'xxxxxxxx' + 'xxxxxxxx' + cmd_descriptor
    print "Sending Ready Msg"
    mgr.send_to_all_motes(msg)

# ============================ main ============================================

try:
    global TerminalMode
    global T_com_port
    
    T_com_port = sys.argv[1]
    print("\nENTERING TERMINAL MODE... if motedata.xlsx already exists, ensure that it is closed. Data will be appended to it\n")
    TerminalMode = True
except:
    print("\nNo arguments passed when calling script. Initiating GUI Mode")
    TerminalMode = False

if TerminalMode:
    try:
        global T_num_samples
        global T_samp_freq
        global T_num_stages
        global T_axis_en
        global T_offset_removal

        T_samp_freq = sys.argv[2]
        print("Sampling Freq                                   : {} Hz".format(T_samp_freq))
        T_num_samples = sys.argv[3]
        print("Number of Samples in a Frame (per axis)         : {}".format(T_num_samples))
        T_num_stages = int(sys.argv[4])
        print("Number of Stages to perform                     : {}".format(T_num_stages))
        T_axis_en = sys.argv[5]
        print("Enabled axes (XYZ)                              : {}".format(T_axis_en))
        T_sleep_s = sys.argv[6]
        print("Sleep duration after one Stage is performed     : {} s".format(T_sleep_s))
        T_offset_removal = int(sys.argv[7])

        if T_offset_removal == 1:
            msg = "X,Y=0, Z=1 Mode"
        elif T_offset_removal == 1:
            msg = "Mean Removal Mode"
        else:
            msg = "False"

        #offset_msg = True if T_offset_removal == 1 else False
        print("ADC Offset Removal                              : {}".format(msg))
        if T_offset_removal:
            print("Adding 1 to number of expected Stages since offset removal is desired.")
            print("1st frame used to calculate offset. Subsequent frames shall have offsets removed")
            T_num_stages += 1


        
        # ----- Check Inputs ----- #
        # Find total number of expected frames and ensure axis input is correct
        num_axes_en = 0
        if len(T_axis_en) != 3:
            print "!!! Axis parameter is not 3 characters long: ", T_axis_en
            raise ValueError()
        else:
            for i in range(3):
                if T_axis_en[i] == '1':
                    num_axes_en += 1
                elif T_axis_en[i] != '0':
                    print "!!! Axis parameter characters can only be '0' or '1': ", T_axis_en
                    raise ValueError()

        # Check stages
        if T_num_stages < 1:
            print("!!! Number of Stages to perform needs to be a positive integer")
            raise ValueError()
        
        T_expected_num_frames = T_num_stages * num_axes_en
        save_frames = T_expected_num_frames
        print("There are {} axes enabled. User defined {} frames expected in total\n".format(num_axes_en, T_expected_num_frames))
        
        # Check num samples
        allowedFftNum = (32, 64, 128, 256, 512, 1024)
        if int(T_num_samples) <= FLASH_PAGE_SAMPLES:
            if int(T_num_samples) not in allowedFftNum:
                print("!!! If num_samples <= 1024 the allowed amounts are only {} due to FFT calculation".format(allowedFftNum))
                raise ValueError()
        else:
            if int(T_num_samples) > 1e6: 
                print("!!! If num_samples > 1024 any integer is allowed up to 165,000,000")
                print("There is 1/3GB reserved per axis in the off-chip flash")
                raise ValueError()

        # Check sampling frequency
        max_samp_freq = 1e6
        if (int(T_samp_freq) < 0) or (int(T_samp_freq) > max_samp_freq):
            print("!!! Sampling frequency must be a positive number less than {} Hz".format(max_samp_freq))


    except:
        print("\n!!! Incorrect arguments passed when calling script The format for the command should be as follows:")
        print("python CBM_app.py com<PORT_NUM> <SAMP_FREQ> <NUM_SAMPLES> <NUM_STAGES> <EN_AXES_XYZ> <SLEEP_DURATION_S> <OFFSET_REMOVAL>")
        print("\nExample:")
        print("python CBM_app.py com9 500 1024 2 110 120 1 0")
        quit()


if not TerminalMode:
    root = tk.Tk()  # Creates base Tkinter window

try:
    
    mgr = Manager()

    if TerminalMode:
        T_frame_count = 0
        numMotes      = 0

        while(numMotes == 0):
            mgr.connect()
            numMotes = len(mgr.macList)
            if numMotes == 0:
                print "Waiting 5s before retrying connection attempt"
                mgr.disconnect()
                time.sleep(5)

        print "Waiting for mote to indicate it is ready"
        mgr.axis_sel_msg = T_axis_en + 'xxxxx'  # Must fill to 8 bytes
        mgr.adc_num_samples = int(T_num_samples)
        while not moteStartAckd:
            # Wait for mote to say its ready
            time.sleep(1)

        if T_offset_removal == 1:
            update_offsetsM1()
        elif T_offset_removal == 2:
            update_offsetsM2()
        
        while T_frame_count < T_expected_num_frames:
            time.sleep(1)

        mgr.disconnect(None)  # Disconnect on program finish

        # Converting SQL DB to .xlsx file
        print "\nConverting SQL database to .xlsx file... Please wait" 
        convert_sql2excel()

    else:
        controller = AnimationController()
        style.use('ggplot')  # Tkinter command, choose plotting style for graphs

        tk.Tk.wm_title(root, "Condition Based Monitoring GUI")  # Draw the main GUI window
        v = tk.IntVar()  # Tk int variable used with alarm setting checkboxes
        v1 = tk.IntVar()
        v2 = tk.IntVar()
        v3 = tk.IntVar()
        v.set(0)  # Initialise to no check box selected
        var = 0  # Persistant variable updated by v, keeps alarm active when option box is closed

        frame = tk.Frame(root)  # Connect and indicator frame
        connection_frame_init(frame)

        nb = ttk.Notebook(root)  # Notebook for plotting mote data to canvas

        menu = tk.Menu(root)  # Dropdown menu for graph options
        menu_init(menu)

        root.mainloop()  # Begins tkinter mainloop thread, allows the program to keep running

    mgr.disconnect(None)  # Disconnect on program finish

except:
    traceback.print_exc()
    mgr.disconnect(None)  # Disconnect on program finish
    print("Script ended with an error.")
    quit()


